#include "client.h"

int main(int c, char* argv[])
{
    // Local Vars
    char* prmpt = "Fileserver> ";

    // TODO: PARSE THE CONFIG FILE FOR CONNECTIONS

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


static int getLine (char *prmpt, char *buff, size_t sz) {
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
