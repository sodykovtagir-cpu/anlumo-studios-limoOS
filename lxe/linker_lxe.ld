/* =============================================================================
 * linker_lxe.ld — links userland apps that will be packaged as .lxe binaries
 *
 * Produces a plain ELF with .text/.data/.bss laid out contiguously at a
 * fixed load address. The build then runs objcopy to pull .text and .data
 * out as flat binaries, and mklxe (see lxe/tools/mklxe.c) prepends the
 * LXE1 header and concatenates them into the final .lxe file. .bss is
 * never written to disk — only its size is recorded in the header so the
 * kernel loader can zero-fill it at load time.
 *
 * LXE_LOAD_BASE must match where load_lxe() places the binary in memory
 * (see kernel/lxe_loader.c). Every .lxe app is currently loaded at the
 * same fixed address — no relocation, no dynamic linking, single app at
 * a time. Revisit this once LimoOS gets per-process address spaces.
 * ========================================================================== */

ENTRY(_start)

LXE_LOAD_BASE = 0x00400000;

SECTIONS
{
    . = LXE_LOAD_BASE;

    .text ALIGN(4) :
    {
        __text_start = .;
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        __text_end = .;
    }

    .data ALIGN(4) :
    {
        __data_start = .;
        *(.data)
        *(.data.*)
        __data_end = .;
    }

    .bss ALIGN(4) :
    {
        __bss_start = .;
        *(COMMON)
        *(.bss)
        *(.bss.*)
        __bss_end = .;
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
    }
}
