/**	StoS.h
 * 
 * Fichier Header avec les méthodes Serveurs liées à la synchro
 * 
 * Auteur : MARECAR Farzana
 * Projet : Entrepôt Réparti en mémoire
 * Responsable : SENS Pierre
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>



/// Fonctions pour la synchro
void heartbeat();

void prevent_init(int nbPages);
void prevent_create(char* name, char* val);
void prevent_access_read(char* name, int clnt);
void prevent_access_readwrite(char* name, int clnt);
void prevent_release_read(char* name, int clnt);
void prevent_release_readwrite(char* name, int clnt, char* buf);
