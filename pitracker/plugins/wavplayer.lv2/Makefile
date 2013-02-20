CFLAGS ?= -I../../includes -I../../includes/posix -Wall -O2 -std=c99 -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp

all : wavplayer.o

clean :
	rm -f *.o

wavplayer.o : player.o sample.o
	$(LD) -r player.o sample.o -o $@

player.o : wavplayer.c
	$(CC) $(CFLAGS) -c wavplayer.c -o player.o

sample.o : sample.wav
	$(LD) -s -r -o $@ -b binary $^

