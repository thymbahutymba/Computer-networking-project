#include "condivisi.h"

void send_username(int sock, char* username){
	
	uint16_t lenght;
	uint16_t lentc=strlen(username)+1;

	lenght = (username)?htons(lentc):htons(0);
	if(send(sock, (void*)&lenght, sizeof(uint16_t), 0) < 0){
		perror("Errore nell'invio della lunghezza dell'username");
		exit(1);
	}
	if(ntohs(lenght)){
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


