CWD = $(shell pwd)
SHELL := /bin/bash
export LV2_PATH=$(CWD)/installed_plugins
export INSTALL_DIR=$(LV2_PATH)
export STATIC_LV2=1


ifeq ($(PLATFORM), LINUX)

PLATFORM_SRC = $(wildcard includes/linux/*.c)
all : pitracker_exe
CC = gcc
OBJDUMP = objdump
OBJCOPY = objcopy

else

PLATFORM = RASPBERRY_PI
AS = arm-none-eabi-as
export CC = arm-none-eabi-gcc
export LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump
OBJCOPY = arm-none-eabi-objcopy

EXTRA_CFLAGS = -I$(CWD)/includes/posix -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfloat-abi=hard -mfpu=vfp -ffast-math
AFLAGS = --warn --fatal-warnings -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -march=armv6
LIB = -L ../buildgcc/gnuarm/arm-none-eabi/lib/fpu/ -L ../buildgcc/gnuarm/lib/gcc/arm-none-eabi/4.7.2/fpu/


PLATFORM_SRC = $(wildcard includes/pi/*.c)

all : kernel_img

endif


export CFLAGS = -D$(PLATFORM) -I$(CWD)/includes -Wall -O2 -std=c99 $(EXTRA_CFLAGS)

PLUGIN_DIRS = $(wildcard plugins/*.lv2)
PLUGIN_OBJS = $(shell cat plugin_library_list.txt)

PLATFORM_OBJS = $(PLATFORM_SRC:.c=.o)

POSIX_SRC = $(wildcard includes/posix/*.c)
POSIX_OBJS = $(POSIX_SRC:.c=.o)


clean :
	rm -f *.o includes/*.o includes/*/*.o includes/*/*/*.o
	rm -f *.a includes/*/*.a includes/*/*/*.a
	rm -f *.bin
	rm -f *.elf
	rm -f kernel.img pitracker
	rm -f descriptors.h
	rm -rf installed_plugins 
	rm -f plugin_library_list.txt
	for dir in $(PLUGIN_DIRS); do \
	make -C $$dir clean; \
	done

pitracker.o : pitracker.c
	$(CC) $(CFLAGS) -c $^ -o $@

vectors.o : vectors.s
	$(AS) $(AFLAGS) $^ -o $@


EMMC_SRC = $(wildcard includes/pi/emmc/*.c)
EMMC_OBJS = $(EMMC_SRC:.c=.o) includes/pi/emmc/qmemcpy.o includes/pi/emmc/vectors.o

includes/pi/emmc/%.o: includes/pi/emmc/%.c
	$(CC) $(CFLAGS) -I$(CWD)/includes/pi/emmc -c $^ -o $@
includes/pi/emmc/%.o: includes/pi/emmc/%.S
	$(AS) $(AFLAGS) $^ -o $@
includes/pi/emmc/libemmc.a: $(EMMC_OBJS)
	ar rcs $@ $^


includes/%.o: includes/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

posix: $(POSIX_OBJS)

platform: $(PLATFORM_OBJS)

pitracker.elf : memmap vectors.o pitracker.o posix platform plugins includes/lv2.o includes/mf_read.o includes/pi/emmc/libemmc.a
	$(LD) vectors.o pitracker.o includes/lv2.o includes/mf_read.o includes/pi/emmc/libemmc.a $(PLUGIN_OBJS) $(PLATFORM_OBJS) $(POSIX_OBJS) -T memmap -o pitracker.elf $(LIB) -lc -lgcc -lm


# This is rotten and almost certainly broken:
pitracker_exe :  platform includes/lv2.o includes/mf_read.o plugins
	$(CC) $(CFLAGS) pitracker.c $(PLUGIN_OBJS) includes/lv2.o includes/mf_read.o $(PLATFORM_OBJS) -lm -lasound -o pitracker

kernel_img : pitracker.elf
	$(OBJCOPY) pitracker.elf -O binary kernel.img
	cp kernel.img SD_Card/


build_plugins :
	mkdir -p installed_plugins
	for dir in $(PLUGIN_DIRS); do \
	make -C $$dir install; \
	done

plugins : build_plugins staticize_plugins
#	$(OBJDUMP) -t plugins/piano.lv2/piano.o | grep lv2_descriptor

staticize_plugins : staticizer/staticizer.py
	./staticizer/staticizer.py

