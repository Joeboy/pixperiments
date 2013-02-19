CFLAGS ?= -I../../includes -I../../includes/posix -Wall -O2 -std=c99 -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp

all : piano.o

clean :
	rm -f *.o

piano.o : piano.c
	$(CC) $(CFLAGS) -c piano.c -o piano.o

