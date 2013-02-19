CFLAGS ?= -I../../includes -I../../includes/posix -Wall -O2 -std=c99 -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp

all : organ.o

clean :
	rm -f *.o

organ.o : organ.c
	$(CC) $(CFLAGS) -c organ.c -o organ.o

