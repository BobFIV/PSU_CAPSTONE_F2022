CC=gcc
CFLAGS=-Wall -I. -fpic -g -fbounds-check -Werror

server: server.c server.h
	$(CC) $(CFLAGS) server.c server.h -o server

clean:
	rm server
