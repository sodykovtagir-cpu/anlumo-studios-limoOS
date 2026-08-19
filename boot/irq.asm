; =============================================================================
; irq.asm — low-level entry stubs for hardware IRQs (interrupt vectors 32-47)
;
; Mirrors isr.asm's structure exactly, but hardware IRQs never push an error
; code, so every stub pushes a dummy 0. The "interrupt number" pushed here
; is the remapped vector (32-47), same convention as isr.asm, so both share
; one registers_t struct — irq_handler in irq.c subtracts 32 to get the
; raw IRQ line (0-15) for dispatch and PIC EOI.
; =============================================================================

section .text

%macro IRQ_STUB 2 ; %1 = IRQ line (0-15), %2 = remapped vector (32-47)
global irq%1
irq%1:
    cli
    push dword 0    ; dummy error code, keeps the stack shape uniform with isr.asm
    push dword %2
    jmp irq_common_stub
%endmacro

IRQ_STUB 0,  32   ; PIT timer
IRQ_STUB 1,  33   ; Keyboard
IRQ_STUB 2,  34   ; Cascade (never fires directly — slave PIC uses this line)
IRQ_STUB 3,  35   ; COM2
IRQ_STUB 4,  36   ; COM1
IRQ_STUB 5,  37   ; LPT2
IRQ_STUB 6,  38   ; Floppy disk
IRQ_STUB 7,  39   ; LPT1 / spurious
IRQ_STUB 8,  40   ; RTC
IRQ_STUB 9,  41   ; Free / redirected
IRQ_STUB 10, 42   ; Free
IRQ_STUB 11, 43   ; Free
IRQ_STUB 12, 44   ; PS/2 mouse
IRQ_STUB 13, 45   ; FPU / coprocessor
IRQ_STUB 14, 46   ; Primary ATA
IRQ_STUB 15, 47   ; Secondary ATA

extern irq_handler

irq_common_stub:
    pusha

    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8
    iret

section .note.GNU-stack noalloc noexec nowrite progbits
