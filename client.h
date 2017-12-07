#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define CMD_SIZE 50
#define BUFFER 1024

void welcome(const char*,const char*,const char*);
void stampacomandi();
void split_command(const char*, char**);
int register_user(char*, int, char*, char*);
void who_command(int, char*);
void put_command(int, char*);
void send_username(int, char*);
