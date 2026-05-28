// (c) 2026 Anlumo Studios
#ifndef VFS_H
#define VFS_H

#include <stdint.h>

void vfs_init(void);
void vfs_format(void);
int vfs_mkdir(const char* path);
int vfs_touch(const char* path);
int vfs_write(const char* path, const char* data);
int vfs_read(const char* path, char* buffer, int max_len);
int vfs_rm(const char* path);
int vfs_cd(const char* path);
int vfs_ls(const char* path, int start_row, int start_col);
const char* vfs_get_cwd(void);

#endif