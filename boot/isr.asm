; =============================================================================
; isr.asm — low-level entry stubs for CPU exceptions (interrupt vectors 0-31)
;
; The CPU pushes an error code automatically for exceptions 8, 10-14, and 17;
; for the rest we push a dummy 0 so every stub hands isr_common_stub an
; identical stack shape regardless of which vector fired.
; =============================================================================

section .text

%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push dword 0        ; dummy error code
    push dword %1        ; interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    cli                  ; CPU already pushed a real error code
    push dword %1
    jmp isr_common_stub
%endmacro

ISR_NOERR 0   ; Divide-by-zero
ISR_NOERR 1   ; Debug
ISR_NOERR 2   ; Non-maskable interrupt
ISR_NOERR 3   ; Breakpoint
ISR_NOERR 4   ; Overflow
ISR_NOERR 5   ; Bound range exceeded
ISR_NOERR 6   ; Invalid opcode
ISR_NOERR 7   ; Device not available
ISR_ERR   8   ; Double fault
ISR_NOERR 9   ; Coprocessor segment overrun (legacy)
ISR_ERR   10  ; Invalid TSS
ISR_ERR   11  ; Segment not present
ISR_ERR   12  ; Stack-segment fault
ISR_ERR   13  ; General protection fault
ISR_ERR   14  ; Page fault
ISR_NOERR 15  ; Reserved
ISR_NOERR 16  ; x87 floating-point exception
ISR_ERR   17  ; Alignment check
ISR_NOERR 18  ; Machine check
ISR_NOERR 19  ; SIMD floating-point exception
ISR_NOERR 20  ; Virtualization exception
ISR_ERR   21  ; Control protection exception
ISR_NOERR 22  ; Reserved
ISR_NOERR 23  ; Reserved
ISR_NOERR 24  ; Reserved
ISR_NOERR 25  ; Reserved
ISR_NOERR 26  ; Reserved
ISR_NOERR 27  ; Reserved
ISR_NOERR 28  ; Hypervisor injection exception
ISR_ERR   29  ; VMM communication exception
ISR_ERR   30  ; Security exception
ISR_NOERR 31  ; Reserved

extern isr_handler

; Common stub: save full register state, switch to kernel data segments,
; call the C dispatcher with a pointer to the saved state, then restore
; everything and return via iret.
isr_common_stub:
    pusha                 ; edi,esi,ebp,esp,ebx,edx,ecx,eax

    mov ax, ds
    push eax              ; save the original data segment

    mov ax, 0x10           ; kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp               ; pass pointer to the whole saved-state struct
    call isr_handler
    add esp, 4

    pop eax                 ; restore original data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8               ; drop the error code + interrupt number
    iret                      ; restores EFLAGS (and thus IF) from the stack

section .note.GNU-stack noalloc noexec nowrite progbits
