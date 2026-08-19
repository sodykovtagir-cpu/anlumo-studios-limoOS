/* =============================================================================
 * mklxe.c — packages a .lxe binary from the pieces the build produces
 *
 * This is a HOST tool: compiled with the regular system gcc (not the
 * i686-elf cross compiler) and run on the build machine, never on LimoOS.
 *
 * Usage:
 *   mklxe <entry_hex> <text.bin> <data.bin> <bss_size_dec> <out.lxe>
 *
 * The Makefile extracts entry/bss_size from the linked ELF via nm and
 * passes them in; see the `%.lxe` rule.
 * ========================================================================== */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char     magic[4];
    uint32_t entry;
    uint32_t text_offset;
    uint32_t text_size;
    uint32_t data_offset;
    uint32_t data_size;
    uint32_t bss_size;
    uint32_t reserved;
} lxe_header_t;

static long read_whole_file(const char *path, unsigned char **out)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }

    if (fseek(f, 0, SEEK_END) != 0) { perror("fseek"); exit(1); }
    long size = ftell(f);
    if (size < 0) { perror("ftell"); exit(1); }
    rewind(f);

    unsigned char *buf = malloc((size_t)size > 0 ? (size_t)size : 1);
    if (!buf) { fprintf(stderr, "mklxe: out of memory\n"); exit(1); }

    if (size > 0 && fread(buf, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "mklxe: short read on %s\n", path);
        exit(1);
    }
    fclose(f);

    *out = buf;
    return size;
}

int main(int argc, char **argv)
{
    if (argc != 6) {
        fprintf(stderr,
            "usage: %s <entry_hex> <text.bin> <data.bin> <bss_size_dec> <out.lxe>\n",
            argv[0]);
        return 1;
    }

    uint32_t entry    = (uint32_t)strtoul(argv[1], NULL, 16);
    const char *text_path = argv[2];
    const char *data_path = argv[3];
    uint32_t bss_size = (uint32_t)strtoul(argv[4], NULL, 10);
    const char *out_path  = argv[5];

    unsigned char *text_buf, *data_buf;
    long text_size = read_whole_file(text_path, &text_buf);
    long data_size = read_whole_file(data_path, &data_buf);

    lxe_header_t hdr;
    memcpy(hdr.magic, "LXE1", 4);
    hdr.entry       = entry;
    hdr.text_offset = (uint32_t)sizeof(hdr);
    hdr.text_size   = (uint32_t)text_size;
    hdr.data_offset = hdr.text_offset + hdr.text_size;
    hdr.data_size   = (uint32_t)data_size;
    hdr.bss_size    = bss_size;
    hdr.reserved    = 0;

    FILE *out = fopen(out_path, "wb");
    if (!out) { perror(out_path); return 1; }

    fwrite(&hdr, sizeof(hdr), 1, out);
    if (text_size > 0) fwrite(text_buf, 1, (size_t)text_size, out);
    if (data_size > 0) fwrite(data_buf, 1, (size_t)data_size, out);
    fclose(out);

    printf("mklxe: wrote %s (entry=0x%08x text=%ld data=%ld bss=%u)\n",
           out_path, entry, text_size, data_size, bss_size);

    free(text_buf);
    free(data_buf);
    return 0;
}
