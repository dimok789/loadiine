PATH := $(DEVKITPPC)/bin:$(PATH)
CC=powerpc-eabi-gcc
AS=powerpc-eabi-as
CFLAGS=-std=gnu99 -nostdinc -fno-builtin -c
LD=powerpc-eabi-ld
LDFLAGS=-Ttext 1800000 --oformat binary
OBJDUMP ?= $(PREFIX)objdump
project	:=	.
root:=$(CURDIR)
build	:=	 $(root)/bin
www_loadiine :=$(root)/../../www/loadiine
framework:=$(root)/../../../framework

all: clean setup main main532 print_stats

setup:
	mkdir -p $(root)/bin/

main:
	cd ../ && make clean
	cd ../ && make

main532:
	$(CC) $(CFLAGS) -DVER=532 $(project)/launcher.c
	$(AS) -mregnames -o kernel_patches.o $(project)/kernel_patches.S 
	#-Wa,-a,-ad
	cp -r $(root)/*.o $(build)
	rm $(root)/*.o
	$(LD) $(LDFLAGS) -s -o $(build)/code532.bin $(build)/launcher.o `find $(build) -name "*.o" ! -name "launcher.o"`
	cp -rf ../*.elf $(www_loadiine)
	cp -rf ../*.elf ../www/loadiine

clean:
	rm -rf $(build)

print_stats:
	@echo
	@echo "code size : loadiine   =>" `$(OBJDUMP) -h ../loadiine.elf | awk '/.kernel_code|.text|.menu_code|.rodata|.fs_magic|.loader_magic|.bss/ { sum+=strtonum("0x"$$3) } END {print sum}'` / 7530312
