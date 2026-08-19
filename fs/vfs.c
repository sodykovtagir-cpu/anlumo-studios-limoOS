/* =============================================================================
 * vfs.c — minimal Virtual File System: a flat, in-memory table
 *
 * This is deliberately not a real filesystem — no directories, no
 * on-disk storage. What it IS: the seam where a real backend (initrd, a
 * FAT/ext2 driver, whatever comes later) plugs in without changing how
 * the rest of the kernel reads/writes files. Everything above this
 * layer (a shell's `cat`, the text editor's Save button, the LXE loader
 * reading an app off "disk") should go through vfs_find()/vfs_read()/
 * vfs_write(), never touch node storage directly — that's what keeps
 * swapping the backend possible.
 *
 * Two kinds of file:
 *   - Read-only, via vfs_create_file(): backed by memory the CALLER
 *     owns (typically a `static const` array in .rodata) — capacity 0,
 *     vfs_write() always fails on these.
 *   - Writable, via vfs_create_writable_file(): backed by one of a
 *     small fixed pool of buffers THIS FILE owns (see
 *     VFS_WRITABLE_SLOTS/VFS_WRITABLE_SLOT_SIZE below). There's no
 *     kmalloc yet (see kernel/pmm.c's TODOs), so this is a hard,
 *     hardcoded capacity rather than a real dynamic allocator — plenty
 *     for a handful of small text files, not a general-purpose fs.
 * ========================================================================== */

#include "vfs.h"

static vfs_node_t nodes[VFS_MAX_NODES];

#define VFS_WRITABLE_SLOTS 8
#define VFS_WRITABLE_SLOT_SIZE 4096

static uint8_t writable_pool[VFS_WRITABLE_SLOTS][VFS_WRITABLE_SLOT_SIZE];
static int writable_pool_used[VFS_WRITABLE_SLOTS];

static int names_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b; /* both hit '\0' at the same position */
}

static vfs_node_t *alloc_node_slot(const char *name)
{
    if (vfs_find(name) != 0) {
        return 0; /* name already taken */
    }

    for (int i = 0; i < VFS_MAX_NODES; i++) {
        if (!nodes[i].in_use) {
            int j = 0;
            for (; j < VFS_MAX_NAME - 1 && name[j] != '\0'; j++) {
                nodes[i].name[j] = name[j];
            }
            nodes[i].name[j] = '\0';
            nodes[i].type = VFS_FILE;
            nodes[i].in_use = 1;
            return &nodes[i];
        }
    }
    return 0; /* table full */
}

vfs_node_t *vfs_create_file(const char *name, const uint8_t *data, uint32_t size)
{
    vfs_node_t *node = alloc_node_slot(name);
    if (node == 0) {
        return 0;
    }
    node->size = size;
    node->capacity = 0; /* read-only */
    node->data = data;
    return node;
}

vfs_node_t *vfs_create_writable_file(const char *name, const uint8_t *initial_data,
                                      uint32_t initial_size)
{
    int slot = -1;
    for (int i = 0; i < VFS_WRITABLE_SLOTS; i++) {
        if (!writable_pool_used[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        return 0; /* every writable buffer already taken */
    }

    vfs_node_t *node = alloc_node_slot(name);
    if (node == 0) {
        return 0; /* name taken or node table full — slot stays free */
    }

    writable_pool_used[slot] = 1;
    node->data = writable_pool[slot];
    node->capacity = VFS_WRITABLE_SLOT_SIZE;
    node->size = 0;

    if (initial_data != 0 && initial_size > 0) {
        uint32_t n = (initial_size < node->capacity) ? initial_size : node->capacity;
        uint8_t *buf = (uint8_t *)(uintptr_t)node->data; /* safe: we own this
            buffer (it's writable_pool[slot], never a caller's const data) */
        for (uint32_t i = 0; i < n; i++) {
            buf[i] = initial_data[i];
        }
        node->size = n;
    }

    return node;
}

vfs_node_t *vfs_find(const char *name)
{
    for (int i = 0; i < VFS_MAX_NODES; i++) {
        if (nodes[i].in_use && names_equal(nodes[i].name, name)) {
            return &nodes[i];
        }
    }
    return 0;
}

uint32_t vfs_read(const vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (node == 0 || offset >= node->size) {
        return 0;
    }

    uint32_t available = node->size - offset;
    uint32_t to_copy = (size < available) ? size : available;

    for (uint32_t i = 0; i < to_copy; i++) {
        buffer[i] = node->data[offset + i];
    }
    return to_copy;
}

uint32_t vfs_write(vfs_node_t *node, const uint8_t *data, uint32_t size)
{
    if (node == 0 || node->capacity == 0) {
        return 0; /* NULL or read-only (const-backed) node */
    }

    uint32_t to_copy = (size < node->capacity) ? size : node->capacity;
    uint8_t *buf = (uint8_t *)(uintptr_t)node->data; /* safe: writable
        nodes always point into writable_pool, which we own */

    for (uint32_t i = 0; i < to_copy; i++) {
        buf[i] = data[i];
    }
    node->size = to_copy;
    return to_copy;
}

uint32_t vfs_list_names(char *out, uint32_t out_size)
{
    if (out_size == 0) {
        return 0;
    }

    uint32_t pos = 0;
    uint32_t count = 0;
    out[0] = '\0';

    for (int i = 0; i < VFS_MAX_NODES; i++) {
        if (!nodes[i].in_use) {
            continue;
        }

        uint32_t needed = 0;
        while (nodes[i].name[needed] != '\0') {
            needed++;
        }
        if (count > 0) {
            needed++; /* separating space */
        }

        if (pos + needed + 1 > out_size) { /* +1 for the final NUL */
            break; /* wouldn't fit — stop rather than truncate mid-name */
        }

        if (count > 0) {
            out[pos++] = ' ';
        }
        for (uint32_t j = 0; nodes[i].name[j] != '\0'; j++) {
            out[pos++] = nodes[i].name[j];
        }
        out[pos] = '\0';
        count++;
    }

    return count;
}
