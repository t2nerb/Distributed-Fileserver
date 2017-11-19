#include "client.h"

int verbose = 0;

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
                list_routine(&config_data);
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

void list_routine(struct ConfigData *config_data)
{
    // Local Vars
    int sockfd[4];
    int snum = -1;
    int construct_flag = 0;
    // unsigned int rebytes = 0;
    char list_format[] = "list %s 0\n";
    char list_header[MAX_MSG_LEN];
    char list_msg[MAX_BUF_LEN];
    char *filename = NULL;

    // Create socket descriptors for available servers and authenticate username/password
    for (int i = 0; i < 4; i++) {
        sockfd[i] = create_socket(i, config_data);
        if (sockfd[i] != -1 && handshake(sockfd[i], config_data) == 0) {
            if (verbose) printf(" ERROR: Invalid username/password\n");
            return;
        }
    }

    // Check if file can be reconstructed or not
    if ((sockfd[0] > 0 && sockfd[2] > 0) || (sockfd[1] > 0 && sockfd[3] > 0)) {
        construct_flag = 1;
    }

    // Send message to available servers and only utilize first server
    snprintf(list_header, sizeof(list_header), list_format, "list");
    for(int i = 0; i < 4; i++) {
        if (sockfd[i] > 0 && snum == -1) {
            send(sockfd[i], list_header, sizeof(list_header), 0);
            snum = i;
        }
        else if (sockfd[i] > 0 && snum != -1) {
            send(sockfd[i], "exit exit 0\n", sizeof(list_header), 0);
            close(sockfd[i]);
            sockfd[i] = -1;
        }
    }

    // Receive message from available server
    recv(sockfd[snum], list_msg, sizeof(list_msg), 0);

    filename = strtok(list_msg, "\n");
    while (filename) {
        if (construct_flag) {
            printf(" %s\n", filename);
        }
        else {
            printf(" [incomplete] %s\n", filename);
        }
        filename = strtok(NULL, "\n");
    }


    // Cleanup
    close(sockfd[snum]);

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
            if (verbose) printf(" ERROR: Invalid username/password\n");
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


    // BEGIN THE FILE TRANSFER
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
    int server_pair[2] = {0};
    int shutdown_pair[2] = {0};

    // Create socket descriptors for available servers and authenticate username/password
    for (int i = 0; i < 4; i++) {
        sockfd[i] = create_socket(i, config_data);
        if (sockfd[i] != -1 && handshake(sockfd[i], config_data) == 0) {
            if (verbose) printf(" ERROR: Invalid username/password\n");
            return;
        }
    }

    // Figure out which pair of servers to use
    if (sockfd[0] > 0 && sockfd[2] > 0) {
        server_pair[0] = 0; // Use servers: 1, 3
        server_pair[1] = 2;
        shutdown_pair[0] = 1;
        shutdown_pair[1] = 3;
    }
    else if (sockfd[1] > 0 && sockfd[3] > 0) {
        server_pair[0] = 1; // Use servers: 2, 4
        server_pair[1] = 3;
        shutdown_pair[0] = 0;
        shutdown_pair[1] = 2;
    }

    // Serialize message header
    snprintf(msgheader, sizeof(msgheader), header_format, filename);

    // Send message header to available servers and shutdown non-utilized servers
    for (int i = 0; i < 2; i++) {
        int sindex = server_pair[i];
        int bindex = shutdown_pair[i];
        send(sockfd[sindex], msgheader, sizeof(msgheader), 0);
        send(sockfd[bindex], "exit exit 0\n", sizeof(msgheader), 0);
        sockfd[bindex] = -1;
    }

    // Begin receiving files
    recv_files(sockfd, filename, server_pair);

    // Cleanup
    bzero(msgheader, sizeof(msgheader));
    for (int i = 0; i < 4; i++) {
        if (sockfd[i] > 0) {
            close(sockfd[i]);
        }
    }

}


