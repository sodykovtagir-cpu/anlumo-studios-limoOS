; =============================================================================
; wallpaper.asm — embeds assets/wallpaper.bin directly into the kernel image
;
; incbin is the standard way to embed a large binary blob without going
; through a C initializer list — 4MB of individual byte literals would be
; painfully slow to compile (and a nightmare to read). The bytes are raw
; XRGB8888 pixels (red,green,blue,alpha per pixel, confirmed empirically —
; see kernel/wallpaper.c) at 1280x800, matching this project's detected
; VESA resolution.
; =============================================================================

section .rodata
align 4

global wallpaper_data
global wallpaper_data_end

wallpaper_data:
    incbin "assets/wallpaper.bin"
wallpaper_data_end:

section .note.GNU-stack noalloc noexec nowrite progbits
