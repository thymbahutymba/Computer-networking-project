#include "condivisi.h"

void send_username(int sock, char* username){
	
	uint16_t lenght;
	uint16_t lentc=(username)?strlen(username)+1:0;

	lenght = htons(lentc);
	if(send(sock, (void*)&lenght, sizeof(uint16_t), 0) < 0){
		perror("Errore nell'invio della lunghezza dell'username");
		exit(1);
	}
	if(lentc){
		if(send(sock,(void*)username, lentc, 0) < 0){
			perror("Error nell'invio dell'username");
			exit(1);
		}
	}
}

char* receive_username(int sock){
	uint16_t lenght;
	char* username=NULL;

	if(recv(sock, (void*)&lenght, sizeof(uint16_t), 0) <0){
		perror("Errore nel ricevere la lunghezza dell'username");
	}

	if(ntohs(lenght)){
		username=malloc(ntohs(lenght));
		memset(username, 0, (ntohs(lenght)));
		if(recv(sock, (void*)username, ntohs(lenght), 0) <0){
			perror("Errore nel ricevere l'username");
		}
	}
	return username;
}

void send_uint(int sock, unsigned int valore){
	unsigned int status=htons(valore);
	
	if(send(sock, &status, sizeof(status), 0) <0){
		perror("Errore nell'invio dello status.");
	}
}

unsigned int receive_uint(int sock){
	unsigned int status;
	
	if(recv(sock, &status, sizeof(unsigned int), 0) <0)
		perror("Errore nella ricezione dello status");
	return ntohs(status);
}

void send_str(int sock, char* buffer){
	unsigned int ltos, lenght;

	ltos = strlen(buffer)+1;
	lenght=htons(ltos);

	if(send(sock, &lenght, sizeof(uint16_t), 0) <0)
		perror("Errore nell'inviare la lunghezza");
	if(send(sock, buffer, ltos, 0) <0)
		perror("Errore nell'invio del messaggio");
}

char* receive_str(int sock){
	unsigned int lenght;
	char* buffer;

	if(recv(sock, &lenght, sizeof(uint16_t), 0) <0)
		perror("Errore nel ricevere la lunghezza del messaggio");

	buffer=malloc(ntohs(lenght));
	memset(buffer, 0, ntohs(lenght));
	if(recv(sock, buffer, ntohs(lenght), 0) <0)
		perror("Errore nel ricevere il messaggio");
	return buffer;
}
