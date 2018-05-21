/**	StoS.c
 * 
 * Fichier source les méthodes Serveurs liées à la synchro
 * 
 * Auteur : MARECAR Farzana
 * Projet : Entrepôt Réparti en mémoire
 * Responsable : SENS Pierre
 **/


#include "StoS.h"

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))


void heartbeat(){							/// Annonce que le serveur est actif
    struct msghdr msg;
    struct cmsghdr *cmsghdr;
    struct iovec iov[1];
    ssize_t nbytes;
    int i, *p;
    char buf[CMSG_SPACE(sizeof(int))], c;
 
    c = '*';
    iov[0].iov_base = &c;
    iov[0].iov_len = sizeof(c);
    memset(buf, 0x0b, sizeof(buf));
    cmsghdr = (struct cmsghdr *)buf;
    cmsghdr->cmsg_len = CMSG_LEN(sizeof(int));
    cmsghdr->cmsg_level = SOL_SOCKET;
    cmsghdr->cmsg_type = SCM_RIGHTS;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
    msg.msg_control = cmsghdr;
    msg.msg_controllen = CMSG_LEN(sizeof(int));
    msg.msg_flags = 0;
	p = (int *)CMSG_DATA(buf);
    *p = fd;
    msg.msg_iovlen = 1;
	
	
	while(1){
		sleep(5);
		int i;
		int nbIP = NELEMS(listIP); 
		for(i=0; i<nbIP; i++){
			if(listIP[0][i]==myIP){
				continue;
			}
			nbytes = sendmsg(listIP[1][i], &msg, 0);
			if (nbytes == -1){
				printf("Send msg failed");
			}
		}
	}
}







int prevent_init(int nbPages){

	char buf[32];
	sprintf(buf, "init %d", nbPages);
    
	int i;
	for(i=0; i<N; i++){
		/**	Annonce du nb de pages créées **/
		if(sendto(listIP[1][i], buf, sizeof(buf), 0, (struct sockaddr *)&server , (socklen_t)sizeof(server))==-1){
			printf("Annonce init - envoie échouée: %s\n", strerror(errno));
			return 0;
		}
		
		
		
		/** Reception de reponse **/
		if(recvfrom(listIP[1][i], buf, 10, 0, (struct sockaddr *)&server , (socklen_t *)sizeof(server))<sizeof("init XX")){
			printf("Réponse annonce init - réception échouée: %s\n", strerror(errno));
			return 0;
		}
		char *res = buf;
		strtok(res, " ");
		if(strcmp(res, "init")==0){
			res = strtok(NULL, " ");
			if(strcmp(res, "OK")!=0){
				printf("Error prevent init server %s\n", listIP[0][i]);
				return 0;
			}
		}
	}
	return 1;
}



int prevent_create(char* name, char* val){
	char buf[32];
	sprintf(buf, "create %s %s", name, val);
    
	int i;
	for(i=0; i<N; i++){
		/**	Annonce du nb de pages créées **/
		if(sendto(listIP[1][i], buf, sizeof(buf), 0, (struct sockaddr *)&server , (socklen_t)sizeof(server))==-1){
			printf("Annonce create - envoie échouée: %s\n", strerror(errno));
			return 0;
		}
		
		
		
		/** Reception de reponse **/
		if(recvfrom(listIP[1][i], buf, 10, 0, (struct sockaddr *)&server , (socklen_t *)sizeof(server))<sizeof("create XX")){
			printf("Réponse annonce create - réception échouée: %s\n", strerror(errno));
			return 0;
		}
		char *res = buf;
		strtok(res, " ");
		if(strcmp(res, "create")==0){
			res = strtok(NULL, " ");
			if(strcmp(res, "OK")!=0){
				printf("Error prevent create server %s\n", listIP[0][i]);
				return 0;
			}
		}
	}
	return 1;
}



