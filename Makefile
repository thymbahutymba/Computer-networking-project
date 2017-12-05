CC=gcc
CFLAGS=-Wall

all: msg_client.o msg_server.o
	$(CC) $(CFLAGS) -o msg_client msg_client.o
	$(CC) $(CFLAGS) -o msg_server msg_server.o

msg_client.o: msg_client.c client.h
	$(CC) $(CFLAGS) -c msg_client.c -o msg_client.o

msg_server.o: msg_server.c server.h
	$(CC) $(CFLAGS) -c msg_server.c -o msg_server.o

clean: 
	rm *.o msg_server msg_client
