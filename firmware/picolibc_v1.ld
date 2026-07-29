
__flash = 0x00000000;
__flash_size = 64K;
__ram = 0x00010000;
__ram_size = 1M - 1K; /* To account for the IRQ_TABLE */
__stack_size = 512;

IRQ_TABLE = 0x10FC00;
__stack = IRQ_TABLE;

INCLUDE picolibc.ld

SECTIONS
{
    /* 1KB interrupt table */
    .interrupt_table IRQ_TABLE (NOLOAD) : ALIGN(4) {
        . = . + 0x400;
    }
}