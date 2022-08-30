CC=gcc
CFLAGS=-Wall -I. -fpic -g -fbounds-check -Werror

thingy: thingy.c thingy.h
	$(CC) $(CFLAGS) thingy.c thingy.h -o thingy

clean:
	rm thingy
