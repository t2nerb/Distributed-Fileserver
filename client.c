#include "client.h"

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
        char* cmd;
        char* filename;

        // Get command from standard input
        getLine(prmpt, inp_buffer, MAX_BUF_LEN);

        // Parse command and possibly filename from inp_buffer
        cmd = strtok(inp_buffer, " ");
        filename = strtok(NULL, "\n");

        // Check if command is one of commands and call applicable routine
        if (strcmp(cmd, "get") == 0) {
            get_routine(inp_buffer, &config_data);
        }
        else if (strcmp(cmd, "put") == 0) {
            printf("PUT implementation missing\n");
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

    return 0;
}

void get_routine(char* inp_buffer, struct ConfigData* config_data)
{
    // Local Vars
    int sockfd;
    int conn_status;

    // Get socket file descriptor
    sockfd = create_socket(0, config_data);

    // TODO: Authenticate username and password with server
    conn_status = handshake(sockfd, config_data);

}

// Send username and password to server
// If handshake returns -1, then invalid credentials
int handshake(int server, struct ConfigData *config_data)
{
    // Local Vars
    // int data_len;
    char userpass[24];

    // Create username and password pair from config_data
    strcpy(userpass, config_data->name);
    strcat(userpass, " ");
    strcat(userpass, config_data->password);
    strcat(userpass, "\n");

    // Send the user password pair
    send(server, userpass, sizeof(userpass), 0);

    // TODO: Wait 1 second for response from server
    // If no response, return with error


    return 0;
}

int create_socket(int server_num, struct ConfigData *config_data)
{
    // Local Vars
    struct sockaddr_in server;
    int port_number = config_data->serv_port[server_num];

    // Populate server struct
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(port_number);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Socket could not be created\n");
        return -1;
    }

    // Try to connect
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Connection could not be made\n");
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
    int server_ctr = 0;
    while (fgets(conf_line, MAX_BUF_LEN, config_file))
    {
        // Local Vars
        char* line_type;

        // Don't parse comments
        if (conf_line[0] != '#' && conf_line[0] != ' ') {
            line_type = strtok(conf_line, " ");

            if (strcmp(line_type, "Server") == 0) {
                config_data->serv_name[server_ctr] = strdup(strtok(NULL, " "));
                config_data->serv_addr[server_ctr] = strdup(strtok(NULL, ":"));
                config_data->serv_port[server_ctr] = atoi(strdup(strtok(NULL, "\n")));
                server_ctr++;
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

    buff[strlen(buff)-1] = '\0';
    return 0;
}
