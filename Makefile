CC=gcc
CFLAGS=-Wall
LDFLAGS=-lpthread
TERM=konsole

all: msg_client.o msg_server.o condivisi.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o msg_client msg_client.o condivisi.o
	$(CC) $(CFLAGS) -o msg_server msg_server.o condivisi.o

msg_client.o: msg_client.c client.h
	$(CC) $(CFLAGS) -c msg_client.c -o msg_client.o

msg_server.o: msg_server.c server.h
	$(CC) $(CFLAGS) -c msg_server.c -o msg_server.o

condivisi.o: condivisi.c condivisi.h
	$(CC) $(CFLAGS) -c condivisi.c -o condivisi.o

clean: 
	rm *.o msg_server msg_client

test:
	$(TERM) --fullscreen -e ./test.sh
	./stopall.sh
