.PHONY: all clean


all: MainS





StoC.o: StoC.c StoC.h ServerListS.h
	 gcc -c StoC.c -Wall -lm


MainS.o: MainS.c StoC.o
	gcc -c MainS.c


MainS: MainS.o
	gcc -o MainS MainS.o StoC.o




clean:
	rm -rf *.o StoC MainS *.gch
