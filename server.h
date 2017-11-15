#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#define MAX_BUF_LEN 128
#define MAX_USERS 32

struct ConfigData {
    int port;
    char* users[MAX_USERS];
    char* passwords[MAX_USERS];
};

int config_socket(struct ConfigData config_data);
void config_parse(struct ConfigData *config_data);
