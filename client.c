#include "client.h"

int verbose = 1;

int main(int c, char* argv[])
{
    // Local Vars
    char* prmpt = "Fileserver> ";
    struct ConfigData config_data;

    // Parse the config file
    config_parse(&config_data);


    // Primary loop for command line interface
    for (;;) {

        // Local Vars
        char inp_buffer[MAX_BUF_LEN];
        char* cmd = NULL;
        char* filename = NULL;

        // Get command from standard input
        if (getLine(prmpt, inp_buffer, MAX_BUF_LEN) != -1) {

            // Parse command and possibly filename from inp_buffer
            cmd = strtok(inp_buffer, " ");
            filename = strtok(NULL, "\n");

            // Check if command is one of commands and call applicable routine
            if (strcmp(cmd, "get") == 0) {
                get_routine(filename, &config_data);
            }
            else if (strcmp(cmd, "put") == 0) {
                put_routine(filename, &config_data);
            }
            else if (strcmp(cmd, "list") == 0) {
                printf("LIST implementation missing\n");
            }
            else if (strcmp(cmd, "exit") == 0) {
                break;
            }
            else {
                printf("Command not recognized: %s\n", cmd);
            }
        }


    }

    return 0;
}

void put_routine(char *filename, struct ConfigData *config_data)
{
    // Local Vars
    int sockfd[4], ifile_size;
    char msgheader[MAX_MSG_LEN];
    char header_format[] = "put %s %d\n";
    FILE *ifile;

    // If filename doesn't exist, return
    if (access(filename, F_OK) == -1)  {
        printf("File not found: %s\n", filename);
        return;
    }

    // Create socket descriptors for available servers and authenticate username/password
    for (int i = 0; i < 4; i++) {
        sockfd[i] = create_socket(i, config_data);
        if (sockfd[i] != -1 && handshake(sockfd[i], config_data) == 0) {
            if (verbose) printf("INVALID username/password\n");
            return;
        }
    }

    // Open file for reading as binary
    ifile = fopen(filename, "rb");
    if (!ifile) {
        printf("Uh oh, something bad happened\n");
        return;
    }
    // Get file size
    else {
        fseek(ifile, 0L, SEEK_END);
        ifile_size = ftell(ifile);
        fseek(ifile, 0L, SEEK_SET);
    }

    // Serialize message header and send to server(s)
    snprintf(msgheader, sizeof(msgheader), header_format, filename, ifile_size);

    // Send message header to available servers
    for (int i = 0; i < 4; i++) {
        if (sockfd[i] != -1)
            send(sockfd[i], msgheader, sizeof(msgheader), 0);
    }


    // TODO: BEGIN THE FILE TRANSFER
    send_file(ifile, sockfd, ifile_size);
    fclose(ifile);

    // Clean up because bugs errywhere
    bzero(msgheader, sizeof(msgheader));
    for (int i = 0; i < 4; i++) {
        if (sockfd[i] > 0) close(sockfd[i]);
    }
}

void get_routine(char* filename, struct ConfigData *config_data)
{
    // Local Vars
    int sockfd[4];
    char header_format[] = "get %s 0\n";
    char msgheader[MAX_MSG_LEN];

    // Create socket descriptors for available servers and authenticate username/password
    for (int i = 0; i < 4; i++) {
        sockfd[i] = create_socket(i, config_data);
        if (sockfd[i] != -1 && handshake(sockfd[i], config_data) == 0) {
            if (verbose) printf("INVALID username/password\n");
            return;
        }
    }

    // Serialize message header
    snprintf(msgheader, sizeof(msgheader), header_format, filename);

    // Send message header to available servers
    for (int i = 0; i < 4; i++) {
        if (sockfd[i] != -1)
            send(sockfd[i], msgheader, sizeof(msgheader), 0);
    }

    bzero(msgheader, sizeof(msgheader));

}


