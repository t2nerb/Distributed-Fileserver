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
    char header[MAX_MSG_LEN];
    char *cmd, *filename, *username;
    unsigned int user_index, filesize;

    // Authenticate username and password from client
    user_index = validate_credentials(client, config_data);
    username = config_data->users[user_index];

    // Setup working directory for connected user
    setup_directory(username, config_data);

    // Receive the message header which contains: <command> <option1> <option2>
    recv_header(client, header, sizeof(header));

    // Parse command, filename and filesize from header
    cmd = strtok(header, " ");
    filename = strdup(strtok(NULL, " "));
    filesize = atoi(strtok(NULL, "\n"));

    // Call relevant cmd procedure
    if (strcmp(cmd, "put") == 0) {
        printf("%s[%d]: %s %s %d\n", username, getpid(), cmd, filename, filesize);
        put_routine(client, filename, filesize);
    }
    else if (strcmp(cmd, "get") == 0) {
        printf("%s[%d]: %s %s\n", username, getpid(), cmd, filename);
    }
    else if (strcmp(cmd, "list") == 0) {
        printf("Command is list!\n");
    }


}

void put_routine(int client, char *filename, unsigned int filesize)
{
    // Local Vars
    char fn_format[] = "%s.%d";   // Format: .filename.txt.1
    char put_msg[MAX_MSG_LEN], buffer[MAX_BUF_LEN], fname[2][MAX_MSG_LEN];
    char *filebuf;
    unsigned int rebytes = 0, offset = 0, chunk_offset = 0;
    unsigned int chunk_len, lchunk_len;
    unsigned int chunks[2];
    FILE *ofile;

    // Receive message to indicate what file chunks to keep
    rebytes = recv(client, put_msg, sizeof(put_msg), 0);
    chunks[0] = atoi(strtok(put_msg, " "));
    chunks[1] = atoi(strtok(NULL, "\n"));

    // Receive entire file
    filebuf = malloc(filesize);
    rebytes = 0, offset = 0;
    while (offset < filesize) {
        rebytes = recv(client, buffer, sizeof(buffer), 0);
        memcpy(filebuf + offset, buffer, rebytes);
        offset += rebytes;
    }

    // Compute size for each 'quarter' of the file
    chunk_len = (filesize + 4 - 1) / 4;
    lchunk_len = filesize % 4;
    if (lchunk_len == 0) lchunk_len = chunk_len;

    // Write the relevant file chunks
    for (int i = 0; i < 2; i++) {
        // Calculate offset where chunk is
        chunk_offset = (chunks[i] - 1) * chunk_len;
        snprintf(fname[i], sizeof(fname[i]), fn_format, filename, chunks[i]);
        ofile = fopen(fname[i], "wb");

        if (chunks[i] < 4) {
            fwrite(filebuf+chunk_offset, 1, chunk_len, ofile);
        }
        else {
            fwrite(filebuf+chunk_offset, 1, lchunk_len, ofile);
        }
        fclose(ofile);
    }

    // Cleanup
    free(filebuf);
}


// Reliably receive first 24 bytes for header
void recv_header(int client, char *header, int header_size)
{
    // Local Vars
    unsigned int data_len;

    data_len = recv(client, header, MAX_MSG_LEN, 0);

    if (data_len < MAX_MSG_LEN) printf("DANGER, NOT ALL BYTES WERE READ FROM HEADER\n");

}


int validate_credentials(int client, struct ConfigData *config_data)
{
    // Local Vars
    int data_len;
    char userpass[MAX_MSG_LEN];
    char *username, *password;

    // Receive the credentials
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

void setup_directory(char *user, struct ConfigData *config_data)
{
    // Local Vars
    struct stat st = {0};
    char new_dir[MAX_MSG_LEN];
    char wd_format[] = "./%s/%s/";
    char cwd[1024];

    // Construct the new working directory
    snprintf(new_dir, sizeof(new_dir), wd_format, config_data->work_dir, user);

    // If directory doesn't exist, make the directory
    if (stat(new_dir, &st) == -1) {
        printf("CREATED NEW DIRECTORY: %s\n", user);
        mkdir(new_dir, 0700);
    }

    // Change working directory
    if (chdir(new_dir) == -1) {
        printf("SETUP_DIRECTORY: %s\n", strerror(errno));
    }

    // Print current working directory
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        if (verbose) printf("CURRENT WORKING DIRECTORY: %s\n", cwd);
    }

}
