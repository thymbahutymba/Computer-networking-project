#include "condivisi.h"
#include "server.h"

void send_command(int, struct users*);

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
					}else if(!strcmp(buffer, "!quit\0")){
						
						quit_command(i, utenti);
						close(i);
						FD_CLR(i, &master);	
					}else if(!strcmp(buffer, "!deregister\0")){
						deregister_command(i, &utenti);
					}else if(!strcmp(buffer, "!send\0")){
						send_command(i, utenti);
					}
				}
			}
		}
	}
	close(listener);
}

void send_command(int sock, struct users* utenti){
	char *username, *sender;
	struct msg_offline *append=NULL, *search, *prec;
	struct users* to_send=NULL;
	unsigned int trovato=0, status;
	char msg[BUFFER_SIZE];
	char* buffer;
	uint16_t lenght, len, ptos;

	// Destinatario del messaggio
	username=receive_username(sock);
	sprintf(msg, "Destinatario del messaggio %s", username);
	logging(msg);

	// Ricerca utente
	for(;utenti;utenti=utenti->next_user){
		if(!strcmp(utenti->username, username)){
			to_send = utenti;
			trovato=1;
			break;
		}
	}

	// Utente non registrato
	if(!trovato){
		status=htons(0);
		if(send(sock, &status, sizeof(status), 0) <0){
			perror("Errore nell'invio dello status.");
		}
		sprintf(msg, "Tentativo di invio messaggio a username (%s) inesistente.", username);
		logging(msg);
		return;
	}

	// Utente non loggato
	if(to_send->my_info==NULL){
		status=htons(1);
		if(send(sock, &status, sizeof(status), 0) <0){
			perror("Errore nell'invio dello status.");
		}
		
		// Ricezione del mittente
		sender = receive_username(sock);

		// Salvo messaggio offline
		if(recv(sock, &lenght, sizeof(uint16_t), 0) <0){
			perror("Errore nel ricevere la lunghezza del messaggio");
		}

		buffer=malloc(ntohs(lenght));
		if(recv(sock, buffer, ntohs(lenght), 0) <0)
			perror("Errore nel ricevere il messaggio");

		prec=search=to_send->first_msg;
		for(;search; prec=search, search=search->next_msg)
			if(!strcmp(search->username, sender)){
				append=search;
				break;
		}

		if(append){
			//Sender già registrato
			append->msg[append->count++]=buffer;
			free(sender);
		}else{
			// Nuovo sender
			prec->next_msg=malloc(sizeof(struct msg_online*));
			prec->next_msg->username=sender;
			prec->next_msg->msg[prec->next_msg->count++]=buffer;
			prec->next_msg->next_msg=NULL;
		}
		sprintf(msg, "Ricezione messaggio instantaneo da %s per %s", sender, username);
		logging(msg);
		free(username);
	}else{
		len = sizeof(to_send->my_info->ip);
		lenght = htons(len);
		if(send(sock, &lenght, sizeof(uint16_t), 0) <0)
			perror("Errore invio lunghezza ip");
		if(send(sock, to_send->my_info->ip, len, 0 ) <0)
			perror("Errore nell'invio dell'IP");
		ptos = htons(to_send->my_info->port);
		if(send(sock, &ptos, sizeof(uint16_t), 0)<0)
			perror("Errore nell'invio della porta");
		
	}
}

void deregister_command(int sock, struct users** utenti){
	char* username=receive_username(sock);
	struct users* tmp=*utenti;
	struct users* prec=*utenti;
	char buffer[BUFFER_SIZE];

	for(;tmp;prec=tmp, tmp=tmp->next_user)
		if(!strcmp(username,tmp->username))
			break;

	if(tmp==*utenti)
		*utenti=tmp->next_user;
	else
		prec->next_user=tmp->next_user;

	free(tmp->username);
	free(tmp->my_info->ip);
	sprintf(buffer, "Utente %s deregistrato", username);
	free(username);

	logging(buffer);
}
/*
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
*/
void quit_command(int sock, struct users* utenti){
	char* username;
	char buffer[BUFFER_SIZE];
	
	username=receive_username(sock);
	if(username){
		for(;utenti;utenti=utenti->next_user){
			if(!strcmp(username,utenti->username))
				break;
		}
		free(utenti->my_info);
		utenti->my_info=NULL;
		utenti->first_msg=malloc(sizeof(struct msg_offline));
		sprintf(buffer, "Disconnessione di: %s", username);
		logging(buffer);
		return;
	}
	logging("Disconnessione di un client non registrato");
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
	uint16_t lenght, lentc;
	uint16_t finito=htons(0);
	unsigned int status;

	for(;utenti;utenti=utenti->next_user){
		lentc=strlen(utenti->username)+1;
		lenght = htons(lentc);
		if(send(sock, (void*)&lenght, sizeof(uint16_t),0) <0){
			perror("Errore nell'inviare la lunghezza dell'username");
			exit(1);
		}
		if(send(sock, (void*)utenti->username, lentc,0) <0){
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
	char* username;
	uint16_t port;
	char* ip;
	char msg[50];
	struct users* tmp=*utenti;
	struct users* prec=*utenti;

	memset(msg, 0, 50);

	username=receive_username(sock);

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
			memset(*new_username,0, strlen(tmp->username)+1);
			sprintf(*new_username, "%s", tmp->username);

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

	*new_username=malloc(strlen(tmp->username));
	memset(*new_username,0, strlen(tmp->username)+1);
	sprintf(*new_username, "%s", tmp->username);

	sprintf(msg, "Nuovo utente registrato, Username: %s", tmp->username);
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
