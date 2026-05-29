# <img src="logo.png" width="32"> LimoOS – свободная лёгкая операционная система

**LimoOS** – проект 12-летнего энтузиаста, который начался с желания понять, как работают компьютеры «под капотом».  
Это 32-битная ОС с собственным ядром **LimCore**, командной строкой.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![GitHub release](https://img.shields.io/badge/release-v0.1-blue)](https://github.com/anlumo/limos/releases)

---

## ✨ Особенности

- **Загрузчик** – Multiboot‑совместимый (работает с QEMU и реальным железом).
- **Ядро LimCore** – написано на C и ассемблере, минимальное и быстрое.
- **Текстовый режим** – полноценный VGA‑вывод (80×25) с мигающим курсором.
- **Драйвер клавиатуры** – PS/2, поддержка английских букв, цифр, Backspace, Enter.
- **Командная строка** – встроенный shell с командами:
  - `help` – справка  
  - `cls` – очистка экрана  
  - `ver` – версия ОС и ядра  
  - `echo <текст>` – вывод текста обратно  
  - `calc <a> <+|-|*|/> <b>` – целочисленный калькулятор  
  - `reboot` – перезагрузка системы  
  - `mem` – информация о памяти (заглушка)  
  - `run <file.lxe>` – запуск приложений (в разработке)
- **История команд** (планируется) – навигация по предыдущим вводам стрелками.
- **Брендинг** – `(c) 2025 Anlumo Studios` в комментариях и при загрузке.

---

## 🚀 Как собрать и запустить

### Требования
- Linux (или WSL под Windows)  
- Установленный кросс-компилятор `i686-elf-gcc`, `nasm`, `make`, `qemu-system-i386`

### 📦 Создание приложений для LimoOS (.lxe)
LimoOS использует свой собственный формат исполняемых файлов — .lxe (Limo eXecutable). Это простой плоский бинарник (flat binary), который загружается в память и запускается напрямую.

🔧 Требования
nasm (для ассемблера)

i686-elf-gcc (для C, опционально)

LimoOS (конечно)

1. Простейшее приложение на ассемблере
Создай файл hello.asm:

nasm
; hello.asm – минимальное .lxe приложение
bits 32
section .text

_start:
    mov eax, 0xB8000          ; видеопамять текстового режима
    mov ebx, msg
    xor ecx, ecx

.loop:
    mov dl, [ebx + ecx]
    test dl, dl
    jz .done
    mov [eax + ecx*2], dl
    mov byte [eax + ecx*2 + 1], 0x0F  ; белый на чёрном
    inc ecx
    jmp .loop

.done:
    ret

msg db "Hello from .lxe!", 0
Сборка:

bash
nasm -f bin hello.asm -o hello.lxe
2. Приложение на C
hello.c:

c
void _start(void) {
    volatile char* vga = (char*)0xB8000;
    const char msg[] = "Hello from C .lxe!";
    int i = 0;
    while (msg[i]) {
        vga[i*2] = msg[i];
        vga[i*2 + 1] = 0x0F;
        i++;
    }
}
Сборка:

bash
i686-elf-gcc -ffreestanding -m32 -fno-pic -fno-pie -c hello.c -o hello.o
i686-elf-gcc -nostdlib -ffreestanding -m32 -fno-pic -fno-pie \
    -Ttext=0x400000 -Wl,--oformat=binary -Wl,-e,_start -o hello.lxe hello.o
3. Запуск в LimoOS
Помести hello.lxe в образ дискеты или в корень ISO. В командной строке LimoOS выполни:

text
run hello.lxe
Если приложение написано правильно, ты увидишь вывод на экране.

4. Формат .lxe (технические детали)
Файл .lxe – это плоский бинарник без заголовков. Он загружается по фиксированному адресу (обычно 0x400000). Точка входа определяется линковщиком (метка _start).

Поле	Размер	Описание
Код	любое	Инструкции процессора x86
Данные	любое	Константы, строки
Никаких сложных заголовков — только код и данные.

5. Пример Makefile для сборки .lxe
makefile
# Makefile для сборки .lxe приложений

CC = i686-elf-gcc
ASM = nasm
CFLAGS = -ffreestanding -m32 -fno-pic -fno-pie -nostdlib
LDFLAGS = -Ttext=0x400000 --oformat binary -e _start

all: hello.lxe

hello.asm.lxe: hello.asm
	$(ASM) -f bin $< -o $@

hello.c.lxe: hello.c
	$(CC) $(CFLAGS) -c $< -o hello.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ hello.o

clean:
	rm -f *.o *.lxe
6. Важные замечания
Не используй системные вызовы Linux/Windows — они не работают в LimoOS. Общайся с "железом" напрямую (видеопамять, порты).

Приложение должно завершаться инструкцией ret.

Если приложение зависает — проверь, что точка входа (_start) и адрес линковки (0x400000) правильные.
