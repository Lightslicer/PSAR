/**	StoC.c
 * 
 * Fichier source les méthodes Serveurs liées aux services
 * 
 * Auteur : MARECAR Farzana
 * Projet : Entrepôt Réparti en mémoire
 * Responsable : SENS Pierre
 **/


#include "StoC.h"
#include "StoS.h"




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
		nouveau->lects = NULL;
		pthread_mutex_init(nouveau->mutex, NULL);
		nouveau->next=unused->list;
		unused->list=nouveau;
		unused->nbUnused++;
		printf("Page n°%d:		@:%p\n", unused->nbUnused, nouveau->begin);		
	}
	pthread_mutex_unlock(unused->mutex);

	// appeler méthode réplication initiale sur serveur
	prevent_init(unused->nbUnused);
	
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
	struct client *tmp = p->lects;
	printf("isReader: sock=%d", sock);
	while(tmp != NULL){
		if((tmp->sock==sock){
			return 1;						/// true
		}
		printf("%d\n", tmp);
	}
	return 0;								/// false
}


int removeReader(struct page* p, int sock){
	struct client *tmp = p->lects;
	while(tmp!=NULL){
         if(tmp->sock==sock){
			tmp = tmp->next; 
			return 1;
         }      
    }
	return 0;	
}


/** Format de messages recus : create <nom> <val>
 * 	Format de messages envoyes : create <nom> OK <details> 
 * 						ou	   : create <nom> KO <details> 
 **/
char* p_create(char* name, char* val, int sock){
	static char res[4096];
	if(getPage(name)!=NULL){
		sprintf(res, "create %s KO Error - Page already exists", name);
		return res;
	}
	pthread_mutex_lock(used->mutex);
	pthread_mutex_lock(unused->mutex);
	if(used->nbUsed==NB_PAGES || unused->nbUnused==0){
		/// Si déjà 1000000000 de pages allouées OU une page portant ce nom existe OU 0 page libre disponible --> false
		sprintf(res, "create %s KO Error Server - Server Memory full", name);
		pthread_mutex_unlock(used->mutex);
		pthread_mutex_unlock(unused->mutex);
		return res;
	}

	/// Retrait d'une page depuis la liste des pages non utilisees
	struct page *p = unused->list;
	unused->list = unused->list->next;
	pthread_mutex_unlock(unused->mutex);

	if(p!=NULL){
		/// Poser le verroue de la page
		pthread_mutex_lock(p->mutex);
		
		p->name=name;
		
		if(mmap(p->begin, psize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0)==MAP_FAILED){	// mmap fonctionnel que dans une hôte
			/// On le remet dans la liste des non utilisés
			p->name=NULL;
			p->next=unused->list;
			unused->list=p;
			pthread_mutex_unlock(p->mutex);
			sprintf(res, "create %s KO Error Server - Memory Error", name);
			return res;
		}
		memcpy(p->begin, val, sizeof(*val));
		p->next = used->list;
		used->list = p;
		used->nbUsed++;
		/**printf("New page used: Name=%s Size=%zu octets @=%p\n", p->name, psize, p->begin);**/
		pthread_mutex_unlock(used->mutex);

		// Prevenir les autres
		prevent_create(name, val);		
		
		/// Deverouiller l'acces a la page
		pthread_mutex_unlock(p->mutex);
		sprintf(res, "create %s OK Page created with success !", name);
	}
	else{
		sprintf(res, "create %s KO Error Server - Could not create page", name);
	}
	return res;
}



/** 
 *  Format de messages recus : access_read <nom>
 * 	Format de messages envoyes : access_read <nom> OK <val> 
 * 						ou	   : access_read <nom> KO <details>
 **/
