ARMGNU ?= arm-none-eabi

COPS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp
AOPS = --warn --fatal-warnings -mcpu=arm1176jzf-s -march=armv6


all : kernel.img

clean :
	rm -f *.o
	rm -f *.bin
	rm -f *.elf
	rm -f kernel.img

vectors.o : vectors.s
	$(ARMGNU)-as $(AOPS) -mfpu=vfp vectors.s -o vectors.o

pitracker.o : pitracker.c
	$(ARMGNU)-gcc $(COPS) -c pitracker.c -o pitracker.o

pitracker.elf : memmap vectors.o pitracker.o 
	$(ARMGNU)-ld vectors.o pitracker.o -T memmap -o pitracker.elf

kernel.img : pitracker.elf
	$(ARMGNU)-objcopy pitracker.elf -O binary kernel.img

