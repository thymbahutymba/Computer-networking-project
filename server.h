#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define BUFFER_SIZE 1024

struct users{
	char* username;
	struct info_sock* my_info;
	struct msg_offline* first_msg;
	struct users* next_user;
};

struct info_sock{
	char* ip;
	uint16_t port;
};

struct msg_offline{
	char* username;
	char** msg;
	int count;
	struct msg_offline* next_msg;
};

void logging(char*);
int register_username(int, struct users**, char**);
void get_command(int, char*);
void who_command(int, struct users*);
void quit_command(int, struct users*);
char* receive_username(int);
void deregister_command(int, struct users**);


