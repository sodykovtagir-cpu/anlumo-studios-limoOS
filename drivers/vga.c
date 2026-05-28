// (c) 2026 Anlumo Studios
#include "vga.h"
#include "io.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

static int cursor_x = 0;
static int cursor_y = 0;

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void update_cursor(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_MEMORY[i] = vga_entry(' ', 0x07);
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

void vga_putchar(char c, uint8_t color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) cursor_x--;
        else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = VGA_WIDTH - 1;
        }
        VGA_MEMORY[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(' ', color);
    } else {
        VGA_MEMORY[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(c, color);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= VGA_HEIGHT) {
        // Прокрутка
        for (int y = 0; y < VGA_HEIGHT - 1; y++)
            for (int x = 0; x < VGA_WIDTH; x++)
                VGA_MEMORY[y * VGA_WIDTH + x] = VGA_MEMORY[(y+1) * VGA_WIDTH + x];
        for (int x = 0; x < VGA_WIDTH; x++)
            VGA_MEMORY[(VGA_HEIGHT-1) * VGA_WIDTH + x] = vga_entry(' ', 0x07);
        cursor_y = VGA_HEIGHT - 1;
    }
    update_cursor();
}

void vga_putchar_at(int x, int y, char c, uint8_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT)
        VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(c, color);
}

void vga_print(const char* str, uint8_t color) {
    while (*str) vga_putchar(*str++, color);
}

void vga_print_at(const char* str, int x, int y, uint8_t color) {
    int old_x = cursor_x, old_y = cursor_y;
    cursor_x = x;
    cursor_y = y;
    while (*str) vga_putchar(*str++, color);
    cursor_x = old_x;
    cursor_y = old_y;
    update_cursor();
}

void vga_set_cursor(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        cursor_x = x;
        cursor_y = y;
        update_cursor();
    }
}

void vga_get_cursor(int* x, int* y) {
    *x = cursor_x;
    *y = cursor_y;
}