; =============================================================================
; luext_icon.asm — embeds assets/luext_icon.bin (24x24 RGBA lemon logo)
; ============================================================================

section .rodata
align 4

global luext_icon_data
global luext_icon_data_end

luext_icon_data:
    incbin "assets/luext_icon.bin"
luext_icon_data_end:

section .note.GNU-stack noalloc noexec nowrite progbits
