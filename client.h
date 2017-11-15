#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BUF_LEN 128
#define MAX_SERVERS 4


// Structs
struct ConfigData {
    char* name;
    char* password;
    char* serv_name[MAX_SERVERS];
    char* serv_addr[MAX_SERVERS];
};


// Function Declarations
static int getLine(char* prmpt, char* buff, size_t sz);
void config_parse(struct ConfigData* config_data);
