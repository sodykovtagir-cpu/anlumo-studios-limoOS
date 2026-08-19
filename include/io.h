#ifndef LIMOOS_IO_H
#define LIMOOS_IO_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* A tiny delay for old hardware that needs a beat between consecutive I/O
 * port writes (the 8259 PIC in particular). Writing to an unused port
 * (0x80, POST diagnostic port on real BIOS-era machines) burns roughly one
 * bus cycle without needing a real timer — standard OSDev trick. */
static inline void io_wait(void)
{
    outb(0x80, 0);
}

#endif /* LIMOOS_IO_H */
