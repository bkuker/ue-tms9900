# Toolchain
CC      = tms9900-gcc
AS      = tms9900-gcc
OBJCOPY = tms9900-objcopy
OBJDUMP = tms9900-objdump

# Flags
CFLAGS  = -ffreestanding -fno-builtin -O1 -fomit-frame-pointer
ASFLAGS = 
LDFLAGS = -nostdlib -nostartfiles -T link.ld -Wl,-Map=out.map

# Targets
all: out.rom main.s out.lst

# Link
out.elf: crt0.o main.o link.ld Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) crt0.o main.o -o $@

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

# ALSO emit compiler-generated assembly
main.s: main.c Makefile
	$(CC) $(CFLAGS) -S $< -o $@

clean:
	rm -f *.o *.elf *.rom *.lst main.s *.map
