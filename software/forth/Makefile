# Toolchain
CC      = tms9900-gcc
AS      = tms9900-gcc
OBJCOPY = tms9900-objcopy
OBJDUMP = tms9900-objdump

# Flags
CFLAGS  = -ffreestanding -fno-builtin -Os -fomit-frame-pointer -Iinclude
ASFLAGS = 
LDFLAGS = -nostdlib -nostartfiles -T link.ld -Wl,-Map=out.map

# Targets
all: out.rom main.s out.lst

# Link
out.elf: crt0.o main.o zforth.o include/lib_c.o include/lib_a.o link.ld Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) crt0.o main.o include/lib_c.o include/lib_a.o zforth.o -o $@

# Raw ROM image
out.rom: out.elf Makefile
	$(OBJCOPY) -O binary $< $@

# Disassembly
out.lst: out.elf Makefile
	$(OBJDUMP) -D $< > $@

# Startup assembly
crt0.o: crt0.a99 Makefile
	$(AS) $(ASFLAGS) -x assembler -c $< -o $@

# Compile C to object
main.o: main.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

zforth.o: zforth.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@
	
include/lib_c.o: include/lib.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

include/lib_a.o: include/lib.a99 Makefile
	$(AS) $(ASFLAGS) -x assembler -c $< -o $@

# ALSO emit compiler-generated assembly
main.s: main.c Makefile
	$(CC) $(CFLAGS) -S $< -o $@

clean:
	rm -f *.o *.elf *.rom *.lst main.s *.map
