// (c) 2025 Anlumo Studios
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_init(void);
char keyboard_read_char(void);   // блокирующее чтение
int keyboard_has_key(void);      // неблокирующая проверка
uint8_t keyboard_read_scancode(void);   // читает скан-код (для стрелок и т.п.)
char scancode_to_ascii(uint8_t sc);

#endif