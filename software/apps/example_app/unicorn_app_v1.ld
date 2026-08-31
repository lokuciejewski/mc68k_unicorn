/* unicorn_app_v1.ld */

__flash = 0x00010000;
__flash_size = 0K; /* increase if ever needed */
__ram = __flash + __flash_size;
/* This can be set up to 1M - 32K (bootloader + IRQ table) but it's best to be lower */
/* since picolibc treats all empty ram as heap by default, causing the bootloader to */
/* zero-out that memory and making the loading longer */
__ram_size = 128K - 32K;

__stack_size = 8192;

INCLUDE picolibc_noflash.ld