all: thorn.n64
clean:
	rm -f codesegment.o main.o
.PHONY: all clean

CFLAGS := -O2 -std=c11
CFLAGS += -ffreestanding -mfix4300 -G 0 -I../sdk/ultra/include -D_FINALROM
CFLAGS += -Wall -Wextra -Wpointer-arith -Wwrite-strings -Wmissing-prototypes -Wdouble-promotion -Werror=implicit-function-declaration -Winit-self -Wstrict-prototypes
#CFLAGS += -Werror

CC := mips32-elf-gcc

CFLAGS += -O2 -D_FINALROM 

codesegment.o: main.o
	$(CC) -nostdlib -r -o $@ $^ ../sdk/ultra/lib/libgultra_rom.a /opt/n64/lib/gcc/mips32-elf/10.2.0/libgcc.a

thorn.n64: spec codesegment.o
	spicy -r $@ spec --toolchain-prefix=mips32-elf-
	makemask $@
