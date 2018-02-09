#include "condivisi.h"
#include "server.h"

int main(int argc, char** argv){
	int listener, new_sock;
	struct sockaddr_in my_addr;
	struct sockaddr_in cl_addr;
	int i, fdmax, result;
	unsigned int addrlen;
	char buffer[BUFFER_SIZE];
	fd_set master, master_cpy;	//set di socket per la listen
	struct users* utenti=NULL;	//Utenti registrati
	char* new_username=NULL;

	listener = socket(AF_INET, SOCK_STREAM, 0);

	FD_ZERO(&master);
	FD_ZERO(&master_cpy);
	memset(&my_addr, 0, sizeof(my_addr));

	my_addr.sin_family=AF_INET;
	my_addr.sin_port = htons(atoi(argv[1]));
	my_addr.sin_addr.s_addr = INADDR_ANY;
	
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

					// Nuova richiesta di connessione
					addrlen = (sizeof(cl_addr));
					new_sock = accept(listener, (struct sockaddr*)&cl_addr, &addrlen);
					FD_SET(new_sock, &master);
					fdmax = (new_sock>fdmax)?new_sock:fdmax;

					logging("Nuovo client connesso");

				}else{
					memset(buffer, 0, BUFFER_SIZE);
					get_command(i, buffer, utenti, &master);

					if(!strcmp(buffer,"!register\0")){
						/*
						 * Register Command
						 */
						result = register_username(i, &utenti, &new_username);
						send_uint(i, result);

						if(result==2){
							send_offmessage(i, new_username, utenti);
						}

					}else if(!strcmp(buffer, "!who\0")){
						/* 
						 * Who command
						 */
						who_command(i, utenti);
					}else if(!strcmp(buffer, "!quit\0")){
						/*
						 * Quit Command
						 */
						quit_command(i, utenti);
						close(i);
						FD_CLR(i, &master);	
					}else if(!strcmp(buffer, "!deregister\0")){
						/*
						 * Deregister command
						 */
						deregister_command(i, &utenti);
					}else if(!strcmp(buffer, "!send\0")){
						/*
						 * Send Command
						 */
						send_command(i, utenti);
					}
				}
			}
		}
	}
	close(listener);
}

void send_offmessage(int sock, char* username, struct users* utenti){
	struct msg_offline *fromuser, *deleteuser;
	struct msg *message, *deletemsg;

	for(;utenti;utenti=utenti->next_user)
		if(!strcmp(utenti->username, username))
			break;
	
	fromuser=utenti->first_msg;
	utenti->first_msg=NULL;

	// Per ogni utente che ha inviato un messaggio
	while(fromuser){
		send_uint(sock, 1);
		send_username(sock, fromuser->username);
		message=fromuser->msg;
		while(message){
			// Invio testo del messaggio
			send_uint(sock,1);
			send_str(sock, message->text);

			// Cancello il messaggio
			deletemsg=message;
			message=message->next;

			// Free text and struct
			free(deletemsg->text);
			free(deletemsg);
		}
		// Segnale di fine messaggi per utente
		send_uint(sock, 0);
		
		// Cancello utente dalla lista
		deleteuser=fromuser;
		fromuser=fromuser->next_msg;

		free(deleteuser->username);
		free(deleteuser);
	}
	
	// Fine messaggi
	send_uint(sock, 0);
	
}

void send_command(int sock, struct users* utenti){
	char *username, *sender;
	struct msg_offline *append=NULL, *search, *prec;
	struct users* to_send=NULL;
	unsigned int trovato=0;
	char msg[BUFFER_SIZE];
	char* buffer;
	struct msg *new_msg=NULL;
	struct msg *tmp_msg=NULL;

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
		sprintf(msg, "Tentativo di invio messaggio a username (%s) inesistente.", username);
		logging(msg);
		free(username);
		send_uint(sock, 0);
		return;
	}

	// Utente non loggato
	if(to_send->my_info==NULL){

		send_uint(sock, 1);

		// Ricezione del mittente
		sender = receive_username(sock);

		// Ricezione messaggio offline
		buffer=receive_str(sock);

		// Ricerca utente fra i messaggi già ricevuti
		prec=search=to_send->first_msg;
		for(;search; prec=search, search=search->next_msg)
			if(!strcmp(search->username, sender)){
				append=search;
				break;
		}
		
		if(append){
			/*
			 * Sender già presente
			 */

			// Ricerca ultimo messaggio
			tmp_msg=append->msg;
			for(new_msg=tmp_msg; tmp_msg; new_msg=tmp_msg, tmp_msg=tmp_msg->next);

			if(tmp_msg==append->msg){
				// Nessun messaggio presente
				append->msg=malloc(sizeof(struct msg));
				tmp_msg=append->msg;
			}else{
				// Aggiungo in coda il messaggio
				new_msg->next=malloc(sizeof(struct msg));
				tmp_msg=new_msg->next;
			}
			tmp_msg->text=buffer;
			tmp_msg->next=NULL;
			free(sender);
		}else{
			/*
			 * Nuovo sender
			 */
			if(!to_send->first_msg){
				// Aggiunta in testa fra tutti i messaggi offline
				to_send->first_msg=malloc(sizeof(struct msg_offline));
				append=to_send->first_msg;
			}else{
				// Aggiunta in coda fra tutti i messaggi offline
				prec->next_msg=malloc(sizeof(struct msg_offline));
				append=prec->next_msg;
			}
			append->username=sender;
			append->next_msg=NULL;
			append->msg=malloc(sizeof(struct msg));
			append->msg->text=buffer;
			append->msg->next=NULL;

		}
		sprintf(msg, "Ricezione messaggio offline da %s per %s", sender, username);
		logging(msg);
	}else{
		send_uint(sock, 2);

		//Invio dell'indirizzo ip
		send_str(sock, to_send->my_info->ip);
		//Invio della porta
		send_str(sock,to_send->my_info->port);
		
		sprintf(msg, "Invio IP e Porta di %s per lo scambio di messaggi instantanei", username);
		logging(msg);
	}
	free(username);
}

