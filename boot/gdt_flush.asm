; =============================================================================
; gdt_flush.asm — loads the GDTR and reloads every segment register
;
; cs cannot be reloaded with a plain mov — only a far jump (or far ret/iret)
; changes it, which is why this has to be assembly rather than C.
; =============================================================================

section .text
global gdt_flush

; void gdt_flush(uint32_t gdt_ptr_addr);
gdt_flush:
    mov eax, [esp + 4]   ; first argument: pointer to the gdt_ptr struct
    lgdt [eax]

    mov ax, 0x10          ; kernel data selector (GDT_SEL_KERNEL_DATA)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush        ; far jump into the kernel code selector, which
.flush:                     ; reloads cs with the new descriptor
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