char* p_access_read(char *name, int sock){
	static char res[4096];
	struct page* p = getPage(name);
	if(p==NULL){ 	/// La page n'existe pas
		sprintf(res, "access_read %s KO Error - Page doesn't exist", name);
		return res;
	}
		
	while(p->writer!=0){		/// Si quelqu'un lit
		sleep(2); // attente active, pas le plus optimal
	}
	
	/// Poser le verroue de la page
	if(isReader(p, sock){
		sprintf(res, "access_read %s KO Error - You're already a reader of it", name);
		return res;
	}
	pthread_mutex_lock(p->mutex);
	
	struct client* nv = malloc(sizeof(struct client*));
	nv->sock = sock;
	nv->next=p->lects;
	p->lects=nv;
	
	char* dest=malloc(sizeof(psize));
	memcpy(dest, p->begin, psize);
	sprintf(res, "access_read %s OK %s", name, dest);
	///printf("p_access_read %s %p '%s' %d\n", p->name, p->begin, dest, p->nbR);
	
	// prévenir les autres serveurs
	prevent_access_read(name, sock);

	/// Déverouiller l'accès à la page
	pthread_mutex_unlock(p->mutex);
	
	return res;
}




/**
 * Format de messages recus : access_readwrite <nom>	
 * Format de messages envoyes : access_readwrite <nom> OK <val> 
 * 						ou	   : access_readwrite <nom> KO <details> 
 **/
char* p_access_readwrite(char *name ,int sock){		// prévoir le cas où un client quitte le réseau sans libérer la donnée proprement
	struct page* p = getPage(name);
	static char res[4096];
	if(p==NULL){ 		/// La page n'existe pas
		sprintf(res, "access_readwrite %s KO Error - Page doesn't exist", name);
		return res;
	}
	
	while(p->writer!=0 || p->lects!=NULL){		/// Si une écriture en cours OU une lecture en cours
		sleep(2); // attente active, pas le plus optimal
	}

	/// Poser le verroue de la page
	pthread_mutex_lock(p->mutex);
	
	char* dest=malloc(sizeof(psize));
	memcpy(dest, p->begin, psize);
	sprintf(res, "access_readwrite %s OK %s", name, dest);
	/**	printf("p_access_readwrite %s %p '%s'\n", p->name, p->begin, dest);**/
	
	// Prévenir les autres		
	p->writer=sock;
	prevent_access_readwrite(name, sock);

	/// Déverouiller l'accès à la page
	pthread_mutex_unlock(p->mutex);
	return res;
}



/** Fin Lecture :
 *  Format de messages envoyes : release_read <nom>
 * 	Format de messages recus : release_read <nom> OK <details> 
 * 						ou	 : release_read <nom> KO <details>
 **/
char* p_release_read(char *name ,int sock){
	static char res[4096];
	struct page* p = getPage(name);
	if((p==NULL)){ 		/// La page n'existe pas
		sprintf(res, "release_read %s KO Error - Page doesn't exist", name);
		return res;
	}

	if(p->lects != NULL && isReader(p, sock)==1){
		/// Poser le verroue de la page
		pthread_mutex_lock(p->mutex);
		
		if(mprotect(p->begin, psize, PROT_NONE)==-1){
			pthread_mutex_unlock(p->mutex);
			return 0;
		}
		removeReader(p, sock);
		
		// prévenir les autres
		prevent_release_read(name, sock);

		/// Déverouiller l'accès à la page
		pthread_mutex_unlock(p->mutex);
		sprintf(res, "release_read %s OK Released successfully!", name); 
	}
	else{
		sprintf(res, "release_read %s KO Error - You're not a reader of this page", name);
	}
	return res;
}



/** Format de messages envoyes : release_readwrite <nom> <val>
 * 	Format de messages recus : release_readwrite <nom> OK <details> 
 * 						ou	 : release_readwrite <nom> KO <details>  
 **/
char* p_release_readwrite(char* name, int sock, char* buf){
	struct page *p = getPage(name);
	static char res[4096];
	if(p==NULL){ 		/// La page n'existe pas
		sprintf(res, "release_readwrite %s KO Error - Page doesn't exist", name); 
		return res;
	}
	if(p->writer==sock){
		/// Poser le verroue de la page
		pthread_mutex_lock(p->mutex);
		
		char* buf=malloc(psize);
		read(sock, buf, psize);
		
		if(memcpy(p->begin, buf, psize)==NULL||mprotect(p->begin, psize, PROT_NONE)==-1){
			pthread_mutex_unlock(p->mutex);
			sprintf(res, "release_readwrite %s KO Error Server - Memory error", name);
			return res;
		}
		p->writer=0;
		
		// prévenir les autres
		prevent_release_readwrite(name, sock, buf);

		/// Déverouiller l'accès à la page
		pthread_mutex_unlock(p->mutex);
		sprintf(res, "release_readwrite %s OK Released successfully", name);
	}
	else{
		sprintf(res, "release_readwrite %s KO Error - You're not a writer of this page", name);
	}
	return res;
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


