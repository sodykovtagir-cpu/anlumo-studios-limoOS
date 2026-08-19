#ifndef LIMOOS_VFS_H
#define LIMOOS_VFS_H

#include <stdint.h>

#define VFS_MAX_NODES 64
#define VFS_MAX_NAME  32

typedef enum {
    VFS_FILE,
    VFS_DIR, /* not really usable yet — namespace is flat (see vfs.c) */
} vfs_node_type_t;

typedef struct {
    char name[VFS_MAX_NAME];
    vfs_node_type_t type;
    uint32_t size;
    uint32_t capacity;   /* 0 = read-only (const-backed); >0 = writable,
                          * this many bytes available via vfs_write() */
    const uint8_t *data; /* backing storage */
    int in_use;
} vfs_node_t;

/* Registers a read-only file backed by memory that outlives the call
 * (a string literal, a static array, or a buffer from the LXE loader/
 * physical allocator — never stack memory). Returns the node, or NULL
 * if the name already exists or the node table is full. */
vfs_node_t *vfs_create_file(const char *name, const uint8_t *data, uint32_t size);

/* Registers a WRITABLE file backed by one of a small fixed pool of
 * internal buffers (see vfs.c — no real allocator exists yet, so this
 * is capacity-limited: VFS_WRITABLE_SLOTS files of VFS_WRITABLE_SLOT_SIZE
 * bytes each, hardcoded). initial_data/initial_size seed the content
 * (pass NULL/0 for an empty new file). Returns NULL if the name exists,
 * the node table is full, or every writable slot is already taken. */
vfs_node_t *vfs_create_writable_file(const char *name, const uint8_t *initial_data,
                                      uint32_t initial_size);

/* Flat-namespace lookup by exact name — no path separators understood
 * yet (no directories, no "/"). Returns NULL if nothing matches. */
vfs_node_t *vfs_find(const char *name);

/* Copies up to `size` bytes starting at `offset` into `buffer`, clamped
 * to the node's actual size (reading past the end returns fewer bytes,
 * not garbage or an out-of-bounds read). Returns the number of bytes
 * actually copied. */
uint32_t vfs_read(const vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

/* Overwrites a writable node's content (from the start — no partial/
 * append writes yet). Returns bytes actually written, clamped to the
 * node's capacity; returns 0 without changing anything if the node is
 * NULL or read-only (capacity == 0) — always check for that, since 0 is
 * also the legitimate result of writing an empty buffer. */
uint32_t vfs_write(vfs_node_t *node, const uint8_t *data, uint32_t size);

/* Writes every registered file name into `out`, space-separated, NUL-
 * terminated, truncated to fit `out_size` (including the NUL) if
 * necessary. Returns the number of names actually written. For a
 * terminal's `ls` — no directories, so nothing to distinguish per-entry
 * beyond the name. */
uint32_t vfs_list_names(char *out, uint32_t out_size);

#endif /* LIMOOS_VFS_H */
