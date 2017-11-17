#include "server.h"

int verbose = 1;

int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("Usage: ./server DFS<num> port\n");
        exit(-1);
    }

    // Local Vars
    int sockfd;
    unsigned int client_len;
    struct sockaddr_in client;
    struct ConfigData config_data;
    config_data.work_dir = argv[1];
    config_data.port = atoi(argv[2]);
    client_len = sizeof(struct sockaddr_in);

    // Read in configuration file to struct
    config_parse(&config_data);

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
            child_handler(clientfd, &config_data);
            exit(0);
        }

        // I am the parent
        if (pid > 0) {
            close(clientfd);

            waitpid(0, NULL, WNOHANG);
        }
    }

    return 0;
}


// Parse the dfs.conf file and store the data into a struct
void config_parse(struct ConfigData *config_data)
{
    // Local vars
    char conf_line[MAX_BUF_LEN];
    FILE *config_file;

    config_file = fopen("./dfs.conf", "r");
    if (!config_file) {
        perror("Could not find ./dfs.conf");
        exit(-1);
    }

    // Loop through file and store relevant information into struct
    int counter = 0;
    while (fgets(conf_line, MAX_BUF_LEN, config_file))
    {
        // Local Vars

        // Don't parse comments
        if (conf_line[0] != '#') {
            config_data->users[counter] = strdup(strtok(conf_line, " "));
            config_data->passwords[counter] = strdup(strtok(NULL, "\n"));
            counter++;
        }

    }
    fclose(config_file);

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


void child_handler(int client, struct ConfigData* config_data)
{
    // Local Vars
    char header[24];
    char *cmd, *filename, *username;
    int user_index, filesize;

    // Authenticate username and password from client
    user_index = validate_credentials(client, config_data);
    username = config_data->users[user_index];

    // Receive the message header which contains: <command> <filename> <filesize>
    recv_header(client, header, sizeof(header));

    // Parse command, filename and filesize from header
    cmd = strtok(header, " ");
    filename = strdup(strtok(NULL, " "));
    filesize = atoi(strtok(NULL, "\0"));


    printf("PID[%d] %s %s %s %d\n", getpid(), username, cmd, filename, filesize);
}


// Reliably receive first 24 bytes for header
void recv_header(int client, char *header, int header_size)
{
    // Local Vars
    unsigned int data_len;

    data_len = recv(client, header, 24, 0);

}


int validate_credentials(int client, struct ConfigData *config_data)
{
    // Local Vars
    int data_len;
    char userpass[24];
    char *username, *password;

    data_len = recv(client, userpass, sizeof(userpass), 0);
    if (data_len < 0) {
        perror("Recv: ");
        exit(-1);
    }

    // Parse username and password from userpass
    username = strtok(userpass, " ");
    password = strtok(NULL, "\n");

    // Check if username name exists AND matches password
    for (int i = 0; i < MAX_USERS; i++) {
        if ((strcmp(username, config_data->users[i]) == 0) && (strcmp(password, config_data->passwords[i]) == 0)) {
            if (verbose) printf("VERIFIED USER: %s\n", config_data->users[i]);
            write(client, "1", 1);
            return i;
        }
    }

    if (verbose) printf("INVALID USER: %s\n", username);

    exit(-1);

}
