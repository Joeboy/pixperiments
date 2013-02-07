CWD = $(shell pwd)
ifeq ($(TARGET), LINUX)

PLATFORM_SRC = $(wildcard includes/linux/*.c)

all : pitracker_exe

CC = gcc
OBJDUMP = objdump
OBJCOPY = objcopy

else

TARGET = RASPBERRY_PI

AS = arm-none-eabi-as
export CC = arm-none-eabi-gcc
export LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump
OBJCOPY = arm-none-eabi-objcopy

EXTRA_CFLAGS = -I$(CWD)/includes/posix -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfloat-abi=hard -mfpu=vfp -ffast-math -nostdlib
AFLAGS = --warn --fatal-warnings -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -march=armv6

PLATFORM_SRC = $(wildcard includes/pi/*.c)

all : kernel_img

endif

export CFLAGS = -D$(TARGET) -I$(CWD)/includes -Wall -O2 -std=c99 $(EXTRA_CFLAGS)

PLATFORM_OBJSX = $(PLATFORM_SRC:.c=.o)
PLATFORM_OBJS = $(PLATFORM_OBJSX:includes%=libs%)

POSIX_SRC = $(wildcard includes/posix/*.c)
POSIX_OBJSX = $(POSIX_SRC:.c=.o)
POSIX_OBJS = $(POSIX_OBJSX:includes%=libs%)


clean :
	rm -f *.o
	rm -f libs/*.o
	rm -f libs/*/*.o
	rm -f plugins/*/*.o
	rm -f *.bin
	rm -f *.elf
	rm -f kernel.img pitracker

libdirs:
	mkdir -p libs/posix
	mkdir -p libs/linux
	mkdir -p libs/pi

tune_o : tune.mid
	$(LD) -s -r -o tune.o -b binary tune.mid

vectors_o : vectors.s
	$(AS) $(AFLAGS) vectors.s -o vectors.o

lv2_o: includes/lv2.h includes/lv2.c
	$(CC) $(CFLAGS) -c includes/lv2.c -o libs/lv2.o

posix: libdirs $(POSIX_OBJS)

libs/posix/%.o: includes/posix/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

platform: libdirs $(PLATFORM_OBJS)

libs/linux/%.o: includes/linux/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

libs/pi/%.o: includes/pi/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

hardware: libdirs $(PLATFORM_OBJS)

pitracker_o : pitracker.c
	$(CC) $(CFLAGS) -c pitracker.c -o pitracker.o

pitracker.elf : memmap vectors_o pitracker_o tune_o posix hardware lv2_o prepare_plugins
	$(LD) vectors.o plugins/piano/piano.o plugins/sawtooth/sawtooth.o plugins/organ/organ.o pitracker.o libs/lv2.o tune.o $(PLATFORM_OBJS) $(POSIX_OBJS) -T memmap -o pitracker.elf

pitracker_exe :  tune_o hardware lv2_o prepare_plugins
	$(CC) $(CFLAGS) pitracker.c plugins/piano/piano.o plugins/sawtooth/sawtooth.o plugins/organ/organ.o libs/lv2.o tune.o $(PLATFORM_OBJS) -lm -lasound -o pitracker

kernel_img : pitracker.elf
	$(OBJCOPY) pitracker.elf -O binary kernel.img
	cp kernel.img SD_Card/


piano : plugins/piano/piano.c
	make -C plugins/piano

sawtooth : plugins/piano/piano.c
	make -C plugins/sawtooth

organ : plugins/organ/organ.c
	make -C plugins/organ

plugins : piano sawtooth organ

prepare_plugins : plugins
	$(OBJDUMP) -t plugins/piano/piano.o | grep lv2_descriptor
	$(OBJCOPY) --redefine-sym  lv2_descriptor=lv2_descriptor_1 plugins/piano/piano.o
	$(OBJCOPY) --redefine-sym  lv2_descriptor=lv2_descriptor_2 plugins/organ/organ.o
	$(OBJCOPY) --redefine-sym  lv2_descriptor=lv2_descriptor_3 plugins/sawtooth/sawtooth.o

