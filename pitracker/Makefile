ARMGNU ?= arm-none-eabi
COPS = -I./includes -Wall -O2 -std=c99 -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp
AOPS = --warn --fatal-warnings -mcpu=arm1176jzf-s -march=armv6


all : kernel.img plugins

clean :
	rm -f *.o
	rm -f libs/*.o
	rm -f libs/pi/*.o
	rm -f plugins/*/*.o
	rm -f *.bin
	rm -f *.elf
	rm -f kernel.img

tune_o : tune.mid
	arm-none-eabi-ld -s -r -o tune.o -b binary tune.mid

vectors_o : vectors.s libs
	$(ARMGNU)-as $(AOPS) -mfpu=vfp vectors.s -o vectors.o

string_o: includes/string.h includes/string.c
	$(ARMGNU)-gcc $(COPS) -c includes/string.c -o libs/string.o

unistd_o: includes/unistd.h includes/unistd.c
	$(ARMGNU)-gcc $(COPS) -c includes/unistd.c -o libs/unistd.o

malloc_o: includes/malloc.h includes/malloc.c
	$(ARMGNU)-gcc $(COPS) -c includes/malloc.c -o libs/malloc.o

assert_o: includes/assert.h includes/assert.c
	$(ARMGNU)-gcc $(COPS) -c includes/assert.c -o libs/assert.o

audio_o: includes/pi/audio.h includes/pi/audio.c
	$(ARMGNU)-gcc $(COPS) -c includes/pi/audio.c -o libs/pi/audio.o

uart_o: includes/pi/uart.h includes/pi/uart.c
	$(ARMGNU)-gcc $(COPS) -c includes/pi/uart.c -o libs/pi/uart.o

stdio_o: includes/stdio.h includes/stdio.c
	$(ARMGNU)-gcc $(COPS) -c includes/stdio.c -o libs/stdio.o

math_o: includes/math.h includes/math.c
	$(ARMGNU)-gcc $(COPS) -c includes/math.c -o libs/math.o

pitracker_o : pitracker.c
	$(ARMGNU)-gcc $(COPS) -c pitracker.c -o pitracker.o

pitracker.elf : memmap vectors_o pitracker_o tune_o string_o unistd_o malloc_o assert_o audio_o uart_o stdio_o math_o plugins
	$(ARMGNU)-ld vectors.o plugins/sawtooth/sawtooth.o pitracker.o libs/pi/audio.o tune.o libs/assert.o libs/string.o libs/math.o libs/malloc.o libs/stdio.o libs/unistd.o libs/pi/uart.o -T memmap -o pitracker.elf

kernel.img : pitracker.elf
	$(ARMGNU)-objcopy pitracker.elf -O binary kernel.img
	cp kernel.img SD_Card/

plugins : piano sawtooth

piano : plugins/piano/piano.c
	make -C plugins/piano

sawtooth : plugins/piano/piano.c
	make -C plugins/sawtooth

