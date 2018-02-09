#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 1024

struct users{
	char* username;
	int TCP_sock;
	struct info_sock* my_info;
	struct msg_offline* first_msg;
	struct users* next_user;
};

struct info_sock{
	char* ip;
	char* port;
};

struct msg_offline{
	char* username;
	struct msg* msg;
	int count;
	struct msg_offline* next_msg;
};

struct msg{
	char* text;
	struct msg* next;
};

void logging(char*);
int register_username(int, struct users**, char**);
void get_command(int, char*, struct users*, fd_set*);
void who_command(int, struct users*);
void quit_command(int, struct users*);
void deregister_command(int, struct users**);
void send_offmessage(int, char*, struct users*);
void send_command(int, struct users*);

#endif
