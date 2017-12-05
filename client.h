#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

void welcome(const char*,const char*,const char*);
void stampacomandi();
void split_command(const char*, char**);
