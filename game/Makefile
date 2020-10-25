name := Thornmarked

all: $(name).n64
clean:
	rm -f $(objs) $(name).elf $(name).n64
.PHONY: all clean

CFLAGS := -O2 -std=c11
CFLAGS += -ffreestanding -mfix4300 -G 0 -I../sdk/ultra/include -D_FINALROM
CFLAGS += -Wall -Wextra -Wpointer-arith -Wwrite-strings -Wmissing-prototypes -Wdouble-promotion -Werror=implicit-function-declaration -Winit-self -Wstrict-prototypes
#CFLAGS += -Werror

AS := mips32-elf-as
CC := mips32-elf-gcc

CFLAGS += -O2 -D_FINALROM
ASFLAGS := -32 -mips3

objs := main.o start.o
$(name).elf: link.lds $(objs)
	$(CC) -nostdlib -o $@ -T link.lds $(objs) ../sdk/ultra/lib/libgultra_rom.a /opt/n64/lib/gcc/mips32-elf/10.2.0/libgcc.a

$(name).n64: $(name).elf
	mips32-elf-objcopy -O binary $< $@
	makemask $@
