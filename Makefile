CC=gcc
CFLAGS= -Wall

.PHONY: clean all

all: server client

server: server.c
	$(CC) $(CFLAGS) $^ -o $@

client: client.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f server
	rm -f client
	rm -f *.o
