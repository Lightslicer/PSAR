/**	Client.c
 * Fichier sources implémentant les méthodes creer, lire et modifier côté Client
 * Les méthodes envoient et réceptionnent eux-mêmes les messages aux Serveurs
 * Elles ont connaissance du numéro de socket de connection ainsi que de l'adresse internet transmis par le main
 * 
 * Auteurs : HE Chenhui - MARECAR Farzana
 * 			 Etudiants en 1e année de Master SAR - Paris 6
 * Client : SENS Pierre - KORDON Fabrice
 **/


#include "Client.h"





/** Format de messages envoyes : create <nom> <val>
 * 	Format de messages recus : create <nom> OK <details> 
 * 						ou	 : create <nom> KO <details> 
 **/
int creer(int sock, struct sockaddr_in server, char* nom, char* val){

	char buf[4096];
	sprintf(buf, "create %s %s", nom, val);
	printf("%s\n",buf);
	
	/**	Demande de creation **/
	if(sendto(sock, buf, sizeof(buf), 0, (struct sockaddr *)&server , (socklen_t)sizeof(server))==-1){
		printf("Creation - envoie échouée: %s\n", strerror(errno));
		return 0;
	}
	
	
	
	/** Reception de reponse **/
	if(recvfrom(sock, buf, 10, 0, (struct sockaddr *)&server , (socklen_t *)sizeof(server))<sizeof("create ")+sizeof(nom)+sizeof(" xx")){
		printf("Creation - réception échouée: %s\n", strerror(errno));
		return 0;
	}
	
	char *res = buf;
	strtok(res, " "); 
	int i = 0;
    while (res != NULL){
		if(i==0 && strcmp(res, "create")!=0){
	         break;
		}
		if(i==1 && strcmp(res, nom)!=0){
			break;
		}
		if(i==2 && strcmp(res, "OK")==0){
			printf("Creation reussie :\n");
			while((res=strtok(NULL, " "))!=NULL){
				printf("%s ", res);
			}
			printf("\n");
	        return 1;
		}
		else if(i==2 && strcmp(res, "KO")==0){
			printf("Creation echouee :\n");
			while((res=strtok(NULL, " "))!=NULL){
				printf("%s ", res);
			}
			printf("\n");
			return 0;
		}
		else if(i==2 && strcmp(res, "OK")!=0 && strcmp(res, "KO")!=0){
			break;
		}
		res = strtok (NULL, " ");
		i++;
    }
	printf("Creation Echouee: Erreur message format\n");
	
	return 0;
}




















/** 1) Récupération de valeur : (p_access_read)
 *  Format de messages envoyes : access_read <nom>
 * 	Format de messages recus : access_read <nom> OK <val> 
 * 						ou	 : access_read <nom> KO <details>
 *  2) Fin Lecture :
 *  Format de messages envoyes : release_read <nom>
 * 	Format de messages recus : release_read <nom> OK <details> 
 * 						ou	 : release_read <nom> KO <details>
 **/
int lire(int sock, struct sockaddr_in server, char* nom){

	char buf[4096];
	sprintf(buf, "access_read %s", nom);
	printf("%s\n",buf);
	
   /* Récupération de valeur */	
	/**	Demande de lecture **/
	if(sendto(sock, buf, sizeof(buf), 0, (struct sockaddr *)&server , (socklen_t)sizeof(server))==-1){
		printf("Lecture - envoie échouée: %s\n", strerror(errno));
		return 0;
	}
	
	
	
	/** Reception de resultat **/
	if(recvfrom(sock, buf, 10, 0, (struct sockaddr *)&server , (socklen_t *)sizeof(server))<sizeof("access_read  xx")+sizeof(nom)){
		printf("Lecture - réception échouée: %s\n", strerror(errno));
		return 0;
	}
	
	char *res = buf;
	strtok(res, " "); 
	int i = 0;
    while (res != NULL){
		if(i==0 && strcmp(res, "access_read")!=0){
	         break;
		}
		if(i==1 && strcmp(res, nom)!=0){
			break;
		}
		if(i==2 && strcmp(res, "OK")==0){
			/** Affichage de resultat -> lecture seule **/
			printf("Lecture reussie :\n");	
			while((res = strtok (NULL, " "))!=NULL){
				printf("%s ", res);
			}
			printf("\n");
	        goto endLect;
		}
		else if(i==2 && strcmp(res, "KO")==0){
			printf("Lecture echouee :\n");
			while((res = strtok (NULL, " "))!=NULL){
				printf("%s ", res);
			}
			printf("\n");
			return 0;
		}
		else if(i==2 && strcmp(res, "OK")!=0 && strcmp(res, "KO")!=0){
			break;
		}
		res = strtok (NULL, " ");
		i++;
    }
	printf("Lecture Echouee: Erreur message format\n");	
	return 0;


  /* Fin Lecture */	
	endLect:
		sprintf(buf, "release_read %s", nom);
		printf("%s\n",buf);	
		if(sendto(sock, buf, sizeof(buf), 0, (struct sockaddr *)&server , (socklen_t)sizeof(server))==-1){
			printf("Fin Lecture - envoie échouée: %s\n", strerror(errno));
			goto endLect;
		}
		while(recvfrom(sock, buf, 10, 0, (struct sockaddr *)&server , (socklen_t *)sizeof(server))<sizeof("release_read  xx")+sizeof(nom)){
			printf("Fin Lecture - reception échouée: %s\n", strerror(errno));
		}
		res = buf;
		strtok(res, " "); 
		i = 0;
		while (res != NULL){
			if(i==0 && strcmp(res, "release_read")!=0){
				 break;
			}
			if(i==1 && strcmp(res, nom)!=0){
				 break;
			}
			if(i==2 && strcmp(res, "OK")==0){
				/** Affichage de resultat -> lecture seule **/			
				printf("Fin Lecture reussie :\n");
				while((res = strtok (NULL, " "))!=NULL){
					printf("%s ", res);
				}
				printf("\n");
				return 1;
			}
			else if(i==2 && strcmp(res, "KO")==0){
				printf("Fin Lecture echouee :\n");
				while((res = strtok (NULL, " "))!=NULL){
					printf("%s ", res);
				}
				printf("\n");
				return 0;
			}
			else if(i==2 && strcmp(res, "OK")!=0 && strcmp(res, "KO")!=0){
				break;
			}
			res = strtok (NULL, " ");
			i++;
		}
		printf("Fin Lecture Echouee: Erreur message format\n");
		
	return 0;
}




















