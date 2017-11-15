#include "client.h"

int main(int c, char* argv[])
{
    // Local Vars
    char* prmpt = "Fileserver> ";
    struct ConfigData config_data;

    // TODO: PARSE THE CONFIG FILE FOR CONNECTIONS
    config_parse(&config_data);

    // Primary loop for command line interface
    for (;;) {
        // Local Vars
        char inp_buffer[MAX_BUF_LEN];
        char* cmd;
        char* filename;

        // Get command from standard input
        getLine (prmpt, inp_buffer, MAX_BUF_LEN);

        // Parse command and possibly filename from inp_buffer
        cmd = strtok(inp_buffer, " ");
        filename = strtok(NULL, "\n");

        // Check if command is one of commands and call applicable routine
        if (strcmp(cmd, "get") == 0) {
            printf("GET implementation missing\n");
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
                config_data->serv_name[server_ctr] = strtok(NULL, " ");
                config_data->serv_addr[server_ctr] = strtok(NULL, "\n");

                server_ctr++;
            }
            else if (strcmp(line_type, "Username:") == 0) {
                config_data->name = strtok(NULL, "\n");
            }
            else if (strcmp(line_type, "Password:") == 0) {
                config_data->password = strtok(NULL, "\n");
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
