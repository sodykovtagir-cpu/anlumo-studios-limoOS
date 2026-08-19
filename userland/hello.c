/* =============================================================================
 * hello.c — smoke-test app for the LXE pipeline
 *
 * No libc, no syscalls yet (int 0x80 lands here once kernel/syscall.c
 * exists) — it writes directly to VGA text memory to prove that
 * load_lxe() correctly copied .text/.data, zeroed .bss, and jumped to
 * the right entry point.
 * ========================================================================== */

#include <stdint.h>

static volatile uint16_t *const VGA = (uint16_t *)0xB8000;

/* Exercises .data (non-zero initialized) and .bss (zero-initialized) to
 * confirm the loader handles both correctly. */
static const char msg[] = "Hello from LXE!";
static int call_count; /* .bss: must start at 0 */

void _start(void)
{
    call_count++; /* if .bss weren't zeroed, this could start anywhere */

    for (int i = 0; msg[i] != '\0'; i++) {
        VGA[i] = (uint16_t)msg[i] | (0x0B << 8); /* light cyan on black */
    }

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
