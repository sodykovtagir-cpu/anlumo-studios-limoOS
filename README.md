# LimoOS

A from-scratch i386 (32-bit x86) operating system, built with GRUB/Multiboot,
a custom `.lxe` executable format, and LVGL for the GUI.

## Directory structure

```
LimoOS/
├── Makefile              # builds kernel, .lxe apps, ISO; runs QEMU
├── boot/
│   ├── boot.asm            # multiboot header, stack setup, jump to kernel_main
│   ├── linker.ld            # links the kernel ELF at the 1MiB mark
│   ├── gdt_flush.asm         # loads the GDTR + reloads segment registers
│   ├── idt_flush.asm          # loads the IDTR
│   ├── isr.asm                 # entry stubs for CPU exceptions (vectors 0-31)
│   └── irq.asm                  # entry stubs for hardware IRQs (vectors 32-47)
├── kernel/
│   ├── kernel.c              # kernel_main() — entry from boot.asm
│   ├── gdt.c/.h                # flat-model Global Descriptor Table
│   ├── idt.c/.h                 # Interrupt Descriptor Table (256 vectors)
│   ├── isr.c/.h                  # CPU exception dispatcher (fatal — no recovery model yet)
│   ├── irq.c                      # hardware IRQ dispatcher + handler registry
│   ├── lxe_loader.c/.h             # load_lxe(): validates + loads .lxe binaries
│   └── syscall.c/.h                 # (next) int 0x80 dispatcher
├── drivers/
│   ├── vga/vga.c/.h           # text-mode console (bring-up only)
│   ├── pic/pic.c/.h            # 8259 PIC remap (mandatory before sti) + EOI
│   ├── keyboard/keyboard.c/.h   # PS/2 keyboard, scan set 1, US QWERTY, IRQ1
│   ├── mouse/mouse.c/.h          # PS/2 mouse, 3-byte packets, IRQ12
│   ├── vesa/vesa.c/.h             # linear framebuffer (fed by GRUB's video mode)
│   │   ├── font8x16.h               # generated 8x16 bitmap font, ASCII 0x20-0x7E
│   │   └── text.c/.h                 # draws font8x16 glyphs onto the framebuffer
│   └── pit/pit.c/.h               # PIT timer, IRQ0, 100Hz tick counter
├── fs/                     # (next) virtual file system
├── include/
│   ├── lxe.h                 # shared LXE1 header struct (kernel + tools)
│   └── io.h                   # inb/outb/io_wait port I/O helpers
├── kernel/console.c/.h      # routes chars to VGA text or the VESA terminal
├── kernel/pmm.c/.h          # bitmap physical frame allocator
├── kernel/multiboot.h       # Multiboot 1 info struct (memory + framebuffer fields)
├── lxe/
│   ├── linker_lxe.ld       # links userland apps for .lxe packaging
│   └── tools/mklxe.c        # host tool: wraps text/data blobs in an LXE1 header
├── userland/
│   └── hello.c              # smoke-test .lxe app
└── iso/boot/grub/grub.cfg  # GRUB menu for the bootable ISO
```

Folders marked "(next)" are the natural next milestones and are not created
by this scaffold — add them as you build each subsystem, following the same
one-driver-per-directory / one-concern-per-file pattern already in place.

## Toolchain setup

