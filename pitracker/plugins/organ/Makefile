ARMGNU ?= arm-none-eabi

COPS = -I../../includes -I../../includes/posix -Wall -O2 -std=c99 -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp
AOPS = --warn --fatal-warnings -mcpu=arm1176jzf-s -march=armv6


all : organ.o

clean :
	rm -f *.o

organ.o : organ.c
	$(ARMGNU)-gcc $(COPS) -c organ.c -o organ.o

