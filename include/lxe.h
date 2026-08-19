#ifndef LIMOOS_LXE_H
#define LIMOOS_LXE_H

#include <stdint.h>

/* =============================================================================
 * LXE — Limo Executable format
 *
 * A minimal, fixed-size header followed directly by raw section data laid
 * out back-to-back in the order: .text, .data, .bss (bss occupies zero bytes
 * on disk — it is zero-filled at load time).
 *
 * On-disk layout:
 *   [ lxe_header_t ]
 *   [ .text bytes  ]  (text_size bytes)
 *   [ .data bytes  ]  (data_size bytes)
 *   (no .bss bytes on disk — bss_size tells the loader how much to zero)
 * ========================================================================== */

#define LXE_MAGIC "LXE1"        /* 4-byte magic, not NUL-terminated on disk */
#define LXE_MAGIC_SIZE 4

typedef struct __attribute__((packed)) {
    char     magic[LXE_MAGIC_SIZE]; /* must equal "LXE1" */
    uint32_t entry;                 /* absolute virtual address of _start (code is
                                      * NOT position-independent — it is compiled
                                      * assuming it runs at LXE_LOAD_BASE from
                                      * lxe/linker_lxe.ld, so the loader must place
                                      * it at that exact address) */
    uint32_t text_offset;           /* byte offset of .text from start of file */
    uint32_t text_size;             /* size of .text in bytes */
    uint32_t data_offset;           /* byte offset of .data from start of file */
    uint32_t data_size;             /* size of .data in bytes */
    uint32_t bss_size;              /* size of .bss to zero-fill (not present on disk) */
    uint32_t reserved;              /* padding / future use, must be 0 */
} lxe_header_t;

#define LXE_HEADER_SIZE (sizeof(lxe_header_t))

#endif /* LIMOOS_LXE_H */