// Send the relevant file chunks to each of the servers
void send_file(FILE *ifile, int sockfd[], unsigned int filesize)
{
    // Local Vars
    char pairs[4][MAX_MSG_LEN];
    char *filebuf = NULL;
    unsigned int rbytes = 0, sbytes = 0;

    // Hard code the file pairs to store
    strcpy(pairs[0], "1 2\n");
    strcpy(pairs[1], "2 3\n");
    strcpy(pairs[2], "3 4\n");
    strcpy(pairs[3], "4 1\n");

    // Send msg to server to indicate what chunks to keep
    for (int i = 0; i < 4; i++) {
        if (sockfd[i] > 0) {
            send(sockfd[i], pairs[i], sizeof(pairs[i]), 0);
        }
    }

    // Read the entire file
    filebuf = malloc(filesize);
    while(rbytes < filesize) {
        rbytes += fread(filebuf, 1, filesize, ifile);
        printf("rbytes = %d\n", rbytes);
    }

    // Send entire file to all servers
    for (int i = 0; i < 4; i++) {
        if (sockfd[i] > 0) {
            sbytes = send(sockfd[i], filebuf, filesize, 0);
        }
    }

    // Done on client side

}


// Send username and password to server
// return 0: Invalid credentials
// return 1: Valid credentials
int handshake(int server, struct ConfigData *config_data)
{
    // Local Vars
    char userpass[MAX_MSG_LEN];
    char junk[10];
    int data_len;

    // Create username and password pair from config_data
    strcpy(userpass, config_data->name);
    strcat(userpass, " ");
    strcat(userpass, config_data->password);
    strcat(userpass, "\n");

    // Send the user password pair
    send(server, userpass, sizeof(userpass), 0);

    // If no response, return with 'error'
    data_len = recv(server, junk, sizeof(junk), 0);
    return (data_len > 0) ? 1 : 0;
}

int create_socket(int server_num, struct ConfigData *config_data)
{
    // Local Vars
    struct sockaddr_in server;
    struct timeval tv;
    int port_number = config_data->serv_port[server_num];

    // Populate server struct
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(port_number);

    // Populate sockopts
    tv.tv_sec = 3;  // recv timeout in seconds
    tv.tv_usec = 0;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Socket could not be created\n");
        return -1;
    }

    // set sockoption to timeout after blocking for 3 seconds on recv
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

    // Try to connect
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        if (verbose) printf("Server %d is not responding...\n", server_num);
        return -1;
    }

    return sock;


}
void config_parse(struct ConfigData* config_data)
{
    // Local Vars
    char conf_line[MAX_BUF_LEN];
    FILE *config_file;

    config_file = fopen("./dfc.conf", "r");
    if (!config_file) {
        perror("Could not find ./dfc.conf");
        exit(-1);
    }

    // Loop through file and store relevant information into struct
    int serv_ctr = 0;
    while (fgets(conf_line, MAX_BUF_LEN, config_file))
    {
        // Local Vars
        char* line_type;

        // Don't parse comments
        if (conf_line[0] != '#' && conf_line[0] != ' ') {
            line_type = strtok(conf_line, " ");

            if (strcmp(line_type, "Server") == 0) {
                config_data->serv_name[serv_ctr] = strdup(strtok(NULL, " "));
                config_data->serv_addr[serv_ctr] = strdup(strtok(NULL, ":"));
                config_data->serv_port[serv_ctr] = atoi(strdup(strtok(NULL, "\n")));
                serv_ctr++;
            }
            else if (strcmp(line_type, "Username:") == 0) {
                config_data->name = strdup(strtok(NULL, "\n"));
            }
            else if (strcmp(line_type, "Password:") == 0) {
                config_data->password = strdup(strtok(NULL, "\n"));
            }

        }

    }
    fclose(config_file);

}


static int getLine (char *prmpt, char *buff, size_t sz)
{
    int ch, extra;

    if (prmpt != NULL) {
        printf ("%s", prmpt);
        fflush (stdout);
    }

    if (fgets (buff, sz, stdin) == NULL)
        return 1;

    if (buff[strlen(buff)-1] != '\n') {
        extra = 0;
        while (((ch = getchar()) != '\n') && (ch != EOF))
            extra = 1;
        return (extra == 1) ? 2 : 0;
    }

    if (strlen(buff) == 1) return -1;

    buff[strlen(buff)-1] = '\0';

    return 0;
}
