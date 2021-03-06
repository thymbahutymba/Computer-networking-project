#include "condivisi.h"
#include "client.h"

int main(int argc, char** argv){
	int sock;
	struct sockaddr_in sv_addr;
	char* cmd=malloc(CMD_SIZE);
	char* arg_command=NULL;
	char prompt[50]= "> ";
	char* username=NULL;
	pthread_t thread;
	struct thread_args t_args;

	if(argc!=5){
		printf("Hai dimenticato qualche argomento, la sintassi è:\n");
		printf("./msg_client <ip del client> <porta locale> <ip server> <porta server>\n");
		return 0;
	}

	//socket connessione server
	sock = socket(AF_INET, SOCK_STREAM, 0);

	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family=AF_INET;
	sv_addr.sin_port=htons(atoi(argv[4]));
	inet_pton(AF_INET, argv[3], &sv_addr.sin_addr);
	
	/*
	 * Connection to server
	 */
	if(connect(sock, (struct sockaddr*)&sv_addr, sizeof(sv_addr))<0){
		perror("Errore nella connessione");
		exit(1);
	}

	welcome(argv[3], argv[4], argv[2]);

	while(printf(prompt) && fgets(cmd, CMD_SIZE, stdin)){

		split_command(cmd, &arg_command);

		/*
		 * Controllo situazioni di errore/anomale
		 */

		if(!strcmp("!register\0", cmd) && !arg_command){
			printf("Hai dimenticato a specificare l'username oppure contiene degli spazi\n");
			continue;
		}else if(!strcmp("!register\0", cmd) && username!=NULL){
			printf("Sei già registrato\n");
			continue;
		}else if(!strcmp("!send\0", cmd) && !username){
			printf("Non sei ancora registrato, registrati tramite !register <username>\n");
			continue;
		}else if(!strcmp("!send\0", cmd) && username){
			if(arg_command==NULL || !strlen(arg_command)){
				printf("Nessun username specificato per l'invio del messaggio\n");
				continue;
			}else if(!strcmp(username, arg_command)){
				printf("Tentativo di invio messaggio a te stesso\n");
				continue;
			}
		}

		put_command(sock, cmd);

		if(!strcmp("!register\0", cmd)){
			
			/*
			 * Register command
			 */
			if(!register_user(arg_command, sock, argv[1], argv[2])){

				// Set prompt
				memset(prompt,0,strlen(prompt));
				sprintf(prompt, "%s> ", arg_command);

				// Set username
				username = malloc(strlen(arg_command));
				sprintf(username, "%s", arg_command);

				/*
			 	 * allocazione e inizializzazione struttura per passaggio dati al thread
			 	 */

				t_args.ip=malloc(strlen(argv[1]));
				t_args.port=malloc(strlen(argv[2]));
				t_args.username=malloc(strlen(username));

				sprintf(t_args.ip, "%s", argv[1]);
				sprintf(t_args.port, "%s", argv[2]);
				sprintf(t_args.username, "%s", username);
				pthread_create(&thread, NULL, receive_udp, (void*)&t_args);
			}
		}else if(!strcmp("!send\0", cmd)){
			
			/*
			 * Send command
			 */
			printf("\n");

			// Invio destinatario del messaggio
			send_username(sock, arg_command);
			switch (receive_uint(sock)){
				case 0:
					printf("Impossibile connettersi a %s: utente inesistente\n", arg_command);
					break;
				case 1:
					send_offline(sock, username);
					printf("Messaggio inviato con successo\n");
					break;
				case 2:
					send_online(sock, username);
					printf("Messaggio instantaneo inviato con successo\n");
					break;
			}
		}else if(!strcmp("!quit\0", cmd)){
			
			/*
			 * Quit command
			 */
			send_username(sock, username); 
			if(username)
				free(username);
			close(sock);
			printf("\nClient Disconnesso\n");
			exit(0);
		}else if(!strcmp("!help\0", cmd)){
			/*
			 * Help command
			 */
			stampacomandi();

		}else if(!strcmp("!who\0", cmd)){

			/*
			 * Who command
			 */
			who_command(sock, username);
		}else if(!strcmp("!deregister\0", cmd)){

			/*
			 * Deregister command
			 */
			send_username(sock, username);
			free(username);
			username=NULL;
			memset(prompt,0,strlen(prompt));

			// Deallocazione strutture dati inizializzate
			free(t_args.username);
			free(t_args.ip);
			free(t_args.port);
			
			// Cancellazione thread e chiusura socket per i messaggi instantanei
			pthread_cancel(thread);
			close(*t_args.UDP_sock);
			free(t_args.UDP_sock);

			printf("Deregistrazione avvenuta con successo\n");

			// update nuovo prompt
			sprintf(prompt,"> ");
		}

		memset(cmd, 0, CMD_SIZE);
		free(arg_command);
		arg_command=NULL;
	}
	return 0;
}

void send_online(int sock, char* username){
	char* ip;
	char* port;
	char buffer[BUFFER_SIZE], tmp[BUFFER_SIZE];
	struct sockaddr_in sv_addr;
	int sock_msg;

	// Ricezione indirizzo ip dal server
	ip = receive_str(sock);

	// Ricezione porta di ascolto del client destinatario dal server
	port = receive_str(sock);

	// Testo del messaggio
	memset(buffer,0,sizeof(buffer));
	while(fgets(tmp, BUFFER_SIZE, stdin)){
		if(*tmp=='.' && *(tmp+1)=='\n')
			break;
		strcat(buffer, tmp);
		memset(tmp, 0, sizeof(tmp));
	}

	/*
	 * Creazione socket per invio messaggi instantanei
	 */
	sock_msg=socket(AF_INET, SOCK_DGRAM, 0);

	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family=AF_INET;
	sv_addr.sin_port=htons(atoi(port));
	inet_pton(AF_INET, ip, &sv_addr.sin_addr);

	if(connect(sock_msg, (struct sockaddr*)&sv_addr, sizeof(sv_addr))<0)
		perror("Errore \"connessione\" server UDP");
	
	send_username(sock_msg, username);

	// Invio testo del messaggio
	send_str(sock_msg, buffer);

	close(sock_msg);
}

