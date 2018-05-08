/**	Serveur.c
 * 
 * Fichier source les méthodes Serveurs
 * 
 * Auteur : MARECAR Farzana
 * Projet : Entrepôt Réparti en mémoire
 * Responsable : SENS Pierre
 **/
#include <sys/mman.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "Serveur.h"




#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

static int psize;
///false = 0 ; true > 0
/// MAP_LOCKED Verrouille la page projetée en mémoire à la manière de mlock(2)



int init_persistant(){
	unused=(struct unusedPages*) malloc(sizeof(struct unusedPages*));
	unused->nbUnused=0;
	pthread_mutex_init(unused->mutex, NULL);
	
	used=(struct usedPages*) malloc(sizeof(struct usedPages*));
	used->nbUsed=0;
	pthread_mutex_init(used->mutex, NULL);
	
	psize = getpagesize();		/// En général 4096 octets
	printf("Page size = %d\n", psize);
	int *addr;
	
	/** On peut créer 4503599627370495 pages en mémoire (2⁵⁰) mais on va en créer que 1.000.000.000 dans le cadre de ce projet 
	 *  On boucle 1.000.000.000  fois et alloue des pages mémoire : meilleure solution selon moi car on fixe le nombre de pages partout sur tous les serveurs
	 **/
	
	pthread_mutex_lock(unused->mutex);
	
	while((unused->nbUnused < NB_PAGES) && (addr=malloc(psize))!=NULL){
		struct page* nouveau;
		nouveau=(struct page*) malloc(sizeof(struct page*));
		nouveau->begin = addr;
		pthread_mutex_init(nouveau->mutex, NULL);
		nouveau->next=unused->list;
		unused->list=nouveau;
		unused->nbUnused++;
		printf("Page n°%d:		@:%p\n", unused->nbUnused, nouveau->begin);		
	}
	pthread_mutex_unlock(unused->mutex);

	// appeler méthode réplication initiale sur serveur
	printf("\n%d page(s) créées\n", unused->nbUnused);
	return unused->nbUnused;
}



struct page* getPage(char *name){
	pthread_mutex_lock(used->mutex);
	struct page* tmp = used->list;
	while(tmp !=NULL){
		if(tmp->name == name){
			pthread_mutex_unlock(used->mutex);
			return tmp;
		}
		tmp=tmp->next;
	}
	free(tmp);
	pthread_mutex_unlock(used->mutex);
	return NULL;
}


int isReader(struct page* p, int sock){						/// Veiller à l'appeler dans un context où p->mutex est lock
	int i, tmp;
	printf("isReader: sock=%d", sock);
	for(i=0; i<NELEMS(p->readers); i++){
		if((tmp=p->readers[i])==sock){
			return 1;						/// true
		}
		printf("%d\n", tmp);
	}
	return 0;								/// false
}


int removeReader(struct page* p, int sock){
	int i,j;
	int nb=0;
	for(i=0; i<NELEMS(p->readers); i++ ){
         if(p->readers[i]==sock){
             nb++;
             for(j=i; j<NELEMS(p->readers); j++){
               p->readers[j]=p->readers[j+1]; 
             }
         }      
    }
    p->nbR-=nb;
	return nb;	
}



char* p_create(char* name, int size, int sock){		// A Répliquer sur les N-1 autres serveurs --> avoir un fichier de config contenant l'adresse de chaque serveur, se mettre d'accord sur un port de connexion
	if(getPage(name)!=NULL){
		return "p_create KO : Error - Page already exists";
	}
	pthread_mutex_lock(used->mutex);
	pthread_mutex_lock(unused->mutex);
	if(used->nbUsed==NB_PAGES || unused->nbUnused==0){
		/// Si déjà 1000000000 de pages allouées OU une page portant ce nom existe OU 0 page libre disponible --> false
		pthread_mutex_unlock(used->mutex);
		pthread_mutex_unlock(unused->mutex);
		return "p_create KO : Error Server - Server Memory full";
	}

	/// Retrait d'une page depuis la liste des pages non utilisees
	struct page *p = unused->list;
	unused->list = unused->list->next;
	pthread_mutex_unlock(unused->mutex);

	char* res;
	if(p!=NULL){
		/// Poser le verroue de la page
		pthread_mutex_lock(p->mutex);
		
		p->name=name;
		p->size=size;
		
		if(mmap(p->begin, psize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0)==MAP_FAILED){	// mmap fonctionnel que dans une hôte
			/// On le remet dans la liste des non utilisés
			p->name=NULL;
			p->size=0;
			p->next=unused->list;
			unused->list=p;
			pthread_mutex_unlock(p->mutex);
			return "p_create KO : Error Server - Memory Error";
		}
		memset(p->begin, '\0', p->size);
		p->next = used->list;
		used->list = p;
		used->nbUsed++;
		printf("New page used: Name=%s Size=%zu octets @=%p\n", p->name, p->size, p->begin);
		pthread_mutex_unlock(used->mutex);

		// Prevenir les autres		
		
		/// Deverouiller l'acces a la page
		pthread_mutex_unlock(p->mutex);
		res="p_create OK : Page created with success !";
	}
	else{
		res="p_create KO : Error Server - Could not create page";
	}
	return res;
}



char* p_access_read(char *name ,int sock){
	struct page* p = getPage(name);
	if(p==NULL){ 	/// La page n'existe pas
		return "p_access_read KO : Error - Page doesn't exist";
	}
		
	while(p->flagW==1){		/// Si quelqu'un lit
		sleep(2); // attente active, pas le plus optimal
	}
	
	/// Poser le verroue de la page
	pthread_mutex_lock(p->mutex);

	// prévenir les autres serveurs	

	char* dest=malloc(sizeof(p->size));
	memcpy(dest, p->begin, p->size);
	printf("p_access_read %s %p '%s' %d\n", p->name, p->begin, dest, p->nbR);
	p->readers[p->nbR]=sock;
	p->nbR++;
	/// Déverouiller l'accès à la page
	pthread_mutex_unlock(p->mutex);

	// prévenir les autres serveurs	
	
	return dest;//strcat("p_access_read OK : ", dest);
}



