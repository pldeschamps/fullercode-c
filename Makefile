CC      = gcc
CFLAGS  = -O2 -std=c99 -Wall -Wextra
LDFLAGS = -lm

.PHONY: all clean

all: fullercode_demo

fullercode_demo: fullercode.c main.c fullercode.h
	$(CC) $(CFLAGS) -o $@ fullercode.c main.c $(LDFLAGS)

clean:
	rm -f fullercode_demo fullercode_demo.exe
