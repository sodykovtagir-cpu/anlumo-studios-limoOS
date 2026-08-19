/* =============================================================================
 * linker.ld — links the LimoOS kernel image
 *
 * The kernel is loaded by GRUB at the conventional 1MiB physical address
 * (below 1MiB is reserved for real-mode/BIOS structures, VGA memory, etc).
 * This is a flat, non-higher-half layout — simplest possible for a first
 * boot. Migrating to a higher-half kernel later only requires changing
 * this script and adding a paging trampoline in boot.asm.
 * ========================================================================== */

ENTRY(_start)

KERNEL_VMA = 1M;

SECTIONS
{
    . = KERNEL_VMA;

    kernel_start = .;

    /* Multiboot header must land early in the image */
    .multiboot ALIGN(4K) :
    {
        *(.multiboot)
    }

    .text ALIGN(4K) :
    {
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data ALIGN(4K) :
    {
        *(.data)
        *(.data.*)
    }

    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    kernel_end = .;

    /* Discard sections we don't need in the final image */
    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
    }
}