int prevent_access_read(char* name, int clnt){
	char buf[32];
	sprintf(buf, "access_read %s %d", name, clnt);
    
	int i;
	for(i=0; i<N; i++){
		/**	Annonce du nb de pages créées **/
		if(sendto(listIP[1][i], buf, sizeof(buf), 0, (struct sockaddr *)&server , (socklen_t)sizeof(server))==-1){
			printf("Annonce access_read - envoie échouée: %s\n", strerror(errno));
			return 0;
		}
		
		
		
		/** Reception de reponse **/
		if(recvfrom(listIP[1][i], buf, 10, 0, (struct sockaddr *)&server , (socklen_t *)sizeof(server))<sizeof("access_read XX")){
			printf("Réponse annonce access_read - réception échouée: %s\n", strerror(errno));
			return 0;
		}
		char *res = buf;
		strtok(res, " ");
		if(strcmp(res, "create")==0){
			res = strtok(NULL, " ");
			if(strcmp(res, "OK")!=0){
				printf("Error prevent access_read server %s\n", listIP[0][i]);
				return 0;
			}
		}
	}
	return 1;
}



int prevent_access_readwrite(char* name, int clnt){
	char buf[32];
	sprintf(buf, "access_readwrite %s %d", name, clnt);
    
	int i;
	for(i=0; i<N; i++){
		/**	Annonce du nb de pages créées **/
		if(sendto(listIP[1][i], buf, sizeof(buf), 0, (struct sockaddr *)&server , (socklen_t)sizeof(server))==-1){
			printf("Annonce access_readwrite - envoie échouée: %s\n", strerror(errno));
			return 0;
		}
		
		
		
		/** Reception de reponse **/
		if(recvfrom(listIP[1][i], buf, 10, 0, (struct sockaddr *)&server , (socklen_t *)sizeof(server))<sizeof("access_readwrite XX")){
			printf("Réponse annonce access_readwrite - réception échouée: %s\n", strerror(errno));
			return 0;
		}
		char *res = buf;
		strtok(res, " ");
		if(strcmp(res, "create")==0){
			res = strtok(NULL, " ");
			if(strcmp(res, "OK")!=0){
				printf("Error prevent access_readwrite server %s\n", listIP[0][i]);
				return 0;
			}
		}
	}
	return 1;
}



int prevent_release_read(char* name, int clnt){
	char buf[32];
	sprintf(buf, "release_read %s %d", name, clnt);
    
	int i;
	for(i=0; i<N; i++){
		/**	Annonce du nb de pages créées **/
		if(sendto(listIP[1][i], buf, sizeof(buf), 0, (struct sockaddr *)&server , (socklen_t)sizeof(server))==-1){
			printf("Annonce release_read - envoie échouée: %s\n", strerror(errno));
			return 0;
		}
		
		
		
		/** Reception de reponse **/
		if(recvfrom(listIP[1][i], buf, 10, 0, (struct sockaddr *)&server , (socklen_t *)sizeof(server))<sizeof("release_read XX")){
			printf("Réponse annonce release_read - réception échouée: %s\n", strerror(errno));
			return 0;
		}
		char *res = buf;
		strtok(res, " ");
		if(strcmp(res, "release_read")==0){
			res = strtok(NULL, " ");
			if(strcmp(res, "OK")!=0){
				printf("Error prevent release_read server %s\n", listIP[0][i]);
				return 0;
			}
		}
	}
	return 1;
}



int prevent_release_readwrite(char* name, int clnt, char* buf){
	char buf[32];
	sprintf(buf, "release_readwrite %s %d", name, clnt);
    
	int i;
	for(i=0; i<N; i++){
		/**	Annonce du nb de pages créées **/
		if(sendto(listIP[1][i], buf, sizeof(buf), 0, (struct sockaddr *)&server , (socklen_t)sizeof(server))==-1){
			printf("Annonce release_readwrite - envoie échouée: %s\n", strerror(errno));
			return 0;
		}
		
		
		
		/** Reception de reponse **/
		if(recvfrom(listIP[1][i], buf, 10, 0, (struct sockaddr *)&server , (socklen_t *)sizeof(server))<sizeof("release_readwrite XX")){
			printf("Réponse annonce release_readwrite - réception échouée: %s\n", strerror(errno));
			return 0;
		}
		char *res = buf;
		strtok(res, " ");
		if(strcmp(res, "release_readwrite")==0){
			res = strtok(NULL, " ");
			if(strcmp(res, "OK")!=0){
				printf("Error prevent release_readwrite server %s\n", listIP[0][i]);
				return 0;
			}
		}
	}
	return 1;
}




