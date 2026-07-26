ENTRY(_start)

MEMORY
{
    ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 0x10000   /* 64 KB ROM */
    RAM (rwx) : ORIGIN = 0x00010000, LENGTH = 0x100000  /* 1 MB - 64 KB */
}

SECTIONS
{
    /* Vector table at address 0 */
    .vectors 0x00000000 : {
        KEEP(*(.vectors));
    } > ROM

    /* Code section */
    .text 0x00000100 : ALIGN(4) {
        *(.text*);
    } > ROM

    /* Read-only data */
    .rodata :  ALIGN(4) {
        *(.rodata*);
    } > ROM

    /* Initialized data section */
    .data :  ALIGN(4) {
        *(.data*);
        *(.data.*);
    } > ROM

    /* BSS section - goes to RAM */
    .bss (NOLOAD) :  ALIGN(4) {
        *(.bss*);
        *(COMMON);
    } > RAM

    /* Catch-all for anything that might fall through */
    .other : ALIGN(4) {
        *(*);
    } > ROM


    IRQ_TABLE = 0x10FC00;

    /* Define SV_STACK_TOP symbol */
    SV_STACK_TOP = IRQ_TABLE - 4;
    
    /* Define USER_STACK_TOP symbol */
    USER_STACK_TOP = SV_STACK_TOP - 1024;

    /* 1KB interrupt table */
    .interrupt_table IRQ_TABLE : ALIGN(4) {
        *(.irq*);
    } > RAM
}