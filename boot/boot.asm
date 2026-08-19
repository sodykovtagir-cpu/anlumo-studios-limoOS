; =============================================================================
; boot.asm — LimoOS entry point
; Provides the Multiboot header GRUB looks for, sets up a stack, and hands
; off to kernel_main() in kernel.c.
; =============================================================================

MBALIGN     equ  1 << 0              ; align loaded modules on page boundaries
MEMINFO     equ  1 << 1              ; provide memory map
VIDEOINFO   equ  1 << 2              ; request a specific video mode
MBFLAGS     equ  MBALIGN | MEMINFO | VIDEOINFO
MAGIC       equ  0x1BADB002          ; multiboot magic number
CHECKSUM    equ -(MAGIC + MBFLAGS)   ; must make the header checksum to 0

; -----------------------------------------------------------------------------
; Multiboot header — must be within the first 8KiB of the kernel image and
; 4-byte aligned. GRUB scans for this to recognize LimoOS as a bootable kernel.
; -----------------------------------------------------------------------------
section .multiboot
align 4
    dd MAGIC
    dd MBFLAGS
    dd CHECKSUM

    ; Video mode request fields (present because VIDEOINFO is set above).
    ; GRUB switches to this mode BEFORE jumping to _start — by the time
    ; kernel_main runs, VGA text mode (0xB8000) is already gone if this
    ; succeeded. See drivers/vesa/vesa.c and kernel.c for how the kernel
    ; detects whether GRUB actually honored this request.
    dd 0        ; mode_type: 0 = linear graphics framebuffer (not text)
    dd 1024     ; width  (preference — GRUB may substitute the closest match)
    dd 768      ; height
    dd 32       ; depth (bits per pixel)

; -----------------------------------------------------------------------------
; Stack — 16 KiB, 16-byte aligned as required by the System V i386 ABI on
; entry to any C function.
; -----------------------------------------------------------------------------
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

; -----------------------------------------------------------------------------
; Entry point — GRUB jumps here in 32-bit protected mode with paging
; disabled. eax holds the multiboot magic, ebx a pointer to the multiboot
; info structure. Both are passed through to kernel_main.
; -----------------------------------------------------------------------------
section .text
global _start
extern kernel_main

_start:
    cli                     ; interrupts stay off until IDT is installed
    mov esp, stack_top      ; set up the stack

    push ebx                ; multiboot info pointer -> kernel_main arg 2
    push eax                ; multiboot magic         -> kernel_main arg 1
    call kernel_main

    ; kernel_main should never return, but if it does, halt forever
    cli
.hang:
    hlt
    jmp .hang

; Mark the stack as non-executable so the linker doesn't warn.
section .note.GNU-stack noalloc noexec nowrite progbits
