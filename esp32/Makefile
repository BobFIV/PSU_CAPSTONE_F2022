CC=gcc
CFLAGS=-Wall -I. -fpic -g -fbounds-check -Werror

esp: esp.c esp.h
	$(CC) $(CFLAGS) esp.c esp.h -o esp

clean:
	rm esp