void recv_files(int sockfd[], char *filename, int server_pair[])
{
    // Local Vars
    char getf_format[] = "%d\n";
    char getf_msg[MAX_MSG_LEN];
    char get_msg[MAX_MSG_LEN];
    char *file_buf[4];
    unsigned int file_size[4];
    int fc_loc[4] = {0};

    // If servers 1 AND 3 or 2 AND 4 are up, file cannot be reconstructed
    if (server_pair[0] == 0 && server_pair[1] == 0) {
        printf(" ERROR: Insufficient amount of servers available\n");
        return;
    }

    // Receive message from server pair with which chunks server has
    for (int i = 0; i < 2; i++) {
        int chunk1, chunk2;
        int serv_num = server_pair[i];

        // Recv message from server indicating which chunks it has
        recv(sockfd[serv_num], get_msg, sizeof(get_msg), 0);

        // Parse message and flag corresponding chunk in array
        chunk1 = atoi(strtok(get_msg, " "));
        chunk2 = atoi(strtok(NULL, "\n"));
        fc_loc[chunk1-1] = serv_num;
        fc_loc[chunk2-1] = serv_num;

        // If both chunks are 0, file not found
        if (chunk1 == 0 && chunk2 == 0) {
            printf("File not found: %s\n", filename);
            return;
        }
    }

    // Get each chunk from each server
    for (int i = 0; i < 2; i++) {
        int serv_num = server_pair[i];

        // Get the two chunks from serv_num
        for (int j = 0; j < 4; j++) {
            if (fc_loc[j] == serv_num) {
                unsigned int rebytes, offset;
                char sz_msg[MAX_MSG_LEN];
                char buffer[MAX_BUF_LEN];

                // Send request for file chunk
                snprintf(getf_msg, sizeof(getf_msg), getf_format, j+1);
                send(sockfd[serv_num], getf_msg, sizeof(getf_msg), 0);

                // Receive message from server and parse filesize
                recv(sockfd[serv_num], sz_msg, sizeof(sz_msg), 0);
                file_size[j] = atoi(strtok(sz_msg, "\n"));
                file_buf[j] = malloc(file_size[j]);

                // Receive file chunk
                rebytes = 0, offset = 0;
                while (offset < file_size[j]) {
                    rebytes = recv(sockfd[serv_num], buffer, sizeof(buffer), 0);
                    memcpy(file_buf[j] + offset, buffer, rebytes);
                    offset += rebytes;
                }

            }

        }

    }

    // Create file
    FILE *ofile = fopen(filename, "wb");

    // Now write each of the file chunks to a file with filename
    for (int i = 0; i < 4; i++) {
        fwrite(file_buf[i], 1, file_size[i], ofile);
    }

    // Cleanup
    fclose(ofile);
    for (int i = 0; i < 4; i++) {
        free(file_buf[i]);
    }

    printf(" Successfully wrote file: %s\n", filename);

}

// Send the relevant file chunks to each of the servers
void send_file(FILE *ifile, int sockfd[], unsigned int filesize)
{
    char pairs[4][MAX_MSG_LEN];
    char *filebuf = NULL;
    int shift;
    unsigned int rbytes = 0;

    // Hard code the file pairs to store
    strcpy(pairs[0], "1 2\n");
    strcpy(pairs[1], "2 3\n");
    strcpy(pairs[2], "3 4\n");
    strcpy(pairs[3], "4 1\n");

    // TODO: Use MD5sum % 4 to decide which chunks go on which servers
    shift = md5_mod4(ifile);
    printf("shift: %d\n", shift);

    // Send msg to server to indicate what chunks to keep
    for (int i = 0; i < 4; i++) {
        if (sockfd[i] > 0) {
            int pair_index = (i + shift) % 4;
            send(sockfd[i], pairs[pair_index], sizeof(pairs[pair_index]), 0);
        }
    }

    // Read the entire file
    filebuf = malloc(filesize);
    while(rbytes < filesize) {
        rbytes += fread(filebuf, 1, filesize, ifile);
    }

    // Send entire file to all servers
    for (int i = 0; i < 4; i++) {
        if (sockfd[i] > 0) {
            send(sockfd[i], filebuf, filesize, 0);
        }
    }

    // Done on client side
    printf(" Successfully wrote file to fileserver\n");
}


int md5_mod4(FILE *ifile)
{
    unsigned char data[1024];
    unsigned char c[MD5_DIGEST_LENGTH];
    unsigned int bytes = 0, md5_num;
    MD5_CTX mdContext;

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, ifile)) != 0) {
        MD5_Update (&mdContext, data, bytes);
    }
    printf("MADE IT HERE\n");
    MD5_Final (c,&mdContext);
    // for(int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", c[i]);
    md5_num = (unsigned int) c[MD5_DIGEST_LENGTH-1];
    printf("LAST BIT: %d\n", md5_num);

    // Cleanup
    rewind(ifile);

    return md5_num % 4;
}


// Send username and password to server
// return 0: Invalid credentials
// return 1: Valid credentials
int handshake(int server, struct ConfigData *config_data)
{
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
        if (verbose) printf(" Server %d is not responding...\n", server_num);
        return -1;
    }

    return sock;


}
void config_parse(struct ConfigData* config_data)
{
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