You need an `i686-elf` cross compiler — never use your host's native gcc to
build kernel/userland code, since it targets the wrong platform/ABI and will
produce a kernel that assumes the host OS is present. Build one with
[OSDev's cross-compiler guide](https://wiki.osdev.org/GCC_Cross-Compiler), or
install via your package manager if one is offered (e.g.
`brew install i686-elf-gcc i686-elf-binutils` on macOS).

Also required: `nasm`, `grub-mkrescue` (from `grub-pc-bin`/`grub2` +
`xorriso`), and `qemu-system-i386`.

## Building & running

```sh
make            # builds build/limoos.kernel and build/userland/*.lxe
make iso        # packages limoos.iso with GRUB
make run        # build + boot in QEMU via the ISO (realistic boot path)
make run-kernel # build + boot the raw kernel directly in QEMU (fast iteration)
make clean      # remove all build output
```

A successful `make run` should show:

```
LimoOS kernel booted successfully.
Multiboot signature verified.
GDT installed.
IDT installed.
PIC remapped (IRQ0-15 -> int 32-47).
Keyboard driver registered (IRQ1).
Mouse driver registered (IRQ12).
Interrupts enabled.
```

**From here, the boot log depends on whether GRUB granted the framebuffer
request:**

- **Framebuffer granted** (the common case in QEMU): the screen switches
  to graphics before `kernel_main` even runs. All the VGA text lines above
  are still executed but are no longer visible — the only visible
  confirmation is a color-bar test pattern plus a bordered orange
  rectangle, drawn by `vesa_draw_test_pattern()` in `kernel.c`. There is
  no bitmap font yet, so no text renders on top of it.
- **Framebuffer declined**: a yellow line prints — "VESA framebuffer
  unavailable — staying in VGA text mode." — and the rest of the text
  boot log (keyboard/mouse "try it" prompts) continues as before.

Typing on the keyboard should echo characters directly to the screen —
letters, digits, punctuation, Shift for uppercase/symbols, Enter, Backspace,
Tab all work. Function keys, arrows, and the numpad are recognized but not
mapped yet (see `drivers/keyboard/keyboard.c`). **Only visible in the
text-mode fallback branch above**, until the framebuffer console gets a font.

Left-clicking should print "Mouse click at (x, y)" — same caveat: only
visible in the fallback branch. `mouse_set_bounds()` is called with the
real framebuffer resolution the moment `vesa_init()` succeeds, so
`mouse_get_state()` already reports coordinates in framebuffer pixel space
either way.

## Physical memory allocator

`kernel/pmm.c` is a bitmap frame allocator (one bit per 4K frame), built
from the Multiboot memory map (`mbi->mmap_addr`, falling back to
`mem_upper` if a bootloader doesn't provide the full map). Everything not
explicitly reported as available RAM — reserved regions, ACPI/NVS, the
gap below 1MiB — stays permanently unavailable by default, and
`pmm_init()` additionally force-protects `[0, kernel_end)` so the
allocator can never hand out memory the kernel itself occupies.

- `pmm_alloc_frame()` / `pmm_free_frame()` — any free frame / release one.
- `pmm_reserve_region(addr, size)` — reserve a *specific* physical range,
  failing cleanly if any of it is already in use. This is what
  `load_lxe()` uses for its fixed `LXE_LOAD_BASE`, rather than picking an
  arbitrary address `pmm_alloc_frame()` would offer (see the LXE section
  below for why the address can't just be "any free frame" yet).

The bitmap is a fixed 128KiB array sized for the full 4GiB address space
this kernel can address, rather than dynamically placed after
`kernel_end` — simple and always correct, at the cost of reserving that
much BSS unconditionally. Worth revisiting once BSS size actually matters.

## The LXE format

See `include/lxe.h` for the authoritative header layout. In short: a 32-byte
header (`LXE1` magic + entry point + section offsets/sizes) followed
directly by raw `.text` (with `.rodata` folded in) and `.data` bytes;
`.bss` is never stored on disk, only its size, so `load_lxe()` can
zero-fill it at load time.

Current constraints, deliberate for this stage of the project:
- Every `.lxe` app is linked against the same fixed `LXE_LOAD_BASE`
  (`0x00400000`) and is **not** position-independent — `load_lxe()` must
  place it at exactly that address. A physical allocator (see below)
  doesn't remove this constraint — it just makes the loader correctly
  *reserve* that fixed address instead of blindly assuming it's free.
- No free-on-exit path yet — there's no process-teardown concept, so a
  second `load_lxe()` call always fails while the first is still
  "running" (nothing currently returns control to the kernel after a
  binary's entry point runs anyway).
- No ring 3 separation yet — loaded code runs at the kernel's privilege
  level until the GDT gains user segments (already reserved as selectors
  0x18/0x20) and a TSS for the ring transition itself.

These are flagged with `TODO`/limitations comments at the top of
`kernel/lxe_loader.c`.

## Interrupts (GDT/IDT/PIC/IRQ)

- `gdt_init()` replaces GRUB's GDT with a flat-model table: 5 entries
  (null, kernel code, kernel data, and reserved user code/data for future
  ring3 use).
- `idt_init()` fills all 256 vectors: 0-31 for CPU exceptions (`isr.c`),
  32-47 for hardware IRQs after the PIC remap (`irq.c`). Un-set vectors
  stay "not present" on purpose — an unexpected interrupt should fault
  loudly, not silently no-op.
- `pic_remap()` must run before the first `sti`, or IRQ0-7 collide with
  CPU exception vectors 8-15.
- CPU exceptions are currently fatal (`isr.c` prints diagnostics and
  halts) — there's no per-process fault recovery until a process model
  exists.
- Hardware IRQs dispatch through `register_interrupt_handler()` — see
  `drivers/keyboard/keyboard.c` for the pattern a PIT timer driver should
  follow next.

## VESA framebuffer

`boot.asm`'s multiboot header now requests a linear graphics framebuffer
(1024x768x32 preferred — GRUB substitutes the closest mode it can grant).
**This is a real tradeoff, not a free upgrade**: GRUB performs the mode
switch itself before `_start` ever runs, so if it succeeds, VGA text mode
(`0xB8000`) is gone from the very first instruction of `kernel_main` —
every `vga_puts()` call still runs, but writes to memory nothing displays
anymore. `kernel.c` handles both outcomes explicitly (see the `vesa_init()`
branch in `kernel_main`), and `drivers/vesa/vesa.c` never assumes success —
callers must check its return value.

`vesa.c` provides `vesa_put_pixel()`, `vesa_fill_rect()`, `vesa_draw_rect()`,
and `vesa_clear()`, plus `vesa_pack_color()` which respects whatever
red/green/blue bit positions GRUB reports (this varies by hardware/
emulator — never assume `0xRRGGBB`). Only 24bpp and 32bpp RGB modes are
implemented; indexed-color and EGA-text fallback modes cause `vesa_init()`
to return 0.

**No bitmap font exists yet** — the only current visible output in
graphics mode is `vesa_draw_test_pattern()`'s color bars and bordered box.
Text (kernel logs, and eventually the keyboard echo) needs either a small
embedded font for direct framebuffer text, or waits until LVGL is wired up
and can render its own. Both are reasonable next steps depending on how
soon you want visible framebuffer diagnostics back versus going straight
for LVGL.

## GUI (LVGL)

**Scaffolding is in place; the library itself is not vendored in** —
network access wasn't available while building this, so `lvgl/` is an
empty placeholder directory. To turn it on:

```sh
git clone --branch release/v9.2 https://github.com/lvgl/lvgl.git lvgl
make        # auto-detects lvgl/lvgl.h and compiles LVGL in
make run
```

No other step needed — `Makefile` detects `lvgl/lvgl.h`, adds `-DLIMOOS_HAVE_LVGL`
plus `-I.` (for `lv_conf.h` at the project root) and `-Ilvgl`, globs every
`lvgl/src/**/*.c`, and switches `kernel_main` from the hand-rolled desktop
mockup to `lv_port_init()`. Without `lvgl/`, `make` builds exactly as
before — nothing regresses.

**Status**: confirmed working against a real LVGL v9.2 checkout —
`lv_display_create`/`lv_indev_create` (v9's registration API) compiled
and linked cleanly. The pixel format bug from the first version of this
port (casting `px_map` to `lv_color_t*`, which is only 3 bytes/pixel
with no alpha channel) is fixed: the display format is now set
explicitly via `lv_display_set_color_format(disp, LV_COLOR_FORMAT_XRGB8888)`,
and `my_disp_flush()` casts to `lv_color32_t*` (4 bytes/pixel,
blue/green/red/alpha) to match. If you're building against LVGL v8.x
instead of v9.x, the driver registration calls use the older
`lv_disp_drv_t`/`lv_indev_drv_t` static-struct pattern — everything else
in the port layer (pixel loop bounds, `mouse_get_state()` mapping,
PIT-based tick pacing) is version-independent.

`lv_conf.h` at the project root is a trimmed config (LVGL's own internal
allocator since there's no `kmalloc()` yet, only `LV_USE_LABEL`/`LV_USE_BTN`/
`LV_USE_WIN` enabled) — expand it as the UI grows past a single label.

**Mouse cursor**: `lv_port_init()` creates a small white-filled, black-
bordered circle and registers it via `lv_indev_set_cursor()` — LVGL
repositions it to the indev's reported point automatically. Untested
against real headers like everything else here, but built from fairly
stable, version-independent LVGL calls (`lv_obj_create`,
`lv_obj_set_style_*`, `lv_color_white()`/`lv_color_black()`). Swap it for
`lv_image_create()` plus a real cursor bitmap once one exists.
