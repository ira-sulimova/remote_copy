CC = gcc

all: server client

server: server.c
	$(CC) -o server server.c

client: client.c
	$(CC) -o client client.c

.PHONY: clean

clean: 
	rm -rf *.o server client
