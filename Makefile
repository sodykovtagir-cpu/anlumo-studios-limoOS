# =============================================================================
# LimoOS top-level Makefile
#
# Targets:
#   make            - build kernel + userland .lxe apps
#   make iso        - build limoos.iso (bootable GRUB image)
#   make run        - build the ISO and launch it in QEMU
#   make run-kernel - launch the raw kernel binary in QEMU (faster, skips GRUB)
#   make clean      - remove all build artifacts
# =============================================================================

# ---- Toolchain ---------------------------------------------------------
CC      := i686-elf-gcc
LD      := i686-elf-gcc
OBJCOPY := i686-elf-objcopy
NM      := i686-elf-nm
NASM    := nasm
HOSTCC  := cc
GRUBMK  := grub-mkrescue
QEMU    := qemu-system-i386

# ---- Directories --------------------------------------------------------
BUILD   := build
ISO_DIR := iso

# ---- Flags ---------------------------------------------------------------
CFLAGS  := -std=gnu11 -ffreestanding -fno-pie -fno-stack-protector \
           -Wall -Wextra -Werror -m32 -O2 -g
LDFLAGS := -ffreestanding -O2 -nostdlib -m32
NASMFLAGS := -f elf32

# ---- Kernel sources -------------------------------------------------------
KERNEL_C_SRCS := kernel/kernel.c \
                 kernel/gdt.c \
                 kernel/idt.c \
                 kernel/isr.c \
                 kernel/irq.c \
                 kernel/lxe_loader.c \
                 kernel/console.c \
                 kernel/pmm.c \
                 fs/vfs.c \
                 drivers/vga/vga.c \
                 drivers/pic/pic.c \
                 drivers/keyboard/keyboard.c \
                 drivers/mouse/mouse.c \
                 drivers/vesa/vesa.c \
                 drivers/vesa/text.c \
                 drivers/pit/pit.c

KERNEL_ASM_SRCS := boot/boot.asm \
                    boot/gdt_flush.asm \
                    boot/idt_flush.asm \
                    boot/isr.asm \
                    boot/irq.asm

# ---- LVGL (optional) -------------------------------------------------------
# Detected automatically: clone/extract LVGL into ./lvgl (so ./lvgl/lvgl.h
# exists) and re-run make — no other step needed. Without it, the kernel
# builds exactly as before; kernel/lv_port.c and kernel.c's LVGL calls are
# guarded by #ifdef LIMOOS_HAVE_LVGL, which only gets defined here once
# lvgl.h is actually found.
LVGL_DIR := lvgl
LVGL_PRESENT := $(wildcard $(LVGL_DIR)/lvgl.h)

ifneq ($(LVGL_PRESENT),)
CFLAGS += -DLIMOOS_HAVE_LVGL -I. -I$(LVGL_DIR)
LVGL_C_SRCS := $(shell find $(LVGL_DIR)/src -name '*.c' 2>/dev/null)
KERNEL_C_SRCS += kernel/lv_port.c kernel/wallpaper.c kernel/shell.c kernel/window.c kernel/filemanager.c kernel/texteditor.c $(LVGL_C_SRCS)
KERNEL_ASM_SRCS += assets/wallpaper.asm assets/luext_icon.asm
endif

KERNEL_OBJS := $(patsubst %.c,$(BUILD)/%.o,$(KERNEL_C_SRCS)) \
               $(patsubst %.asm,$(BUILD)/%.o,$(KERNEL_ASM_SRCS))

KERNEL_BIN  := $(BUILD)/limoos.kernel
LINKER_SCRIPT := boot/linker.ld

# ---- LXE userland -----------------------------------------------------
LXE_LINKER_SCRIPT := lxe/linker_lxe.ld
MKLXE_TOOL := $(BUILD)/tools/mklxe

LXE_APPS := hello
LXE_BINS := $(patsubst %,$(BUILD)/userland/%.lxe,$(LXE_APPS))

# ---- ISO / QEMU ------------------------------------------------------
ISO_IMAGE := limoos.iso

.PHONY: all iso run run-kernel clean dirs check-lvgl-state

all: $(KERNEL_BIN) $(LXE_BINS)
	@if [ -n "$(LVGL_PRESENT)" ]; then \
	  echo "LVGL: found in ./lvgl, compiled in ($(words $(LVGL_C_SRCS)) source files)"; \
	else \
	  echo "LVGL: not found in ./lvgl - built without GUI (see README's LVGL section)"; \
	fi

