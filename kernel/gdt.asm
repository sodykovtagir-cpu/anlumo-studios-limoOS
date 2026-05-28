; (c) 2025 Anlumo Studios

global gdt_load

section .text

gdt_load:

    mov eax, [esp + 4]

    lgdt [eax]

    mov ax, 0x10

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush

.flush:
    ret
