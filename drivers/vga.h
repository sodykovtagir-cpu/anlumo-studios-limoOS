// (c) 2025 Anlumo Studios
#ifndef VGA_H
#define VGA_H

#include <stdint.h>

void vga_clear(void);
void vga_putchar(char c, uint8_t color);
void vga_putchar_at(int x, int y, char c, uint8_t color);
void vga_print(const char* str, uint8_t color);
void vga_print_at(const char* str, int x, int y, uint8_t color);
void vga_set_cursor(int x, int y);
void vga_get_cursor(int* x, int* y);

#endif