ENTRY(_start)

MEMORY
{
    ROM (rx)  : ORIGIN = 0x0000, LENGTH = 4K
    RAM (rwx) : ORIGIN = 0x1000, LENGTH = 12K
}

SECTIONS
{
    /* Reset vectors (WP + PC) */
    .vectors 0x0000 :
    {
        KEEP(*(.vectors))
    } > ROM

    /* Code */
    .text 0x100:
    {
        *(.text*)
        *(.rodata*)
    } > ROM

    .workspace (NOLOAD) :
    {
        _workspace = .;
        . += 64;            /* reserve 64 bytes */
    } > RAM

    /* Initialized data (lives in RAM, initial image in ROM) */
    .data :
    {
        _data_start = .;
        *(.data*)
        _data_end = .;
    } > RAM AT > ROM

    _data_load = LOADADDR(.data);

    /* Zero-initialized data */
    .bss :
    {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        _bss_end = .;
    } > RAM

    /* Stack at top of RAM */
    _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
