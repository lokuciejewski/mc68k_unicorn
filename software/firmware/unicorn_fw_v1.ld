/* unicorn_fw_v1.ld */

IRQ_TABLE = 0x10FC00;

__flash = 0x00000000;
__flash_size = 64K;
__ram_size = 32K - 1K; /* To account for the IRQ_TABLE */
__ram = IRQ_TABLE - __ram_size;
__stack_size = 512;

__stack = IRQ_TABLE;

INCLUDE picolibc.ld

SECTIONS
{
    /* 1KB interrupt table */
    .interrupt_table IRQ_TABLE (NOLOAD) : ALIGN(4) {
        . = . + 0x400;
    }
}