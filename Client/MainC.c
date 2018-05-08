/**	MainC.c
 * Fichier sources contenant le main côté Client
 * Initialise la liste des IP Serveurs, et la connection au Serveur via sockets internets
 * Communique en boucle avec le Client pour lui demander la commande souhaitée jusqu'à précision de la fin des traitements
 * et redirige le fil d'exécution vers la méthode de traitement adéquat
 * 
 * Auteurs : HE Chenhui - MARECAR Farzana
 * 			 Etudiants en 1e année de Master SAR - Paris 6
 * Client : SENS Pierre - KORDON Fabrice
 **/
 
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


#include "ServerListC.h"
#include "Client.h"




int main(int argc , char *argv[]){
	init_list();
	
    int sock;
    struct sockaddr_in server;
    char message[1024];
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1){
        printf("Could not create socket");
    }
    puts("Socket created");
    
    char* ipS = SIP_alea();
    server.sin_addr.s_addr = inet_addr(ipS);
    server.sin_family = AF_INET;
    server.sin_port = htons( PORT_SERVICE );
 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("Connection failed - Error");
        return 1;
    }
     
    puts("Connected\n");
     
    //keep communicating with server
    while(1)
    {
        printf("Entrez une commande (creer, lire, modifier ou fin pour terminer les traitements) en précisant le nom de la donnée :\n");
        scanf("%s" , message);
        
        if(strcmp(message, "fin")==0){
			goto fin;
		}
        
        strtok(message, " ");
        
        if(strcmp(message, "creer")==0){
			char* nom = strtok(NULL, " ");
			printf("Veuillez entrer la valeur d'initialisation :\n");
			char val[4096];
			scanf("%s", val);
			creer(sock, server, nom, val);
			continue;
		}
		    
        else if(strcmp(message, "lire")==0){
			char* nom = strtok(NULL, " ");
			lire(sock, server, nom);
			continue;
		}       
		    
        else if(strcmp(message, "modifier")==0){
			char* nom = strtok(NULL, " ");
			modifier(sock, server, nom);
			continue;
		}       

		else{
			printf("Commande non reconnue\n");
			break;
		}
	}
    
  fin:
    close(sock);
    return 0;
}

