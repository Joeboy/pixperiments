ARMGNU ?= arm-none-eabi
COPS = -I./includes -I./includes/posix -Wall -O2 -std=c99 -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp
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

vectors_o : vectors.s
	$(ARMGNU)-as $(AOPS) -mfpu=vfp vectors.s -o vectors.o

string_o: includes/posix/string.h includes/posix/string.c
	$(ARMGNU)-gcc $(COPS) -c includes/posix/string.c -o libs/posix/string.o

unistd_o: includes/posix/unistd.h includes/posix/unistd.c
	$(ARMGNU)-gcc $(COPS) -c includes/posix/unistd.c -o libs/posix/unistd.o

malloc_o: includes/posix/malloc.h includes/posix/malloc.c
	$(ARMGNU)-gcc $(COPS) -c includes/posix/malloc.c -o libs/posix/malloc.o

assert_o: includes/posix/assert.h includes/posix/assert.c
	$(ARMGNU)-gcc $(COPS) -c includes/posix/assert.c -o libs/posix/assert.o

stdio_o: includes/posix/stdio.h includes/posix/stdio.c
	$(ARMGNU)-gcc $(COPS) -c includes/posix/stdio.c -o libs/posix/stdio.o

math_o: includes/posix/math.h includes/posix/math.c
	$(ARMGNU)-gcc $(COPS) -c includes/posix/math.c -o libs/posix/math.o

audio_o: includes/pi/audio.h includes/pi/audio.c
	mkdir -p libs/pi
	$(ARMGNU)-gcc $(COPS) -c includes/pi/audio.c -o libs/pi/audio.o

uart_o: includes/pi/uart.h includes/pi/uart.c
	mkdir -p libs/pi
	$(ARMGNU)-gcc $(COPS) -c includes/pi/uart.c -o libs/pi/uart.o

lv2_o: includes/lv2.h includes/lv2.c
	$(ARMGNU)-gcc $(COPS) -c includes/lv2.c -o libs/lv2.o

libdirs:
	mkdir -p libs/posix

posix: libdirs math_o stdio_o assert_o malloc_o unistd_o string_o

pitracker_o : pitracker.c
	$(ARMGNU)-gcc $(COPS) -c pitracker.c -o pitracker.o

pitracker.elf : memmap vectors_o pitracker_o tune_o posix audio_o lv2_o uart_o plugins prepare_plugins
	$(ARMGNU)-ld vectors.o plugins/piano/piano.o plugins/sawtooth/sawtooth.o pitracker.o libs/lv2.o libs/pi/audio.o tune.o libs/posix/assert.o libs/posix/string.o libs/posix/math.o libs/posix/malloc.o libs/posix/stdio.o libs/posix/unistd.o libs/pi/uart.o -T memmap -o pitracker.elf

kernel.img : pitracker.elf
	$(ARMGNU)-objcopy pitracker.elf -O binary kernel.img
	cp kernel.img SD_Card/

piano : plugins/piano/piano.c
	make -C plugins/piano

sawtooth : plugins/piano/piano.c
	make -C plugins/sawtooth

plugins : piano sawtooth

prepare_plugins : plugins
	$(ARMGNU)-objdump -t plugins/piano/piano.o | grep lv2_descriptor
	$(ARMGNU)-objcopy --redefine-sym  lv2_descriptor=lv2_descriptor_1 plugins/piano/piano.o
	$(ARMGNU)-objcopy --redefine-sym  lv2_descriptor=lv2_descriptor_2 plugins/sawtooth/sawtooth.o

