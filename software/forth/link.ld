ENTRY(_start)

MEMORY
{
    ROM (rx)  : ORIGIN = 0x0000, LENGTH = 4K
    RAM (rwx) : ORIGIN = 0x1000, LENGTH = 12K
}

SECTIONS
{
    /* Code */
    .text 0x1000:
    {
        *(.text*)
        *(.rodata*)
    } > RAM

    .data :
    {
        _data_start = .;
        *(.data*)
        _data_end = .;
    } > RAM

    .bss :
    {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        _bss_end = .;
    } > RAM

    /* Stack at top of RAM */
    _stack_top = ORIGIN(RAM) + LENGTH(RAM) - 36;
}
