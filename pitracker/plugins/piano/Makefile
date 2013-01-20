ARMGNU ?= arm-none-eabi
COPS = -I../../includes -Wall -O2 -std=c99 -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp
AOPS = --warn --fatal-warnings -mcpu=arm1176jzf-s -march=armv6


all : piano.o

clean :
	rm -f *.o

piano.o : piano.c
	$(ARMGNU)-gcc $(COPS) -c piano.c -o piano.o

