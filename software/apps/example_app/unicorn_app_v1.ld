/* unicorn_app_v1.ld */

__flash = 0x00010000;
__flash_size = 0K; /* increase if ever needed */
__ram = __flash + __flash_size;
__ram_size = 1M - 32K; /* subtract bootloader ram + IRQ table*/

__stack_size = 8192;

INCLUDE picolibc_noflash.ld