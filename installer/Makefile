PATH := $(DEVKITPPC)/bin:$(PATH)
CC=powerpc-eabi-gcc
CFLAGS=-std=gnu99 -nostdinc -fno-builtin -c
LD=powerpc-eabi-ld
LDFLAGS=-Ttext 1800000 --oformat binary
project	:=	.
root:=$(CURDIR)
build	:=	 $(root)/bin
www :=$(root)/../../../www
framework:=$(root)/../../../framework

all: clean setup menu loader fs main532

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

clean:
	make -C ../menu/ clean
	make -C ../loader/ clean
	make -C ../fs/ clean
	rm -rf $(build)