# Make only recompiles a .o when its .c is newer — it has no idea CFLAGS
# changed (e.g. -DLIMOOS_HAVE_LVGL appearing because lvgl/ showed up).
# check-lvgl-state catches that by comparing LVGL's presence against what
# the last build recorded, wiping build/ on a mismatch. It's wired in as
# a prerequisite of `dirs` — which every object pattern rule below
# already depends on — so this runs before ANY compilation, no matter
# which target you invoke (make / make run / make iso / ...).
LVGL_STATE_FILE := $(BUILD)/.lvgl_state
check-lvgl-state:
	@mkdir -p $(BUILD)
	@CURRENT="$(if $(LVGL_PRESENT),1,0)"; \
	if [ -f $(LVGL_STATE_FILE) ] && [ "$$(cat $(LVGL_STATE_FILE))" != "$$CURRENT" ]; then \
	  echo "LVGL presence changed since the last build — cleaning build/ first..."; \
	  rm -rf $(BUILD); \
	  mkdir -p $(BUILD); \
	fi; \
	echo "$$CURRENT" > $(LVGL_STATE_FILE)

# ---- Directory bootstrap -------------------------------------------------
dirs: check-lvgl-state
	@mkdir -p $(BUILD)/boot $(BUILD)/kernel $(BUILD)/drivers/vga \
	          $(BUILD)/drivers/pic $(BUILD)/drivers/keyboard \
	          $(BUILD)/drivers/mouse $(BUILD)/drivers/vesa \
	          $(BUILD)/drivers/pit \
	          $(BUILD)/fs \
	          $(BUILD)/userland $(BUILD)/tools

# ---- Kernel object rules -------------------------------------------------
# @mkdir -p $(dir $@) handles arbitrary nested paths (needed once LVGL's
# lvgl/src/core/, lvgl/src/widgets/, etc. join the build — those aren't
# enumerated in dirs: above, so each object creates its own directory).
$(BUILD)/%.o: %.c | dirs
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/%.o: %.asm | dirs
	@mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

# LVGL is vendored third-party code — -Werror (and the strictness that
# goes with it) is for code we own, not for flagging every unused-
# parameter warning in a library we didn't write. Everything else
# (freestanding flags, -m32, optimization level) still applies.
$(BUILD)/$(LVGL_DIR)/%.o: CFLAGS := $(filter-out -Werror,$(CFLAGS))

$(KERNEL_BIN): $(KERNEL_OBJS) $(LINKER_SCRIPT) | dirs
	$(LD) $(LDFLAGS) -T $(LINKER_SCRIPT) -o $@ $(KERNEL_OBJS) -lgcc
	@echo "Verifying multiboot header is present..."
	@grub-file --is-x86-multiboot $@ && echo "  OK: $@ is multiboot-compliant" \
	  || (echo "  FAIL: multiboot header missing/misplaced" && exit 1)

# ---- Host-side LXE packaging tool ----------------------------------------
$(MKLXE_TOOL): lxe/tools/mklxe.c | dirs
	$(HOSTCC) -O2 -Wall -Wextra -o $@ $<

# ---- LXE app build pipeline: .c -> .elf -> text.bin/data.bin -> .lxe ---
$(BUILD)/userland/%.elf: userland/%.c $(LXE_LINKER_SCRIPT) | dirs
	$(CC) $(CFLAGS) -fno-pic -Iinclude -c $< -o $(BUILD)/userland/$*.o
	$(LD) $(LDFLAGS) -T $(LXE_LINKER_SCRIPT) -o $@ $(BUILD)/userland/$*.o -lgcc

$(BUILD)/userland/%.lxe: $(BUILD)/userland/%.elf $(MKLXE_TOOL) | dirs
	$(OBJCOPY) -O binary -j .text $< $(BUILD)/userland/$*.text.bin
	$(OBJCOPY) -O binary -j .data $< $(BUILD)/userland/$*.data.bin
	$(eval ENTRY := $(shell $(NM) $< | awk '$$3=="_start"{print $$1}'))
	$(eval BSS_SIZE := $(shell $(NM) $< | awk \
	    '$$3=="__bss_start"{s=strtonum("0x"$$1)} $$3=="__bss_end"{e=strtonum("0x"$$1)} END{print e-s}'))
	$(MKLXE_TOOL) $(ENTRY) $(BUILD)/userland/$*.text.bin $(BUILD)/userland/$*.data.bin $(BSS_SIZE) $@

# ---- ISO image ------------------------------------------------------------
iso: $(KERNEL_BIN)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/limoos.kernel
	$(GRUBMK) -o $(ISO_IMAGE) $(ISO_DIR)
	@echo "Built $(ISO_IMAGE)"

# ---- Run targets -----------------------------------------------------------
run: iso
	$(QEMU) -cdrom $(ISO_IMAGE) -serial stdio

run-kernel: $(KERNEL_BIN)
	$(QEMU) -kernel $(KERNEL_BIN) -serial stdio

# ---- Cleanup ----------------------------------------------------------------
clean:
	rm -rf $(BUILD) $(ISO_IMAGE) $(ISO_DIR)/boot/limoos.kernel
