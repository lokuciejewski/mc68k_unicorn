/* unicorn_app_v1.ld */
INCLUDE picolibc.ld

SECTIONS
{
    . = 0;                      /* everything relative to 0 */

    .text : {
        *(.text*)
        *(.rodata*)
        *(.got*)                /* GOT must be present for PIE */
        *(.got.plt*)
    } > ram

    .data : {
        *(.data*)
    } > ram

    .bss : {
        *(.bss*)
        *(COMMON)
    } > ram

    /* symbols the bootloader / crt0 will use */
    _etext = .;
    _edata = .;
    _end   = .;

    /* stack grows downward from whatever the bootloader sets */
}