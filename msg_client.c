#include "client.h"

int main(int argc, char** argv){
	int sock;
	struct sockaddr_in sv_addr;
	char* cmd=malloc(CMD_SIZE);
	char* arg_command=NULL;
	char prompt[50]= "> ";
	char* username=NULL;

	if(argc!=5){
		printf("Hai dimenticato qualche argomento, la sintassi è:\n");
		printf("./msg_client <ip del client> <porta locale> <ip server> <porta server>\n");
		return 0;
	}	

	//socket connessione server
	sock = socket(AF_INET, SOCK_STREAM, 0);

	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family=AF_INET;
	sv_addr.sin_port=htons(*argv[4]);
	//sv_addr.sin_addr.s_addr=INADDR_ANY;
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
		put_command(sock, cmd);

		if(!strcmp("!quit\0", cmd)){
			
			/*
			 * Quit command
			 */
			send_username(sock, username);
			free(cmd);
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
		}else if(!strcmp("!register\0", cmd)){
			/*
			 * Register command
			 */
		
			if(!arg_command){
				printf("Hai dimenticato a specificare l'username oppure contiene degli spazi.\n");
			}else if(register_user(arg_command, sock, argv[1], argv[2])!=1){
				memset(prompt,0,strlen(prompt));
				sprintf(prompt, "%s> ", arg_command);
				username = malloc(strlen(arg_command));
				sprintf(username, "%s", arg_command);
			}
		}else if(!strcmp("!deregister\0", cmd)){

			send_username(sock, username);
			free(username);
			username=NULL;
			memset(prompt,0,strlen(prompt));
			printf("Deregistrazione avvenuta con successo\n");
			sprintf(prompt,"> ");
		}
		memset(cmd, 0, CMD_SIZE);
		//memset(arg_command, 0, strlen(arg_command));
		free(arg_command);
		arg_command=NULL;
		//memset(arg_command, 0, BUFFER);

	}
	close(sock);
}


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

void put_command(int sock, char* buffer){

	uint16_t lenght=htons(strlen(buffer));

	if(send(sock, (void*)&lenght, sizeof(uint16_t),0) <0){
		perror("Errore nell'invio della lunghezza del comando");
		exit(1);
	}

	if(send(sock, (void*)buffer, strlen(buffer),0) <0){
		perror("Errore nell'invio del comando");
		exit(1);
	}
}

void who_command(int sock, char* mio_username){
	uint16_t lenght;
	char* username;
	unsigned int i_status;
	
	char** status=malloc(sizeof(char*));
	status[0]=malloc(strlen("Offline"));
	status[1]=malloc(strlen("Online"));
	strcpy(status[0], "Offline");
	strcpy(status[1], "Online");

	printf("Client registrati:\n");
	while(1){
		if(recv(sock, (void*)&lenght, sizeof(uint16_t), 0) <0){
			perror("Errore nel ricevere la lunghezza dell'username");
			exit(1);
		}
		if(!ntohs(lenght))
			break;
		username=malloc(ntohs(lenght));
		memset(username, 0, ntohs(lenght));
		if(recv(sock, (void*)username, ntohs(lenght), 0) <0){
			perror("Errore nel ricevere l'username");
			exit(1);
		}
		memset(&i_status, 0, sizeof(unsigned int));
		if(recv(sock, (void*)&i_status, sizeof(unsigned int),0) <0)
			perror("Errore nel ricevere lo status dell'utente");

		if(mio_username && !strcmp(mio_username, username)){
			free(username);
			continue;
		}
		printf("\t%s (%s)\t\n", username, status[ntohs(i_status)]);
		free(username);
	}
	free(status[0]);
	free(status[1]);
	free(status);
}

int register_user(char* arg_command, int sock, char* ip, char* port){
	uint16_t ptnet;
	int result;

	send_username(sock, arg_command);

	// Send ip and port
	if(send(sock,(void*)ip, 16, 0) < 0){
		perror("Errore nell'invio dell'IP");
		exit(1);
	}
	
	ptnet = htons(*port);
	if(send(sock, (void*)&ptnet, sizeof(uint16_t),0) <0){
		perror("Errore nell'invio della porta");
		exit(1);
	}

	if(recv(sock, (void*)&result, sizeof(int), 0) <0){
		perror("Errore nella ricezione dell'esito della registrazione");
		exit(1);
	}
	result=ntohs(result);
	switch(result){
		case 0:
			printf("Registrazione avvenuta con successo\n");
			break;
		case 1:
			printf("Registrazione fallita, username già presente\n");
			break;
		case 2:
			printf("Riconnessione avvenuta con successo\n");
	}
	return result;
}

void split_command(const char* command, char** arg_command){
	char* tmp;
	
	*(strchr(command, '\n'))='\0';

	//First occurence of ' ', replace with end of string terminator
	tmp = strchr(command, ' ');
	if(tmp)
		*tmp='\0';
	else{
		return;
	}

	//L'username non puo` contenere spazi
	if(!strlen(tmp+1) || strchr(tmp+1,' ')){
		return;
	}
	
	//Copy argument of command
	*arg_command=malloc(strlen(tmp+1)+1);
	sprintf(*arg_command, "%s", tmp+1);
	//memcpy((void*)*arg_command, (void*)(tmp+1), strlen(tmp+1)+1);
	//memset(tmp+1, 0, strlen(tmp+1));
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
