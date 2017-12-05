#include "server.h"

void get_command(int, char*);
void who_command(int, struct users*);

int main(int argc, char** argv){

	int listener, new_sock;
	struct sockaddr_in my_addr;
	struct sockaddr_in cl_addr;
	int i, fdmax, result, s;
	unsigned int addrlen;
	char buffer[BUFFER_SIZE];
	fd_set master, master_cpy;	//set di socket per la listen
	struct users* utenti=NULL;	//Utenti registrati
	char* new_username=NULL;

	listener = socket(AF_INET, SOCK_STREAM, 0);
	close(listener);
	close(listener);
	close(listener);
	listener = socket(AF_INET, SOCK_STREAM, 0);

	FD_ZERO(&master);
	FD_ZERO(&master_cpy);

	memset(&my_addr, 0, sizeof(my_addr));

	my_addr.sin_family=AF_INET;
	my_addr.sin_port = htons(*argv[1]);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	//inet_pton(AF_INET, argv[2], &my_addr.sin_addr);
	if(bind(listener, (struct sockaddr*)&my_addr, sizeof(my_addr))<0){
		perror("Bind error");
		exit(1);
	}else{
		logging("Bind eseguita con successo");
	}

	if(listen(listener, 10)<0){
		perror("Listen error");
		exit(1);
	}

	FD_SET(listener, &master);
	fdmax = listener;
	
	for(;;){
		master_cpy = master;
		select(fdmax+1, &master_cpy, NULL,NULL,NULL);

		for(i=0; i<=fdmax; ++i){

			//se e` un descrittore pronto
			if(FD_ISSET(i,&master_cpy)){
				if(i==listener){

					//New connection request
					addrlen = (sizeof(cl_addr));
					new_sock = accept(listener, (struct sockaddr*)&cl_addr, &addrlen);
					FD_SET(new_sock, &master);
					fdmax = (new_sock>fdmax)?new_sock:fdmax;

					logging("Nuovo client connesso");

				}else{
					memset(buffer, 0, BUFFER_SIZE);
					get_command(i, buffer);

					if(!strcmp(buffer,"!register\0")){
						result = register_username(i, &utenti, &new_username);
						s = htons(result);
						if(send(i, (void*)&s, sizeof(int), 0)<0){
							perror("Errore in risposta allla richiesta di registrazione");
							exit(1);
						}
						if(result==2){
							//TODO Invio messaggi se riconnession
							//send_message(i, new_username);
						}

					}else if(!strcmp(buffer, "!who\0")){
						who_command(i, utenti);
					}
					//close(i);
					//FD_CLR(i, &master);
				}
			}
		}
	}
	close(listener);
}

void get_command(int sock, char* buffer){
	uint16_t lenght;
	memset(&lenght, 0, sizeof(uint16_t));
	//Get lenght of command
	if(recv(sock, (void*)&lenght, sizeof(uint16_t), 0)<0){
		perror("Errore nel ricevere la lunghezza del comando");
		exit(1);
	}

	//Get command
	if(recv(sock, (void*)buffer, ntohs(lenght),0) < 0){
		perror("Errore nel ricevere il comando");
		exit(1);
	}
	return;
}

void who_command(int sock, struct users* utenti){
	uint16_t lenght;
	uint16_t finito=htons(0);
	unsigned int status;
	for(;utenti;utenti=utenti->next_user){
		lenght = htons(strlen(utenti->username)+1);
		if(send(sock, (void*)&lenght, sizeof(uint16_t),0) <0){
			perror("Errore nell'inviare la lunghezza dell'username");
			exit(1);
		}
		if(send(sock, (void*)utenti->username, strlen(utenti->username)+1,0) <0){
			perror("Errore nell'inviare l'username");
			exit(1);
		}

		status=(utenti->my_info==NULL)?htons(0):htons(1);

		if(send(sock, (void*)&status, sizeof(unsigned int), 0) <0)
			perror("Errore nell'invio dello status");
	}
	if(send(sock, (void*)&finito, sizeof(uint16_t), 0) <0){
		perror("Errore nell'invio della fine !who");
		exit(1);
	}


}

int register_username(int sock, struct users** utenti, char** new_username){
	uint16_t lenght;
	char* username;
	uint16_t port;
	char* ip;
	char msg[50];
	struct users* tmp=*utenti;
	struct users* prec=*utenti;

	memset(msg, 0, 50);

	// Ricezione username
	memset(&lenght, 0, sizeof(lenght));
	if(recv(sock, (void*)&lenght, sizeof(uint16_t), 0) <0){
		perror("Errore nel ricevere la lunghezza dell'username");
		exit(1);
	}

	username=malloc(ntohs(lenght)+1);
	memset(username, 0, (ntohs(lenght))+1);
	if(recv(sock, (void*)username, ntohs(lenght), 0) <0){
		perror("Errore nel ricevere l'username");
		exit(1);
	}
	printf("%s", username);
	// Ricezione indirizzo locale client
	ip = malloc(16);
	if(recv(sock, (void*)ip, 16, 0) <0){
		perror("Errore nel ricevere l'IP");
		exit(1);
	}

	if(recv(sock, (void*)&port, sizeof(uint16_t), 0) <0){
		perror("Errore nel ricevere la porta");
	}
	port = ntohs(port);
	
	for(; tmp; prec=tmp, tmp=tmp->next_user){

		/*
		 * Riconnessione utente
		 */
		if(!strcmp(tmp->username,username)){
			if(tmp->my_info!=NULL){
				logging("Tentativo di registrazione con utente già presente");
				return 1;
			}

			sprintf(msg, "Riconnessione di %s", tmp->username);
			logging(msg);

			*new_username=malloc(strlen(tmp->username));
			memset(*new_username,0, strlen(tmp->username));
			memcpy(*new_username, tmp->username, strlen(tmp->username));

			tmp->my_info=malloc(sizeof(struct info_sock));
			memset(tmp->my_info, 0, sizeof(struct info_sock));
			
			tmp->my_info->ip = ip;
			tmp->my_info->port = port;
			return 2;
		}
	}

	/*
	 * Creazione nuovo utente
	 */

	if(*utenti==tmp){
		*utenti=malloc(sizeof(struct users));
		tmp=*utenti;
	}else{
		prec->next_user=malloc(sizeof(struct users));
		tmp=prec->next_user;
	}

	tmp->username=username;
	tmp->next_user=NULL;
	tmp->first_msg=NULL;
	
	tmp->my_info=malloc(sizeof(struct info_sock));
	tmp->my_info->ip = ip;
	tmp->my_info->port = port;

	sprintf(msg, "Nuovo utente registrato, Username: %s", tmp->username);
	//strcat(msg, "Nuovo utente registrato, Username: ");
	//strcat(msg, tmp->username);
	logging(msg);

	return 0;
}

void logging(char* msg){
	time_t current_time;
	char ti[10];
	struct tm* time_info;

	time(&current_time);
	time_info = localtime(&current_time);
	strftime(ti,11,"[%H:%M:%S]", time_info);

	printf("%s %s\n", ti, msg);
	return;
}
