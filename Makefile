#!/usr/bin/make -f
CC=./agcc.pl

fix-bcid: fix-bcid.c mtdutils.o 
	$(CC) -o $@ mtdutils.o $<

%.o: %.c
	$(CC) -c -o $@ $<

clean:
	rm -f mtdutils.o fix-bcid

.PHONY: clean
