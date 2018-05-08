/**	ServerListC.c
 * Fichier header avec le numéro de port de service Serveur (le même pour tous)
 * et la liste des IP Serveurs (espece de fstab/liste de logs développé)
 * Définit une méthode initialisant cette liste et un méthode renvoyant un IP au hasard dans la liste 
 * pour assurer une répartition de la charge 
 * 
 * Auteurs : HE Chenhui - MARECAR Farzana
 * 			 Etudiants en 1e année de Master SAR - Paris 6
 * Client : SENS Pierre - KORDON Fabrice
 **/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>


#define PORT_SERVICE 2000
#define N 10


char* listIP[N];

void init_list(){
	srand(time(NULL));
	listIP[0] = "127.0.0.1";
	listIP[1] = "127.0.0.2";
	listIP[2] = "127.0.0.3";
	listIP[3] = "127.0.0.4";
	listIP[4] = "127.0.0.5";
	listIP[5] = "127.0.0.6";
	listIP[6] = "127.0.0.7";
	listIP[7] = "127.0.0.8";
	listIP[8] = "127.0.0.9";
	listIP[9] = "127.0.0.10";
}

char* SIP_alea(){
	int r = (rand() % (N + 1 - 0));
	return listIP[r];
}

