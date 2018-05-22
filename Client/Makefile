.PHONY: all clean



all: MainC


Client.o: Client.c Client.h
	gcc -c Client.c -lm


MainC.o: MainC.c ServerListC.h
	gcc -c MainC.c -lm


MainC: MainC.o Client.o
	gcc -o MainC MainC.o Client.o



clean:
	rm -f *.o MainC *.gch
