#include "condivisi.h"

void send_username(int sock, char* username){
	
	uint16_t lenght;
	uint16_t lentc=(username)?strlen(username)+1:0;

	lenght = htons(lentc);
	while(send(sock, (void*)&lenght, sizeof(uint16_t), 0) < 0)
		perror("Errore nell'invio della lunghezza dell'username");
	
	if(lentc)
		while(send(sock,(void*)username, lentc, 0) < 0)
			perror("Errore nell'invio dell'username");
}

char* receive_username(int sock){
	uint16_t lenght;
	char* username=NULL;

	while(recv(sock, (void*)&lenght, sizeof(uint16_t), 0) < 0)
		perror("Errore nel ricevere la lunghezza dell'username");

	if(ntohs(lenght)){
		username=malloc(ntohs(lenght));
		memset(username, 0, (ntohs(lenght)));
		while(recv(sock, (void*)username, ntohs(lenght), 0) < 0)
			perror("Errore nel ricevere l'username");
	}

	return username;
}

void send_uint(int sock, unsigned int valore){
	unsigned int uint=htons(valore);
	
	while(send(sock, &uint, sizeof(unsigned int), 0) < 0)
		perror("Errore nell'invio di un uint");
}

unsigned int receive_uint(int sock){
	unsigned int uint;
	
	while(recv(sock, &uint, sizeof(unsigned int), 0) < 0)
		perror("Errore nella ricezione di un uint");

	return ntohs(uint);
}

void send_str(int sock, char* buffer){
	unsigned int ltos, lenght;

	ltos = strlen(buffer)+1;
	lenght=htons(ltos);

	while(send(sock, &lenght, sizeof(uint16_t), 0) < 0)
		perror("Errore nell'inviare la lunghezza");

	while(send(sock, buffer, ltos, 0) < 0)
		perror("Errore nell'invio del messaggio");
}

char* receive_str(int sock){
	unsigned int lenght;
	char* buffer;

	while(recv(sock, &lenght, sizeof(uint16_t), 0) < 0)
		perror("Errore nel ricevere la lunghezza del messaggio");

	buffer=malloc(ntohs(lenght));
	memset(buffer, 0, ntohs(lenght));
	while(recv(sock, buffer, ntohs(lenght), 0) < 0)
		perror("Errore nel ricevere il messaggio");

	return buffer;
}
