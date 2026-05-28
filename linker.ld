/* (c) 2025 Anlumo Studios */
ENTRY(_start)

SECTIONS
{
    . = 1M;

    .multiboot : { *(.multiboot) }
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}