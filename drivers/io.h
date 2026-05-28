// (c) 2025 Anlumo Studios
#ifndef IO_H
#define IO_H

#include "vga.h"
#include "io.h"
#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif