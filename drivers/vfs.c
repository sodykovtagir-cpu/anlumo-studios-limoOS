// (c) 2026 Anlumo Studios
#include <stddef.h>
#include "vga.h"
#include <stdint.h>

#define VFS_SIZE (16 * 1024 * 1024)  // 16 MB
#define MAX_FILES 128
#define MAX_PATH 128
#define MAX_NAME 32
#define MAX_DATA 4096

enum node_type { TYPE_FILE, TYPE_DIR };

struct vfs_node {
    char name[MAX_NAME];
    enum node_type type;
    uint32_t parent;
    uint32_t data_start;
    uint32_t data_size;
    uint32_t first_child;
    uint32_t next_sibling;
};

static uint8_t vfs_buffer[VFS_SIZE];
static struct vfs_node* nodes;
static uint32_t node_count;
static int current_dir;

// внутренние строковые функции
static int vfs_strlen(const char* s) {
    int len = 0;
    while (*s++) len++;
    return len;
}
static int vfs_strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}
static void vfs_strcpy(char* dst, const char* src) {
    while (*src) *dst++ = *src++;
    *dst = '\0';
}
static void vfs_memcpy(void* dst, const void* src, int n) {
    for (int i = 0; i < n; i++) ((char*)dst)[i] = ((char*)src)[i];
}

static int vfs_find_node(const char* path, int start_dir) {
    if (path[0] == '/') {
        start_dir = 0;
        path++;
    }
    if (path[0] == '\0') return start_dir;
    char part[MAX_NAME];
    int i = 0;
    while (*path && *path != '/') {
        if (i < MAX_NAME-1) part[i++] = *path;
        path++;
    }
    part[i] = '\0';
    int child = nodes[start_dir].first_child;
    while (child != 0xFFFFFFFF) {
        if (vfs_strcmp(nodes[child].name, part) == 0) {
            if (*path == '/')
                return vfs_find_node(path, child);
            else
                return child;
        }
        child = nodes[child].next_sibling;
    }
    return -1;
}

static int vfs_add_node(const char* name, enum node_type type, int parent_idx) {
    if (node_count >= MAX_FILES) return -1;
    int idx = node_count++;
    struct vfs_node* node = &nodes[idx];
    vfs_strcpy(node->name, name);
    node->type = type;
    node->parent = parent_idx;
    node->data_start = 0;
    node->data_size = 0;
    node->first_child = 0xFFFFFFFF;
    node->next_sibling = 0xFFFFFFFF;
    if (parent_idx != 0xFFFFFFFF) {
        struct vfs_node* parent = &nodes[parent_idx];
        if (parent->first_child == 0xFFFFFFFF) {
            parent->first_child = idx;
        } else {
            int last = parent->first_child;
            while (nodes[last].next_sibling != 0xFFFFFFFF)
                last = nodes[last].next_sibling;
            nodes[last].next_sibling = idx;
        }
    }
    return idx;
}

static uint32_t vfs_alloc_data(uint32_t size) {
    static uint32_t next_free = 0;
    uint32_t start = next_free;
    next_free += size;
    if (next_free > VFS_SIZE) return 0xFFFFFFFF;
    return start;
}

void vfs_init(void) {
    uint32_t* magic = (uint32_t*)vfs_buffer;
    if (*magic != 0x4C494D4F) {
        vfs_format();
    }
    nodes = (struct vfs_node*)(vfs_buffer + 4);
    node_count = *(uint32_t*)(vfs_buffer + 8);
    current_dir = 0;
}

void vfs_format(void) {
    uint32_t* magic = (uint32_t*)vfs_buffer;
    *magic = 0x4C494D4F;
    nodes = (struct vfs_node*)(vfs_buffer + 4);
    node_count = 0;
    vfs_add_node("/", TYPE_DIR, 0xFFFFFFFF);
    current_dir = 0;
    *(uint32_t*)(vfs_buffer + 8) = node_count;
}

int vfs_mkdir(const char* path) {
    char parent_path[MAX_PATH], dirname[MAX_NAME];
    vfs_strcpy(parent_path, path);
    int last_slash = -1;
    for (int i = 0; parent_path[i]; i++)
        if (parent_path[i] == '/') last_slash = i;
    if (last_slash == -1) return -1;
    if (last_slash == 0) {
        vfs_strcpy(parent_path, "/");
        vfs_strcpy(dirname, path + 1);
    } else {
        parent_path[last_slash] = '\0';
        vfs_strcpy(dirname, path + last_slash + 1);
    }
    int parent = vfs_find_node(parent_path, current_dir);
    if (parent < 0 || nodes[parent].type != TYPE_DIR) return -1;
    int child = nodes[parent].first_child;
    while (child != 0xFFFFFFFF) {
        if (vfs_strcmp(nodes[child].name, dirname) == 0) return -1;
        child = nodes[child].next_sibling;
    }
    int new_dir = vfs_add_node(dirname, TYPE_DIR, parent);
    return (new_dir >= 0) ? 0 : -1;
}

int vfs_touch(const char* path) {
    char parent_path[MAX_PATH], filename[MAX_NAME];
    vfs_strcpy(parent_path, path);
    int last_slash = -1;
    for (int i = 0; parent_path[i]; i++)
        if (parent_path[i] == '/') last_slash = i;
    if (last_slash == -1) return -1;
    if (last_slash == 0) {
        vfs_strcpy(parent_path, "/");
        vfs_strcpy(filename, path + 1);
    } else {
        parent_path[last_slash] = '\0';
        vfs_strcpy(filename, path + last_slash + 1);
    }
    int parent = vfs_find_node(parent_path, current_dir);
    if (parent < 0 || nodes[parent].type != TYPE_DIR) return -1;
    int child = nodes[parent].first_child;
    while (child != 0xFFFFFFFF) {
        if (vfs_strcmp(nodes[child].name, filename) == 0) return -1;
        child = nodes[child].next_sibling;
    }
    int new_file = vfs_add_node(filename, TYPE_FILE, parent);
    return (new_file >= 0) ? 0 : -1;
}

int vfs_write(const char* path, const char* data) {
    int node = vfs_find_node(path, current_dir);
    if (node < 0 || nodes[node].type != TYPE_FILE) return -1;
    int len = vfs_strlen(data);
    if (len > MAX_DATA) len = MAX_DATA;
    uint32_t offset = vfs_alloc_data(len + 1);
    if (offset == 0xFFFFFFFF) return -1;
    vfs_memcpy(vfs_buffer + offset, data, len);
    vfs_buffer[offset + len] = '\0';
    nodes[node].data_start = offset;
    nodes[node].data_size = len;
    return 0;
}

int vfs_read(const char* path, char* buffer, int max_len) {
    int node = vfs_find_node(path, current_dir);
    if (node < 0 || nodes[node].type != TYPE_FILE) return -1;
    int len = nodes[node].data_size;
    if (len > max_len) len = max_len;
    vfs_memcpy(buffer, vfs_buffer + nodes[node].data_start, len);
    buffer[len] = '\0';
    return len;
}

int vfs_rm(const char* path) {
    int node = vfs_find_node(path, current_dir);
    if (node < 0) return -1;
    if (node == 0) return -1;
    if (nodes[node].type == TYPE_DIR && nodes[node].first_child != 0xFFFFFFFF)
        return -1;
    int parent = nodes[node].parent;
    if (parent != 0xFFFFFFFF) {
        struct vfs_node* par = &nodes[parent];
        if (par->first_child == node) {
            par->first_child = nodes[node].next_sibling;
        } else {
            int prev = par->first_child;
            while (prev != 0xFFFFFFFF && nodes[prev].next_sibling != node)
                prev = nodes[prev].next_sibling;
            if (prev != 0xFFFFFFFF)
                nodes[prev].next_sibling = nodes[node].next_sibling;
        }
    }
    nodes[node].name[0] = '\0';
    return 0;
}

int vfs_ls(const char* path, int start_row, int start_col) {
    int dir = (path == NULL) ? current_dir : vfs_find_node(path, current_dir);
    if (dir < 0 || nodes[dir].type != TYPE_DIR) {
        vga_print_at("Directory not found\n", start_col, start_row, 0x0F);
        return 1;
    }
    int child = nodes[dir].first_child;
    int row = start_row;
    if (child == 0xFFFFFFFF) {
        vga_print_at("(empty)\n", start_col, row, 0x0F);
        return 1;
    }
    while (child != 0xFFFFFFFF) {
        vga_print_at(nodes[child].name, start_col, row, 0x0F);
        int len = vfs_strlen(nodes[child].name);
        if (nodes[child].type == TYPE_DIR)
            vga_print_at("/", start_col + len, row, 0x0F);
        row++;
        child = nodes[child].next_sibling;
    }
    return row - start_row;
}

int vfs_cd(const char* path) {
    int new_dir = vfs_find_node(path, current_dir);
    if (new_dir < 0 || nodes[new_dir].type != TYPE_DIR) return -1;
    current_dir = new_dir;
    return 0;
}

const char* vfs_get_cwd(void) {
    static char path_buf[MAX_PATH];
    if (current_dir == 0) {
        path_buf[0] = '/';
        path_buf[1] = '\0';
        return path_buf;
    }
    char stack[MAX_PATH][MAX_NAME];
    int depth = 0;
    int cur = current_dir;
    while (cur != 0) {
        vfs_strcpy(stack[depth++], nodes[cur].name);
        cur = nodes[cur].parent;
    }
    int pos = 0;
    for (int i = depth-1; i >= 0; i--) {
        path_buf[pos++] = '/';
        int j = 0;
        while (stack[i][j]) path_buf[pos++] = stack[i][j++];
    }
    path_buf[pos] = '\0';
    return path_buf;
}