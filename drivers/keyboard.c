// (c) 2026 Anlumo Studios
#include "keyboard.h"
#include "io.h"

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64

uint8_t keyboard_read_scancode(void) {
    while ((inb(KEYBOARD_STATUS_PORT) & 1) == 0);
    return inb(KEYBOARD_DATA_PORT);
}

static uint8_t read_scan_code(void) {
    while ((inb(KEYBOARD_STATUS_PORT) & 1) == 0)
        io_wait();
    return inb(KEYBOARD_DATA_PORT);
}

char scancode_to_ascii(uint8_t scancode) {
    static const char table[128] = {
        0,   0,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '+', '\b', 0,
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
        'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    if (scancode < 128)
        return table[scancode];
    return 0;
}

char keyboard_read_char(void) {
    uint8_t sc;
    char ch;
    do {
        sc = read_scan_code();
        ch = scancode_to_ascii(sc);
    } while (ch == 0);
    return ch;
}

int keyboard_has_key(void) {
    return (inb(KEYBOARD_STATUS_PORT) & 1);
}

void keyboard_init(void) {
    // Можно сбросить клавиатуру, пока ничего
}