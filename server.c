#include "server.h"


int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: ./server DFS<num> port\n");
        exit(-1);
    }

    // Local Vars
    int sockfd;
    unsigned int client_len;
    struct sockaddr_in client;
    struct ConfigData config_data;
    config_data.port = atoi(argv[1]);
    client_len = sizeof(struct sockaddr_in);

    // Read in configuration file to struct

    // Bind the socket and listen on specified port
    sockfd = config_socket(config_data);

    // Primary loop for command line interface
    for (;;) {
        // Local Vars
        int clientfd;
        int pid;

        // Accept new connections and store file descriptor
        clientfd = accept(sockfd, (struct sockaddr *) &client, &client_len);
        if (clientfd < 0) {
            perror("Could not accept: ");
            exit(-1);
        }

        pid = fork();
        if (pid < 0) {
            perror("Could not fork: ");
            exit(-1);
        }

        // I am the child
        if (pid == 0) {
            close(sockfd);
            // Call child handler here
        }

        // I am the parent
        if (pid > 0) {
            close(clientfd);

            waitpid(0, NULL, WNOHANG);
        }
    }

    return 0;
}

// Bind to socket and listen on port
int config_socket(struct ConfigData config_data)
{
    // Local Vars
    int sockfd;
    int enable = 1;
    struct sockaddr_in remote;

    // Populate sockaddr_in struct with configuration for socket
    bzero(&remote, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(config_data.port);
    remote.sin_addr.s_addr = INADDR_ANY;

    // Create a socket of type TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Could not create socket: ");
        exit(-1);
    }

    // Set socket option for fast-rebind socket
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
        perror("Unable to set sock option: ");

    if ((bind(sockfd, (struct sockaddr *) &remote, sizeof(struct sockaddr_in))) < 0) {
        perror("Could not bind to socket: ");
        exit(-1);
    }

    // Allow up to 1 simultaneous connections on the socket
    if ((listen(sockfd, 1)) < 0) {
        perror("Could not listen: ");
        exit(-1);
    }

    return sockfd;
}

// Parse the ws.conf file and store the data into a struct
void config_parse(struct ConfigData *config_data)
{
    // Local vars
    char conf_line[MAX_FIELD_LEN];
    FILE *config_file;

    config_file = fopen("./dfs.conf", "r");
    if (!config_file) {
        perror("Could not find ./ws.conf");
        exit(-1);
    }

    // Loop through file and store relevant information into struct
    int counter = 0, fcounter = 0;
    while (fgets(conf_line, MAX_FIELD_LEN, config_file))
    {
        // Local Vars
        char *saveptr;
        char *firstptr;

        // Don't parse comments
        if (conf_line[0] != '#') {
            // if (counter == 0) {
            //     strtok_r(conf_line, " ", &saveptr);
            //     config_data->port = atoi(saveptr);
            // }
            // else if (counter == 1) {
            //     strtok_r(conf_line, " ", &saveptr);
            //     remove_elt(saveptr, "\"");
            //     remove_elt(saveptr, "\n");
            //     strcpy(config_data->root_dir, saveptr);
            // }
            // else if (counter == 2) {
            //     strtok_r(conf_line, " ", &saveptr);
            //     remove_elt(saveptr, "\n");
            //     strcpy(config_data->default_page, saveptr);
            // }
            // else {
            //     firstptr = strtok_r(conf_line, " ", &saveptr);
            //     strcpy(config_data->extensions[fcounter], firstptr);
            //     remove_elt(saveptr, "\n");
            //     strcpy(config_data->http_enc[fcounter], saveptr);
            //     fcounter++;
            // }
            counter++;
        }

    }
    fclose(config_file);

}
