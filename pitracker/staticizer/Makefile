CFLAGS = -Wall -O2 -std=c99 `pkg-config --cflags lilv-0`
LDFLAGS = `pkg-config --libs lilv-0`
CC = gcc

all : binary

clean :
	rm -f *.o
	rm -f staticizer
	rm -f descriptors.h

binary : staticizer.c md5.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o staticizer

%.o : %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $^ -o $@

