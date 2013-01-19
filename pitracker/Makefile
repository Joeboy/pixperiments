ARMGNU ?= arm-none-eabi

COPS = -I./includes -Wall -O2 -std=c99 -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp
AOPS = --warn --fatal-warnings -mcpu=arm1176jzf-s -march=armv6


all : kernel.img
	cp kernel.img SD_Card/

clean :
	rm -f *.o
	rm -f *.bin
	rm -f *.elf
	rm -f kernel.img
	rm -f tune.o

tune.o : tune.mid
	arm-none-eabi-ld -s -r -o tune.o -b binary tune.mid

vectors.o : vectors.s
	$(ARMGNU)-as $(AOPS) -mfpu=vfp vectors.s -o vectors.o

pitracker.o : pitracker.c
	$(ARMGNU)-gcc $(COPS) -c pitracker.c -o pitracker.o

pitracker.elf : memmap vectors.o pitracker.o  tune.o
	$(ARMGNU)-ld tune.o vectors.o pitracker.o -T memmap -o pitracker.elf

kernel.img : pitracker.elf
	$(ARMGNU)-objcopy pitracker.elf -O binary kernel.img

