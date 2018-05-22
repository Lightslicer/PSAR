#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>    //strlen
#include <arpa/inet.h> //inet_addr
 
#include <fcntl.h>
#include <pthread.h>


#include "StoC.h"
#include "ServerListS.h"






///the thread function for Clients
void *connection_handler(void *);

///the thread function for other Servers
void *servers_handler(void *);





int main(int argc , char *argv[])
{
	pthread_t sync;
	init_list();

	if( pthread_create( &sync , NULL ,  servers_handler , NULL) < 0){
		perror("could not create thread sync");
        return 1;
	}
	
	init_persistant();

    int socket_desc , client_sock , c , *new_sock;
    struct sockaddr_in server , clnt;
     
    ///Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){
        printf("Could not create client socket");
    }
    puts("Client Socket created");
     
     
    ///Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = myIP;
    server.sin_port = htons( PORT_SERVICE );
     
     
    ///Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
     
    ///Listen
    listen(socket_desc , 10);
     
     
    ///Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    while( (client_sock = accept(socket_desc, (struct sockaddr *)&clnt, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;
         
        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
        {
            perror("could not create client thread");
            return 1;
        }
         
        ///Now join the thread , so that we dont terminate before the thread
        pthread_join( sniffer_thread , NULL);
        puts("Connexion Handler assigned");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    pthread_join( sync , NULL);
    puts("Handler assigned");
     
    return 0;
}
















/**
 * This will handle connection for each client
 **/
void *connection_handler(void *socket_desc)
{
    ///Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , client_message[4096];

     
    ///Receive a message from client
    while( (read_size = recv(sock , client_message , 4096 , 0)) > 0 ){
		strtok(client_message, " ");
		
		
/** Format de messages recus : create <nom> <val>
 * 	Format de messages envoyes : create <nom> OK <details> 
 * 						ou	   : create <nom> KO <details> 
 **/		
        if(strcmp(client_message, "create")==0){
			char* nom = strtok(NULL, " ");
			char *val = strtok(NULL, " ");
			char* res = p_create(nom, val, sock);
			write(sock, res, strlen(res));
			continue;
		}
		
/** 1) Récupération de valeur  : (p_access_read)
 *  Format de messages envoyes : access_read <nom>
 * 	Format de messages recus   : access_read <nom> OK <val> 
 * 						ou	   : access_read <nom> KO <details>
 *  2) Fin Lecture :
 *  Format de messages envoyes : release_read <nom>
 * 	Format de messages recus   : release_read <nom> OK <details> 
 * 						ou	   : release_read <nom> KO <details>
 **/
        else if(strcmp(client_message, "access_read")==0){
			char* nom = strtok(NULL, " ");
			char* res = p_access_read(nom, sock);
			write(sock, res, strlen(res));
			continue;
		}       
		    
        else if(strcmp(client_message, "release_read")==0){
			char* nom = strtok(NULL, " ");
			char* res = p_release_read(nom, sock);
			write(sock, res, strlen(res));
			continue;
		}
		
/** 1) Récupération de valeur :
 * Format de messages envoyes : access_readwrite <nom>	
 * Format de messages recus : access_readwrite <nom> OK <val> 
 * 						ou	 : access_readwrite <nom> KO <details> 
 * 2) Renvoie de modification :
 * 	Format de messages envoyes : release_readwrite <nom> <val>
 * 	Format de messages recus : release_readwrite <nom> OK <details> 
 * 						ou	 : release_readwrite <nom> KO <details>  
 **/      
        else if(strcmp(client_message, "access_readwrite")==0){
			char* nom = strtok(NULL, " ");
			char* res = p_access_readwrite(nom, sock);
			write(sock, res, strlen(res));
			continue;
		}       
		    
        else if(strcmp(client_message, "release_readwrite")==0){
			char* nom = strtok(NULL, " ");
			char *val = strtok(NULL, " ");
			char* res = p_release_readwrite(nom, sock, val);
			write(sock, res, strlen(res));
			continue;
		}       


		else{
			char *res = "Error : request not known";
			write(sock, res, strlen(res));
			continue;
		}       
    }
     
    if(read_size == 0){
        puts("Client disconnected");
        // libérer ces ressources
        fflush(stdout);
    }
    else if(read_size == -1){
        perror("recv failed");
    }
         
    ///Free the socket pointer
    free(socket_desc);
     
    return 0;
}


void *servers_handler(void *qqch){
	int socket_desc , client_sock , c , *new_sock;
    struct sockaddr_in server , clnt;
     
    ///Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){
        printf("Could not create sync socket");
    }
    puts("Sync Socket created");
     
     
    ///Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = myIP;
    server.sin_port = htons( PORT_SERVERS );
     
     
    ///Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
     
    ///Listen
    listen(socket_desc , N);
    pthread_t hb;
    pthread_create(&hb, NULL, heartbeat, (void*)socket_desc);
     
    ///Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    while( (client_sock = accept(socket_desc, (struct sockaddr *)&clnt, (socklen_t*)&c)) ){
		
		
	}
	
	pthread_join( hb , NULL);
	
	return NULL;
}
