/**	Client.h
 * Fichier header énumérant les méthodes creer, lire et modifier - méthodes de traitement - côté Client
 * 
 * Auteurs : HE Chenhui - MARECAR Farzana
 * 			 Etudiants en 1e année de Master SAR - Paris 6
 * Client : SENS Pierre - KORDON Fabrice
 **/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>




/// false=0; true>0


int creer(int sock, struct sockaddr_in server, char* nom, char* val);
int lire(int sock, struct sockaddr_in server, char* nom);
int modifier(int sock, struct sockaddr_in server, char* nom);


