
PATH := $(DEVKITPPC)/bin:$(PATH)

PREFIX ?= powerpc-eabi-
LD := $(PREFIX)ld
AS := $(PREFIX)as
CC := $(PREFIX)gcc
OBJDUMP ?= $(PREFIX)objdump
OBJCOPY ?= $(PREFIX)objcopy

SFLAGS := -mgekko -mregnames

# -O2: optimise lots
# -Wall: generate lots of warnings
# -x c: compile as C code
# -std=gnu99: use the C99 standard with GNU extensions
# -ffreestanding: we don't have libc; don't expect we do
# -mrvl: enable wii/gamecube compilation
# -mcpu=750: enable processor specific compilation
# -meabi: enable eabi specific compilation
# -mhard-float: enable hardware floating point instructions
# -fshort-wchar: use 16 bit whcar_t type in keeping with Wii executables
# -msdata-none: do not use r2 or r13 as small data areas
# -memb: enable embedded application specific compilation
# -ffunction-sections: split up functions so linker can garbage collect
# -fdata-sections: split up data so linker can garbage collect
#CFLAGS := -O2 -Wall -x c -std=gnu99 \

# optimizing to -Os gives still disc error on some games like mario kart 8 and mario maker (even though they dont work anyway)
# gotta check what else the compiler is optimizing (memory realignment?)
# so for now i reverted it as there is still enough space in payload
# adding it back in will give another 600 - 1000 bytes of space
# CFLAGS := -Os -nostdinc -nostdlib -Wall -x c -std=gnu99
CFLAGS := -O0 -nostdinc -nostdlib -Wall -x c -std=gnu99 \
          -ffreestanding \
          -mrvl -mcpu=750 -meabi -mhard-float -fshort-wchar \
          -msdata=none -memb -ffunction-sections -fdata-sections  \
          -Wno-unknown-pragmas -Wno-strict-aliasing

SRC := $(wildcard *.S) $(wildcard *.c)
OBJ := $(patsubst %.S,build/%.o,$(patsubst %.c,build/%.o,$(SRC)))

# Simulate an order only dependency
BUILD_REQ := $(filter-out $(wildcard build),build)
BIN_REQ := $(filter-out $(wildcard bin),bin)

all: bin/loader532.h copy

bin/loader%.h: build/loader%.text.bin build/loader%.magic.bin $(BIN_REQ)
	xxd -i build/loader$*.magic.bin | sed "s/unsigned/static const unsigned/g;s/loader$*/loader/g;s/build_//g" > $@
	xxd -i build/loader$*.text.bin | sed "s/unsigned/static const unsigned/g;s/loader$*/loader/g;s/build_//g" >> $@

build/loader%.text.bin: build/loader%.elf $(BUILD_REQ)
	$(OBJCOPY) -j .text -O binary $< $@
build/loader%.magic.bin: build/loader%.elf $(BUILD_REQ)
	$(OBJCOPY) -j .magic -O binary $< $@

build/loader%.elf: loader%.ld $(OBJ) $(BUILD_REQ)
	$(LD) -T $< $(OBJ)
#	$(LD) -T $< $(OBJ) -s -L"$(DEVKITPPC)/lib/gcc/powerpc-eabi/4.8.2" -lgcc

build/%.o: %.c $(BUILD_REQ)
	$(CC) -c $(CFLAGS) -o $@ $<
build/%.o: %.S $(BUILD_REQ)
	$(AS) $(SFLAGS) -o $@ $<

bin:
	mkdir $@
build:
	mkdir $@

copy:
	cp bin/loader532.h ../installer/
#	cp -rf build/ build2/ # only for debuging asm with objdump, delete me

clean:
	rm -rf $(wildcard build) $(wildcard bin) $(wildcard ../installer/loader532.h)
