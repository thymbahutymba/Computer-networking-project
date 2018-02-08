#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define CMD_SIZE 50
#define BUFFER 1024

void welcome(const char*,const char*,const char*);
void stampacomandi();
void split_command(const char*, char**);
int register_user(char*, int, char*, char*);
void who_command(int, char*);
void put_command(int, char*);
void send_online(int, char*);