/** Récupération de valeur :
 * Format de messages envoyes : access_readwrite <nom>	
 * Format de messages recus : access_readwrite <nom> OK <val> 
 * 						ou	 : access_readwrite <nom> KO <details> 
 * Renvoie de modification :
 * 	Format de messages envoyes : release_readwrite <nom> <val>
 * 	Format de messages recus : release_readwrite <nom> OK <details> 
 * 						ou	 : release_readwrite <nom> KO <details>  
 **/
int modifier(int sock, struct sockaddr_in server, char* nom){

	char buf[4096];
	sprintf(buf, "access_readwrite %s", nom);
	printf("%s\n",buf);
	
  
   /* Récupération de valeur */
	/**	Demande de lecture/ecriture **/
	if(sendto(sock, buf, sizeof(buf), 0, (struct sockaddr *)&server , (socklen_t)sizeof(server))==-1){
		printf("Lecture/Ecriture - envoie échouée: %s\n", strerror(errno));
		return 0;
	}
		
	/** Reception de resultat **/
	if(recvfrom(sock, buf, 10, 0, (struct sockaddr *)&server , (socklen_t *)sizeof(server))<sizeof("access_readwrite  xx")+sizeof(nom)){
		printf("Lecture/Ecriture - réception échouée: %s\n", strerror(errno));
		return 0;
	}
	
	char* res = strtok(buf, " "); 
	int i = 0;
    while (res != NULL){
		if(i==0 && strcmp(res, "access_readwrite")!=0){
	         break;
		}
		if(i==0 && strcmp(res, nom)!=0){
	         break;
		}
		if(i==2 && strcmp(res, "KO")==0){
			printf("Lecture/Ecriture echouee :\n");
			while((res = strtok (NULL, " "))!=NULL){
				printf("%s ", res);
			}
			printf("\n");
			return 0;
		}		
		if(i==2 && strcmp(res, "OK")==0){
			res = strtok (NULL, " ");			
			printf("Lecture/Ecriture reussie :\n");
			while((res = strtok (NULL, " "))!=NULL){
				printf("%s ", res);
			}
			printf("\n");
			goto writing;
		}
		else if(i==2 && strcmp(res, "OK")!=0 && strcmp(res, "KO")!=0){
			break;
		}
		res = strtok (NULL, " ");
		i++;
    }
	printf("Ecriture Echouee: Erreur message format\n");	
	return 0;
	
	
	
	
	
  /* Renvoie de modification  - Fin Lecture/Ecriture */
	writing:
		sprintf(buf, "release_readwrite %s ", nom);
		///printf("%s\n",buf);
		printf("Veuillez entrez la nouvelle valeur :\n(Terminez votre saisie en saisissant 'fin')\n");
		char tmp[4096];
		while(scanf("%s", tmp) && strcmp(tmp, "fin")!=0){
			strcat(buf, tmp);
		}
		
		if(sendto(sock, buf, sizeof(buf), 0, (struct sockaddr *)&server , (socklen_t)sizeof(server))==-1){
			printf("Ecriture - envoie échouée: %s\n", strerror(errno));
			goto writing;
		}
		while(recvfrom(sock, buf, 10, 0, (struct sockaddr *)&server , (socklen_t *)sizeof(server))<sizeof("write xx")){
			printf("Ecriture - réception échouée: %s\n", strerror(errno));
		}
		res = strtok(buf, " "); 
		i = 0;
		while (res != NULL){
			if(i==0 && strcmp(res, "release_readwrite")!=0){
				 break;
			}
			if(i==1 && strcmp(res, nom)!=0){
				 break;
			}
			if(i==2 && strcmp(res, "OK")==0){
				/** Affichage de resultat -> lecture seule **/
				printf("Fin Ecriture reussie : %s\n", res);
				while((res = strtok (NULL, " "))!=NULL){
					printf("%s ", res);
				}
				printf("\n");
				return 1;
			}
			if(i==2 && strcmp(res, "KO")==0){
				res = strtok (NULL, " ");
				printf("Fin Ecriture echouee : %s\n", res);
				goto writing;
			}
			else if(i==2 && strcmp(res, "OK")!=0 && strcmp(res, "KO")!=0){
				break;
			}
			res = strtok (NULL, " ");
			i++;
		}
	printf("Fin Ecriture Echouee: Erreur message format\n");	
	return 0;
}