char* p_access_readwrite(char *name ,int sock){		// prévoir le cas où un client quitte le réseau sans libérer la donnée proprement
	struct page* p = getPage(name);
	if(p==NULL){ 		/// La page n'existe pas
		return "p_access_readwrite KO : Error - Page doesn't exist";
	}
	
	while(p->flagW==1 || p->nbR!=0){		/// Si une écriture en cours OU une lecture en cours
		sleep(2); // attente active, pas le plus optimal
	}

	/// Poser le verroue de la page
	pthread_mutex_lock(p->mutex);
	
	char* dest=malloc(sizeof(p->size));
	memcpy(dest, p->begin, p->size);
	printf("p_access_readwrite %s %p '%s'\n", p->name, p->begin, dest);
	p->flagW=1;
	p->writer=sock;
	
	// Prévenir les autres		

	/// Déverouiller l'accès à la page
	pthread_mutex_unlock(p->mutex);
	return dest;//strcat("p_access_readwrite OK : ", dest);
}




char* p_release_read(char *name ,int sock){
	struct page* p = getPage(name);
	if((p==NULL)){ 		/// La page n'existe pas
		return "p_release_read KO : Error - Page doesn't exist";
	}

	if(p->nbR>0 && isReader(p, sock)==1){
		/// Poser le verroue de la page
		pthread_mutex_lock(p->mutex);
		
		if(mprotect(p->begin, psize, PROT_NONE)==-1){
			pthread_mutex_unlock(p->mutex);
			return 0;
		}
		p->nbR--;
		
		// prévenir les autres

		removeReader(p, sock);
		/// Déverouiller l'accès à la page
		pthread_mutex_unlock(p->mutex);
		return "p_release_read OK";
	}
	else{
		return "p_release_read KO : Error - You're not a reader of this page";
	}
}




char* p_release_readwrite(char* name, int sock, char* buf, int size){
	struct page *p = getPage(name);
	if(p==NULL){ 		/// La page n'existe pas
		return "p_release_readwrite KO : Error - Page doesn't exist";
	}
	if(p->flagW==1 && p->writer==sock){
		/// Poser le verroue de la page
		pthread_mutex_lock(p->mutex);
		
		char* buf=malloc(psize);
		read(sock, buf, psize);
		
		if(memcpy(p->begin, buf, psize)==NULL||mprotect(p->begin, psize, PROT_NONE)==-1){
			pthread_mutex_unlock(p->mutex);
			return "p_release_readwrite KO : Error Server - Memory error";
		}
		p->flagW=0;
		// prévenir les autres x2

		removeReader(p, sock);		
		/// Déverouiller l'accès à la page
		pthread_mutex_unlock(p->mutex);
		return "p_release_readwrite OK";
	}
	else{
		return "p_release_readwrite KO : Error - You're not a writer of this page";
	}
}




void afficherUseds(){
	struct page* tmp;
	if((tmp = used->list)!=NULL){
		printf("\nList of Used Pages\n");
		while(tmp!=NULL){
			printf("Used Page name: %s\n", tmp->name);
			tmp = tmp->next;
		}
		printf("\n");
	}
	else{
		printf("List of Used Pages empty !\n");
	}
}


void afficherUnuseds(){
	struct page* tmp;
	if((tmp = unused->list)!=NULL){
		printf("\nList of Unused Pages\n");
		while(tmp!=NULL){
			printf("Unused Page address: %p\n", tmp->begin);
			tmp = tmp->next;
		}
		printf("\n");
	}
	else{
		printf("\nList of Unused Pages empty !\n");
	}
}


void cleanMem(){
	struct page* tmp;
	while((unused->list)!=NULL){
		tmp = unused->list->next;
		munmap(unused->list->begin, psize);
		free(unused->list);
		unused->list = tmp;
	}
	while((used->list)!=NULL){
		tmp = used->list->next;
		munmap(used->list->begin, psize);
		free(used->list);
		used->list = tmp;
	}
	free(tmp);
}








int main(){
	int sock = socket(AF_INET, SOCK_STREAM, 0);		
	if(init_persistant()!=NB_PAGES){
		printf("Initialisation mémoire erronée\n");
		return 0;
	}
	
	char* res;
	
	res = p_create("a", 10, sock);
	printf("%s\n", res);
	afficherUseds();
	res = p_access_read("a", sock);
	printf("%s\n", res);
	res = p_release_read("a", sock);
	printf("%s\n", res);
	/*	
	res = p_access_readwrite("a", sock);
	printf("%s\n", res);
*/
	/*struct page* p = getPage("a");
	char* dest=malloc(sizeof(psize));
	memcpy(dest, p->begin, psize);
	printf("%p '%s'\n", p->begin, dest);*/
	cleanMem();
/*
	char* res;
	res = p_create("a", 10, sock);
	printf("%s\n", res);
	afficherUseds();
	res = p_create("b", 100, sock);
	printf("%s\n", res);
	afficherUseds();

	struct page* p = getPage("a");
	char* dest=malloc(sizeof(psize));
	memcpy(dest, p->begin, psize);
	printf("%s\n", dest);

	res = p_access_read("a", sock);
	printf("%s\n", res);
*//*	printf("\n");
	res = p_release_read("a", sock);
	printf("%s\n", res);
	printf("\n");
	*/
	
		
	/*afficherUseds();
	afficherUnuseds();*/
	close(sock);
	
	return 1;
}
