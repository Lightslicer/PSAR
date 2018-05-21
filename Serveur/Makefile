.PHONY: all clean


all: MainS





Serveur.o: Serveur.c Serveur.h ServerListS.h
	 gcc -c Serveur.c -Wall -lm


MainS.o: MainS.c Serveur.o
	gcc -c MainS.c


MainS: MainS.o
	gcc -o MainS MainS.o Serveur.o




clean:
	rm -rf *.o StoC MainS *.gch
