#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <openssl/md5.h>

#define MAX_BUF_LEN 128
#define MAX_SERVERS 4
#define MAX_MSG_LEN 48

// Structs
struct ConfigData {
    char* name;
    char* password;
    char* serv_name[MAX_SERVERS];
    char* serv_addr[MAX_SERVERS];
    int serv_port[MAX_SERVERS];
};


// Function Declarations
static int getLine(char* prmpt, char* buff, size_t sz);
void config_parse(struct ConfigData* config_data);
void get_routine(char* inp_buffer, struct ConfigData* config_data);
int create_socket(int server_num, struct ConfigData *config_data);
int handshake(int server, struct ConfigData *config_data);
void put_routine(char* inp_buffer, struct ConfigData *config_data);
void send_file(FILE *ifile, int sockfd[], unsigned int filesize);
void recv_files(int sockfd[], char *filename, int server_pair[]);
void list_routine(struct ConfigData *config_data);
int md5_mod4(FILE *ifile);
