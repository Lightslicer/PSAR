#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>       /* pour avoir AF_INET */
#include <netinet/in.h>

#include "Serveur.h"


const int nbT = 10;

void* routine(void *arg){
}

int main(){
	pthread_t tid[nbT];
/*	
	int i;
	for(i=0; i<nbT; i++){
		if(pthread_create(&tid[i], NULL, routine, NULL)!=0){
			printf("Echec pthread_create\n");
			exit(EXIT_FAILURE);
		}
	}
	
	int status;
	for(i=0; i<nbT; i++){
		pthread_join(tid[i], (void **)status);
		printf("Status ; %d\n", *status);
	}
	*/
	int sock = socket(AF_INET, SOCK_STREAM, 0);		
	struct sockaddr_in sin;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);   
	sin.sin_family = AF_INET;
	sin.sin_port = htons(2000);
	bind(sock, (struct sockaddr*)&sin, sizeof(sin));					// liaisonsocket-port
	listen(sock, 10);
		int sinsize = sizeof(sin);	
	while(1){
		accept(sock, (struct sockaddr*)&sin, &sinsize);
	}
	
	close(sock);	
	return 0;
}