/*
 * Ricezione messaggi ricevuti durante periodo offline
 */
void receive_offmessage(int sock){
	char msg[BUFFER_SIZE];
	char* username;
	char* buffer;

	while(receive_uint(sock)){
		username=receive_username(sock);

		memset(msg, 0, sizeof(msg));
		sprintf(msg, "%s (msg offline)>", username);
		printf("%s\n", msg);

		while(receive_uint(sock)){
			memset(msg,0,sizeof(msg));
			buffer=receive_str(sock);
			printf("%s", buffer);
			free(buffer);
		}
		free(username);
	}
}

/*
 * Invio del messaggio a utente offline
 */
void send_offline(int sock, char* sender){
	char buffer[BUFFER_SIZE], tmp[BUFFER_SIZE];

	send_username(sock, sender);
	
	memset(buffer,0,sizeof(buffer));
	while(fgets(tmp, BUFFER_SIZE, stdin)){
		if(*tmp=='.' && *(tmp+1)=='\n')
			break;
		strcat(buffer, tmp);
		memset(tmp, 0, sizeof(tmp));
	}

	// Invio testo del messaggio
	send_str(sock, buffer);
}

/*
 * Funzione di esecuzione del thread
 */
void* receive_udp(void* args){
	struct sockaddr_in my_addr;
	int sock;
	char *buffer, *username=NULL;
	struct thread_args* t_args = (struct thread_args*)args;
	
	// Creazione socket per ricezione messaggi instantanei
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	t_args->UDP_sock=malloc(sizeof(int));
	*(t_args->UDP_sock)=sock;
	memset(&my_addr, 0, sizeof(my_addr));

	my_addr.sin_family=AF_INET;
	my_addr.sin_port=htons(atoi(t_args->port));
	inet_pton(AF_INET, t_args->ip, &my_addr.sin_addr);
	if(bind(sock, (struct sockaddr*)&my_addr, sizeof(my_addr)) <0)
		perror("Errore nel bindare il socket UDP");

	while(t_args->username!=NULL && (username=receive_username(sock))){
		buffer = receive_str(sock);
		printf("\n%s (msg instantaneo)>\n%s", username, buffer);
		free(username);
		free(buffer);
		printf("%s> ", t_args->username);
		fflush(stdout);
	}
	return NULL;
}

/*
 * Invio comando al server
 */
void put_command(int sock, char* buffer){
	
	uint16_t lenght=htons(strlen(buffer)+1);

	if(send(sock, (void*)&lenght, sizeof(uint16_t),0) < 0){
		perror("Errore nell'invio della lunghezza del comando");
		return;
	}

	if(send(sock, (void*)buffer, strlen(buffer)+1,0) < 0){
		perror("Errore nell'invio del comando");
		return;
	}
}

void who_command(int sock, char* mio_username){
	char* username;
	unsigned int i_status;
	
	char** status=malloc(sizeof(char*));
	status[0]=malloc(strlen("Offline"));
	status[1]=malloc(strlen("Online"));
	strcpy(status[0], "Offline");
	strcpy(status[1], "Online");

	printf("Client registrati:\n");
	while(receive_uint(sock)){
		username = receive_username(sock);
		memset(&i_status, 0, sizeof(unsigned int));
		i_status=receive_uint(sock);

		if(mio_username && !strcmp(mio_username, username)){
			free(username);
			continue;
		}
		printf("\t%s (%s)\t\n", username, status[i_status]);
		free(username);
	}
	free(status[0]);
	free(status[1]);
	free(status);
}

int register_user(char* arg_command, int sock, char* ip, char* port){
	unsigned int result;

	send_username(sock, arg_command);

	// Invio ip e port
	send_str(sock, ip);
	send_str(sock, port);
	
	result = receive_uint(sock);
	switch(result){
		case 0:
			printf("Registrazione avvenuta con successo\n");
			break;
		case 1:
			printf("Registrazione fallita, username già presente\n");
			return 1;
		case 2:
			printf("Riconnessione avvenuta con successo\n");
			receive_offmessage(sock);
	}
	return 0;
}

void split_command(const char* command, char** arg_command){
	char* tmp;
	
	*(strchr(command, '\n'))='\0';

	// Sostituzione primo spazio con terminatore stringa
	tmp = strchr(command, ' ');
	if(tmp)
		*tmp='\0';
	else{
		return;
	}

	/*
	 * Secondo parametro non valido se contiene spazi
	 */
	if(!(int)strlen(tmp+1) || strchr(tmp+1,' ')){
		return;
	}
	
	// Copia argomento comando (!register)
	*arg_command=malloc(strlen(tmp+1)+1);
	sprintf(*arg_command, "%s", tmp+1);
}

void stampacomandi(){
	FILE* comandi = fopen("./comandi.txt", "r");
	char str[80];
	while(fgets(str, 80, comandi)){
		printf("%s", str);
	}
	memset(&str,0,80);
	fclose(comandi);
	printf("\n");
	return;
}

void welcome(const char* server, const char* s_port, const char* i_port){
	printf("\n");
	printf("Connessione al server %s (porta %s) effettuata con successo\n", server, s_port);
	printf("Ricezione messaggi istantanei su porta %s\n\n", i_port);
	stampacomandi();
	return;
}
