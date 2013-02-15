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

libdirs : libs/posix libs/linux libs/pi
	mkdir -p $^

pitracker.o : pitracker.c
	$(CC) $(CFLAGS) -c $^ -o $@

tune.o : tune.mid
	$(LD) -s -r -o $@ -b binary $^

vectors.o : vectors.s
	$(AS) $(AFLAGS) $^ -o $@

libs/%.o: includes/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

posix: libdirs $(POSIX_OBJS)

platform: libdirs $(PLATFORM_OBJS)

pitracker.elf : memmap vectors.o pitracker.o tune.o posix platform libs/lv2.o libs/mf_read.o prepare_plugins
	$(LD) vectors.o plugins/piano/piano.o plugins/sawtooth/sawtooth.o plugins/organ/organ.o pitracker.o libs/lv2.o libs/mf_read.o tune.o $(PLATFORM_OBJS) $(POSIX_OBJS) -T memmap -o pitracker.elf

pitracker_exe :  tune.o platform libs/lv2.o libs/mf_read.o prepare_plugins
	$(CC) $(CFLAGS) pitracker.c plugins/piano/piano.o plugins/sawtooth/sawtooth.o plugins/organ/organ.o libs/lv2.o libs/mf_read.o tune.o $(PLATFORM_OBJS) -lm -lasound -o pitracker

kernel_img : pitracker.elf
	$(OBJCOPY) pitracker.elf -O binary kernel.img
	cp kernel.img SD_Card/


piano : plugins/piano/piano.o
	make -C plugins/piano

sawtooth : plugins/piano/piano.o
	make -C plugins/sawtooth

organ : plugins/organ/organ.o
	make -C plugins/organ

plugins : piano sawtooth organ

prepare_plugins : plugins
	$(OBJDUMP) -t plugins/piano/piano.o | grep lv2_descriptor
	$(OBJCOPY) --redefine-sym  lv2_descriptor=lv2_descriptor_1 plugins/piano/piano.o
	$(OBJCOPY) --redefine-sym  lv2_descriptor=lv2_descriptor_2 plugins/organ/organ.o
	$(OBJCOPY) --redefine-sym  lv2_descriptor=lv2_descriptor_3 plugins/sawtooth/sawtooth.o

