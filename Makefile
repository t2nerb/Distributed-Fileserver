CC=gcc
CFLAGS= -Wall
LDFLAGS=-lssl -lcrypto

.PHONY: clean all

all: server client

server: server.c
	$(CC) $(CFLAGS) $^ -o $@

client: client.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f server
	rm -f client
	rm -f *.o
