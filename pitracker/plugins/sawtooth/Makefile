COPS ?= -I../../includes -I../../includes/posix -Wall -O2 -std=c99 -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp

all : sawtooth.o

clean :
	rm -f *.o

sawtooth.o : sawtooth.c
	$(CC) $(COPS) -c sawtooth.c -o sawtooth.o

