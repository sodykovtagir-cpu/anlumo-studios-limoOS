; =============================================================================
; idt_flush.asm — loads the IDTR
; =============================================================================

section .text
global idt_flush

; void idt_flush(uint32_t idt_ptr_addr);
idt_flush:
    mov eax, [esp + 4]
    lidt [eax]
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