void deregister_command(int sock, struct users** utenti){
	char* username=receive_username(sock);
	struct users* tmp=*utenti;
	struct users* prec=*utenti;
	char msg[BUFFER_SIZE];

	// Ricerca utente
	for(;tmp;prec=tmp, tmp=tmp->next_user)
		if(!strcmp(username,tmp->username))
			break;

	if(tmp==*utenti)
		*utenti=tmp->next_user;
	else
		prec->next_user=tmp->next_user;

	// Eliminazione utente
	free(tmp->username);
	free(tmp->my_info->ip);
	free(tmp->my_info->port);
	free(tmp->my_info);
	sprintf(msg, "Utente %s deregistrato", username);
	free(username);

	logging(msg);
}

void quit_command(int sock, struct users* utenti){
	char* username;
	char msg[BUFFER_SIZE];
	
	username=receive_username(sock);

	/*
	 * Se utente registrato
	 * Aggiornamento struttura dati relativa all'utente
	 */
	if(username){
		for(;utenti;utenti=utenti->next_user){
			if(!strcmp(username,utenti->username))
				break;
		}
		free(utenti->my_info->port);
		free(utenti->my_info->ip);
		free(utenti->my_info);
		utenti->my_info=NULL;
		utenti->first_msg=NULL; 
		sprintf(msg, "Disconnessione di: %s", username);
		logging(msg);
		free(username);
		return;
	}
	logging("Disconnessione di un client non registrato");
}


void get_command(int sock, char* buffer, struct users *utenti, fd_set* master){
	uint16_t lenght;
	int ret, registrato=0;
	char msg[50];
	memset(&lenght, 0, sizeof(uint16_t));

	ret = recv(sock, (void*)&lenght, sizeof(uint16_t), 0);
	
	if(ret<0){
		perror("Errore nel ricevere la lunghezza del comando");
		exit(1);
	}else if(!ret){
		/*
		 * Comportamento anomalo durante l'attesa di un comando
		 * Possibile chiusura inaspettata del client
		 * Ricerca e rimozione (disconnessione) dell'utente
		 */
		for(;utenti; utenti=utenti->next_user)
			if(utenti->TCP_sock==sock){
				registrato = 1;
				break;
			}
		if(registrato){
			free(utenti->my_info->ip);
			free(utenti->my_info->port);
			free(utenti->my_info);
			utenti->my_info=NULL;
			utenti->first_msg=NULL;
			sprintf(msg, "Possibile chiusura inaspettata per %s", utenti->username);
			logging(msg);
		}else{
			sprintf(msg, "Chiusura inaspettata di un client non registrato");
			logging(msg);
		}
		FD_CLR(sock, master);
		close(sock);
		return;
	}


	// Ricezione del comando
	if(recv(sock, (void*)buffer, ntohs(lenght),0) < 0){
		perror("Errore nel ricevere il comando");
		exit(1);
	}
	return;
}

void who_command(int sock, struct users* utenti){
	unsigned int status;
	logging("Richiesta lista utenti registrati");
	for(;utenti;utenti=utenti->next_user){
		// Segnale per invio username
		send_uint(sock, 1);
		send_username(sock, utenti->username);

		// Status dell'utente
		status=(utenti->my_info==NULL)?0:1;
		send_uint(sock, status);
	}
	// Segnale di notifica fine utenti
	send_uint(sock, 0);
}

int register_username(int sock, struct users** utenti, char** new_username){
	char* username;
	char* port;
	char* ip;
	char msg[50];
	struct users* tmp=*utenti;
	struct users* prec=*utenti;

	memset(msg, 0, 50);

	username=receive_username(sock);

	// Ricezione indirizzo locale client
	ip = receive_str(sock);
	port = receive_str(sock);
	
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

			tmp->TCP_sock = sock;

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
	tmp->TCP_sock = sock;
	
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
