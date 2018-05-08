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

#include "Serveur.h"



#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

static int psize;
///false = 0 ; true > 0
/// MAP_LOCKED Verrouille la page projetée en mémoire à la manière de mlock(2)



int init_persistant(){
	unused=(struct unusedPages*) malloc(sizeof(struct unusedPages*));
	unused->nbUnused=0;
	used=(struct usedPages*) malloc(sizeof(struct usedPages*));
	used->nbUsed=0;
	
	psize = getpagesize();		/// En général 4096 octets
	/**printf("Page size = %d\n", psize);**/
	int *addr;
	
	/** On peut créer 4503599627370495 pages en mémoire (2⁵⁰) mais on va en créer que 1.000.000.000 dans le cadre de ce projet 
	 *  On boucle 1.000.000.000  fois et alloue des pages mémoire : meilleure solution selon moi car on fixe le nombre de pages partout sur tous les serveurs
	 **/
	
	while((unused->nbUnused < NB_PAGES) && (addr=malloc(psize))!=NULL){
		struct page* nouveau;
		nouveau=(struct page*) malloc(sizeof(struct page*));
		nouveau->begin = addr;
		//pthread_mutex_init(nouveau->mutex, NULL);
		nouveau->next=unused->list;
		unused->list=nouveau;
		unused->nbUnused++;
		printf("Page n°%d:		@:%p\n", unused->nbUnused, nouveau->begin);
		
	}

	// appeler méthode réplication initiale sur serveur
	printf("%d page(s) créées\n", unused->nbUnused);
	return unused->nbUnused;
}



struct page* getPage(char *name){
	struct page* tmp = used->list;
	while(tmp !=NULL){
		if(tmp->name == name){
			return tmp;
		}
		tmp=tmp->next;
	}
	free(tmp);
	printf("getPage %s: impossible de trouver la page\n", name);
	return NULL;
}



int p_create(char* name, int size, int sock){		// A Répliquer sur les N-1 autres serveurs --> avoir un fichier de config contenant l'adresse de chaque serveur, se mettre d'accord sur un port de connexion

	if(used->nbUsed==NB_PAGES || getPage(name)!=NULL || unused->nbUnused==0){
		/// Si déjà 1000000000 de pages allouées OU une page portant ce nom existe OU 0 page libre disponible --> false
		printf("Impossible de créer une page: unused vide OU used rempli OU déjà une page de ce nom\n"); 
		return 0;
	}

	/// Retrait d'une page depuis la liste des pages non utilisées
	struct page *p = unused->list;
	unused->list = unused->list->next;

	if(p!=NULL){
		/// Poser le verroue de la page
		pthread_mutex_lock(p->mutex);
		
		p->name=name;
		p->size=size;
		if(mmap(p->begin, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)==(void *) -1){
			/// On le remet dans la liste des non utilisés
			p->name=NULL;		/// On définit une page non utilisée comme n'ayant pas de nom
			p->size=0;
			p->next=unused->list;
			unused->list=p;
			printf("P_create failed !\n");
			perror("Fail mmap");
			pthread_mutex_unlock(p->mutex);
			return 0;
		}
		p->next = used->list;
		used->list = p;
		used->nbUsed++;
		printf("New page used: Name=%s Size=%zu octets\n", p->name, p->size);

		// Prévenir les autres		
		
		/// Déverouiller l'accès à la page
		pthread_mutex_unlock(p->mutex);
	}
	free(p);
	return 1;
}



int p_access_read(char *name ,int sock){
	struct page* p = getPage(name);
	if(p==NULL){ 	/// La page n'existe pas
		printf("p_access_read échouée\n");
		return 0;
	}
		
	while(p->flagW==1){		/// Si quelqu'un lit
		sleep(2); // attente active, pas le plus optimal
	}
	
	/// Poser le verroue de la page
	pthread_mutex_lock(p->mutex);
	
	// prévenir les autres serveurs	

	printf("p_access_read: Lecture de page %s\n", p->name);
	
	if(mmap(p->begin, psize, PROT_READ, MAP_SHARED, sock, 0)==MAP_FAILED){
		pthread_mutex_unlock(p->mutex);
		printf("MAP_FAILED\n");
		return 1;
	}
	p->readers[p->nbR]=sock;
	p->nbR++;
	printf("p_access_read: sock=%d, reader n°%d=%d\n", sock, p->nbR, p->readers[p->nbR-1]);
	
	/// Déverouiller l'accès à la page
	pthread_mutex_unlock(p->mutex);
	return 1;
}



