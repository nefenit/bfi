# (c) Copyright 2019 Bartosz Mierzynski

CC=cc
CFLAGS=-Wall -pedantic -ansi -O2 -g

.PHONY: all
all:
	@$(CC) bfi.c -o bfi $(CFLAGS) 

.PHONY: clean
clean:
	@rm bfi
