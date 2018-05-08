/**	Serveur.h
 * 
 * Fichier Header avec les méthodes Serveurs
 * 
 * Auteur : MARECAR Farzana
 * Projet : Entrepôt Réparti en mémoire
 * Responsable : SENS Pierre
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>


#define NB_PAGES 10//00000// On peut créer jusqu'à pow(2,50) pages dans une server consacré à cette tâche mais nous allons tester dans le cadre du projet des serveurs offrants le stockage de 1000000 pages/données au plus


/**
 * Lors de l'allocation de pages pour le stockage de données, une structure comme celle-ci sera initialisée pour y contenir les informations
 **/
struct page{
	void* begin;			// Adresse de départ
	char* name;				// Si page vide/non utilisée, name = NULL
	size_t size;
	int nbR;				// Nombre de lecteur(s) en cours de lecture
	int readers[10];
	int flagW;				// Flag pour indiquer une écriture en cours : 0=non; 1=oui  --> Pouvait être un espèce de sémaphore
	int writer;
	struct page *next;		// Liste chaînée avec la page suivante indiquée
	pthread_mutex_t * mutex;
};


struct unusedPages{
	struct page *list;
	int nbUnused;
};
struct unusedPages* unused;


struct usedPages{
	struct page *list;
	int nbUsed;
};
struct usedPages* used;



int init_persistant();
struct page* getPage(char *name);
int p_create(char* name, int size, int sock);
int p_access_read(char *name, int sock);
int p_access_readwrite(char *name, int sock);
int p_release_read(char* name, int sock);
int p_release_readwrite(char* name, int sock);