int p_access_readwrite(char *name ,int sock){		// prévoir le cas où un client quitte le réseau sans libérer la donnée proprement
	struct page* p = getPage(name);
	if(p==NULL){ 		/// La page n'existe pas
		return 0;
	}
	
	while(p->flagW==1 || p->nbR!=0){		/// Si une écriture en cours OU une lecture en cours
		sleep(2); // attente active, pas le plus optimal
	}

	/// Poser le verroue de la page
	pthread_mutex_lock(p->mutex);
		
	if(mmap(p->begin, psize, PROT_READ|PROT_WRITE, MAP_SHARED, sock, 0)==MAP_FAILED){
		pthread_mutex_unlock(p->mutex);
		return 0;
	}
	p->flagW=1;
	// Prévenir les autres		

	/// Déverouiller l'accès à la page
	pthread_mutex_unlock(p->mutex);
	return 1;
}




int p_release_read(char *name ,int sock){
	struct page* p = getPage(name);
	if(p==NULL){ 		/// La page n'existe pas
		printf("p_release_read %s echouée\n", p->name);
		return 0;
	}
	if(p->nbR>0){
		/// Poser le verroue de la page
		pthread_mutex_lock(p->mutex);
		
		if(mprotect(p->begin, psize, PROT_NONE)==-1){
			pthread_mutex_unlock(p->mutex);
			return 0;
		}
		p->nbR--;
		
		// prévenir les autres

		printf("p_release_read %s effectuée\n", p->name);
		
		/// Déverouiller l'accès à la page
		pthread_mutex_unlock(p->mutex);
		return 1;
	}
	else{
		printf("p_release_read %s echouée: pas de lecture sur cette page\n", p->name);
	}
	return 1;
}




int p_release_readwrite(char* name, int sock){
	struct page *p = getPage(name);
	if(p==NULL){ 		/// La page n'existe pas
		return 0;
	}
	if(p->flagW==1){
		/// Poser le verroue de la page
		pthread_mutex_lock(p->mutex);
		
		char* buf=malloc(psize);
		read(sock, buf, psize);
		
		if(memcpy(p->begin, buf, psize)==NULL||mprotect(p->begin, psize, PROT_NONE)==-1){
			pthread_mutex_unlock(p->mutex);
			return 0;
		}		
		p->flagW=0;
		// prévenir les autres x2
		
		/// Déverouiller l'accès à la page
		pthread_mutex_unlock(p->mutex);
	}
	
	return 1;
}




void afficherUseds(){
	struct page* tmp;
	if((tmp = used->list)!=NULL){
		printf("List of Used Pages\n");
		while(tmp!=NULL){
			printf("Used Page name: %s\n", tmp->name);
			tmp = tmp->next;
		}
	}
	else{
		printf("List of Used Pages empty !\n");
	}
}


void afficherUnuseds(){
	struct page* tmp;
	if((tmp = unused->list)!=NULL){
		printf("List of Unused Pages\n");
		while(tmp!=NULL){
			printf("Unused Page address: %p\n", tmp->begin);
			tmp = tmp->next;
		}
	}
	else{
		printf("List of Unused Pages empty !\n");
	}
}


void cleanMem(){
	struct page* tmp;
	struct page* first;
	if((first = used->list)!=NULL){
		while(first!=NULL){
			tmp = first->next;
			free(first);
			first = tmp;
		}
	}
	if((first = unused->list)!=NULL){
		while(first!=NULL){
			tmp = first->next;
			free(first);
			first = tmp;
		}
	}
	free(tmp);
	free(first);
}











int main(){
	int sock = socket(AF_INET, SOCK_STREAM, 0);		
	if(init_persistant()!=NB_PAGES){
		printf("Initialisation mémoire erronée\n");
		return 0;
	}
	p_create("a", 10, sock);
	afficherUseds();
	
	if(p_access_read("a", sock)==0){
		printf("Page 'a' inaccessible\n");
	}
	p_release_readwrite("a", sock);
	/*
	char* buf;	
	if(p_access_readwrite("a", sock)==0){
		printf("Page 'a' inaccessible\n");
	}
	write(sock, "akld", 5);*/
//	printf("read a: %s\n", buf);
	//p_release_readwrite("a", sock);

/*	read(sock, buf, psize);
	printf("read_write a: %s\n", buf);
	write(sock, "akld", 5);					// Test échoué, aucun affichage lors de la lecture de "a"

	
	afficherUseds();*/
	
	//cleanMem();
	close(sock);
	return 1;
}
