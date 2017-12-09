#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>

char* receive_username(int);
void send_username(int, char*);
unsigned int receive_uint(int);
void send_uint(int, unsigned int);
void send_str(int, char*);
char* receive_str(int);
