/* =============================================================================
 * lv_conf.h — LVGL configuration for LimoOS
 *
 * This is NOT the full lv_conf.h LVGL ships as a template (lvgl/lv_conf_template.h)
 * — it's a minimal, deliberately trimmed config covering only what this
 * kernel needs to compile and run LVGL in a freestanding environment (no
 * libc malloc/free, no stdio, no filesystem). Once basic rendering works,
 * more of LVGL's optional features (extra widgets, animations, themes)
 * can be enabled here incrementally.
 *
 * Place this file at the project root or lvgl/lv_conf.h — LVGL's build
 * looks for lv_conf.h on the include path. This project's Makefile adds
 * the project root to -I, so this file staying at LimoOS/lv_conf.h works.
 * ========================================================================== */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* This freestanding i686-elf toolchain has no libc, so <inttypes.h>
 * doesn't exist — LVGL only uses it for PRId32-style format macros
 * (lv_conf_internal.h's LV_INTTYPES_INCLUDE), which include/inttypes_stub.h
 * supplies without pulling in the rest of a real inttypes.h/libc. */
#define LV_INTTYPES_INCLUDE "inttypes_stub.h"

/* ---- Color depth: match the VESA framebuffer -------------------------
 * vesa_init() (drivers/vesa/vesa.c) only accepts 24bpp or 32bpp RGB
 * modes — 32 here matches the common case and what boot.asm requests
 * (1024x768x32). If you change the requested depth in boot.asm's
 * multiboot header, update this to match. */
#define LV_COLOR_DEPTH 32

/* ---- Memory: no malloc/free in this freestanding kernel yet -----------
 * LV_MEM_CUSTOM off means LVGL uses its own internal allocator carved
 * out of a static buffer sized below, rather than calling malloc(). This
 * avoids needing a working heap allocator before LVGL can even start —
 * revisit once kernel/pmm.c grows a proper kmalloc(). */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (256U * 1024U) /* raised from 64KB — a terminal with
                                    * growing scrollback text plus
                                    * multiple open windows can realistically
                                    * exhaust a smaller heap over time */

/* ---- Tick source --------------------------------------------------------
 * LVGL needs to know elapsed milliseconds; lv_port.c below calls
 * lv_tick_inc() from the PIT handler (drivers/pit/pit.c) instead of
 * using LV_TICK_CUSTOM's function-pointer hook, so this stays off. */
#define LV_TICK_CUSTOM 0

/* ---- No OS, no filesystem, no libc standard I/O ------------------------ */
#define LV_USE_OS LV_OS_NONE
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN
#define LV_USE_FS_STDIO 0

/* ---- Logging: off by default (no console to print to before the GUI
 * itself exists, and no vsnprintf-equivalent wired up yet) -------------- */
#define LV_USE_LOG 0

/* ---- Widgets actually needed for a first desktop pass ------------------
 * Trimmed hard on purpose — enable more as the GUI grows past a window +
 * label + button. Check lvgl/lv_conf_template.h for the full list LVGL
 * ships when you're ready to expand this. */
#define LV_USE_LABEL 1
#define LV_USE_BTN   1
#define LV_USE_TEXTAREA 1
#define LV_USE_IMG   0
#define LV_USE_WIN   1

/* Default font: LVGL's own built-in bitmap font, NOT font8x16.h (that
 * one belongs to the raw framebuffer console in kernel/console.c and
 * isn't in a format LVGL understands). The two text renderers are
 * independent until/unless someone writes an lv_font_t wrapper around
 * font8x16 later. */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#endif /* LV_CONF_H */
