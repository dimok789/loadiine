PATH := $(DEVKITPPC)/bin:$(PATH)
CC=powerpc-eabi-gcc
CFLAGS=-std=gnu99 -nostdinc -fno-builtin -c
LD=powerpc-eabi-ld
LDFLAGS=-Ttext 1800000 --oformat binary
OBJDUMP ?= $(PREFIX)objdump
project	:=	.
root:=$(CURDIR)
build	:=	 $(root)/bin
www_loadiine :=$(root)/../../www/loadiine
framework:=$(root)/../../../framework

all: clean setup menu loader fs main532 print_stats

setup:
	mkdir -p $(root)/bin/

menu:
	cd ../menu/ && make

loader:
	cd ../loader/ && make

fs:
	cd ../fs/ && make

main532:
	$(CC) $(CFLAGS) -DVER=532 $(project)/launcher.c
	#-Wa,-a,-ad
	cp -r $(root)/*.o $(build)
	rm $(root)/*.o
	$(LD) $(LDFLAGS) -s -o $(build)/code532.bin $(build)/launcher.o `find $(build) -name "*.o" ! -name "launcher.o"`
	cp -rf $(build)/*.elf $(www_loadiine)

clean:
	make -C ../menu/ clean
	make -C ../loader/ clean
	make -C ../fs/ clean
	rm -rf $(build)

print_stats:
	@echo
	@echo "code size : menu   =>" `$(OBJDUMP) -h $(build)/menu532.elf | awk '/.text|rodata/ { sum+=strtonum("0x"$$3) } END {print sum}'` / 5120
	@echo "code size : loader =>" `$(OBJDUMP) -h $(build)/loader532.elf | awk '/.text/ { sum+=strtonum("0x"$$3) } END {print sum}'` / 5120
	@echo "code size : fs     =>" `$(OBJDUMP) -h $(build)/fs532.elf | awk '/.text|rodata|magic/ { sum+=strtonum("0x"$$3) } END {print sum}'` / 16364
