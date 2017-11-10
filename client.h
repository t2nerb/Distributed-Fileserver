#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BUF_LEN 128
#define MAX_USERS 32


// Structs
struct ConfigData {
    char server_names[MAX_USERS][MAX_BUF_LEN];
    char server_addr[MAX_USERS][MAX_BUF_LEN];
};


// Function Declarations
static int getLine(char* prmpt, char* buff, size_t sz);
