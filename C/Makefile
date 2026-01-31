# Toolchain
CC      = tms9900-gcc
AS      = tms9900-gcc
OBJCOPY = tms9900-objcopy
OBJDUMP = tms9900-objdump

# Flags
CFLAGS  = -ffreestanding -fno-builtin -Os -fomit-frame-pointer
ASFLAGS = 
LDFLAGS = -nostdlib -nostartfiles -T link.ld -Wl,-Map=out.map

# Targets
all: out.rom main.s out.lst

# Link
out.elf: crt0.o main.o link.ld
	$(CC) $(CFLAGS) $(LDFLAGS) crt0.o main.o -o $@

# Raw ROM image
out.rom: out.elf
	$(OBJCOPY) -O binary $< $@

# Disassembly
out.lst: out.elf
	$(OBJDUMP) -D $< > $@

# Startup assembly
crt0.o: crt0.s
	$(AS) $(ASFLAGS) -c $< -o $@

# Compile C to object
main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

# ALSO emit compiler-generated assembly
main.s: main.c
	$(CC) $(CFLAGS) -S $< -o $@

clean:
	rm -f *.o *.elf *.rom *.lst main.s *.map
