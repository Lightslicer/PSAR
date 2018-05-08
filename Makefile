.PHONY: all clean

CC: gcc
CFLAGS: -W -Wall -g


all: Serveur MainC MainS


%.o: %.c Serveur.h
	$(CC) -c $< $(CFLAGS)
	
%: %.o
	$(CC) -o $@ $^ $(CFLAGS)



clean:
	rm -f *.o Serveur MainC MainS

