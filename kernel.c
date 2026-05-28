// (c) 2025 Anlumo Studios
#include "drivers/vga.h"
#include "kernel/gdt.h"
#include "drivers/keyboard.h"
#include "drivers/io.h"   // обязательно

#define HISTORY_SIZE 16
#define CMD_MAX_LEN  63

static char history[HISTORY_SIZE][CMD_MAX_LEN + 1];
static int history_count = 0;
static int history_pos = -1;      // -1 = новая команда, иначе индекс в истории
static char saved_cmd[CMD_MAX_LEN + 1];   // для временного сохранения при навигации

// Простые строковые функции
int strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

int strncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
        if (a[i] == '\0') return 0;
    }
    return 0;
}

void strcpy(char* dst, const char* src) {
    while (*src) *dst++ = *src++;
    *dst = '\0';
}

int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

int starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str != *prefix) return 0;
        str++; prefix++;
    }
    return 1;
}

// Разбиение строки на аргументы (разделитель пробелы)
int split(char* cmd, char* args[], int max_args) {
    int count = 0;
    char* p = cmd;
    while (*p && count < max_args) {
        while (*p == ' ') p++;
        if (*p == '\0') break;
        args[count++] = p;
        while (*p && *p != ' ') p++;
        if (*p) {
            *p = '\0';
            p++;
        }
    }
    return count;
}

int atoi(const char* s) {
    int res = 0;
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res;
}

void itoa(int n, char* buf) {
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    int i = 0;
    int temp = n;
    while (temp) {
        buf[i++] = '0' + (temp % 10);
        temp /= 10;
    }
    // переворот
    for (int j = 0; j < i / 2; j++) {
        char t = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = t;
    }
    buf[i] = '\0';
}

void clear_with_logo(void) {
    vga_clear();
    vga_print_at(" _      _                     ____   _____", 0, 0, 0x0E);
    vga_print_at("| |    (_)                   / __ \\ / ____|", 0, 1, 0x0E);
    vga_print_at("| |     _ _ __ ___   ___    | |  | | (___", 0, 2, 0x0E);
    vga_print_at("| |    | | '_ ` _ \\ / _ \\   | |  | |\\___ \\", 0, 3, 0x0E);
    vga_print_at("| |____| | | | | | | (_) |  | |__| |____) |", 0, 4, 0x0E);
    vga_print_at("|______|_|_| |_| |_|\\___/    \\____/|_____/", 0, 5, 0x0E);
    vga_print_at("LimoOS v0.1", 0, 7, 0x0F);
    vga_print_at("(c) 2025 Anlumo Studios", 0, 8, 0x0A);
}

void kernel_main() {
    gdt_init();
    clear_with_logo();
    keyboard_init();

    // Инициализация истории
    for (int i = 0; i < HISTORY_SIZE; i++)
        history[i][0] = '\0';
    history_count = 0;
    history_pos = -1;

    char cmd[CMD_MAX_LEN + 1];
    int pos = 0;
    int prompt_row = 10;
    int prompt_col = 0;

    vga_print_at("LimCore> ", prompt_col, prompt_row, 0x0F);
    prompt_col += 9;

    while (1) {
        uint8_t sc = keyboard_read_scancode();
        char ch = scancode_to_ascii(sc);

        // Обработка стрелок (скан-коды без ASCII)
        if (sc == 0x48) { // Стрелка вверх
            if (history_pos >= 0) {
                // Если мы уже в истории, сохраняем текущую команду перед перемещением
                if (history_pos == -1) {
                    strcpy(saved_cmd, cmd);
                }
                history_pos--;
            } else if (history_count > 0) {
                // Выход из редактирования новой команды
                if (history_pos == -1) {
                    strcpy(saved_cmd, cmd);
                }
                history_pos = history_count - 1;
            }
            if (history_pos >= 0) {
                // Очищаем текущую строку на экране
                for (int i = 0; i < pos; i++)
                    vga_putchar_at(prompt_col - 1 - i, prompt_row, ' ', 0x0F);
                strcpy(cmd, history[history_pos]);
                pos = strlen(cmd);
                prompt_col = 9; // исходная позиция после "LimCore> "
                vga_print_at(cmd, prompt_col, prompt_row, 0x0F);
                prompt_col += pos;
            }
            continue;
        } else if (sc == 0x50) { // Стрелка вниз
            if (history_pos < history_count - 1) {
                history_pos++;
                // Очищаем строку
                for (int i = 0; i < pos; i++)
                    vga_putchar_at(prompt_col - 1 - i, prompt_row, ' ', 0x0F);
                strcpy(cmd, history[history_pos]);
                pos = strlen(cmd);
                prompt_col = 9;
                vga_print_at(cmd, prompt_col, prompt_row, 0x0F);
                prompt_col += pos;
            } else if (history_pos == history_count - 1) {
                // Выход из истории к текущей редактируемой команде
                history_pos = -1;
                for (int i = 0; i < pos; i++)
                    vga_putchar_at(prompt_col - 1 - i, prompt_row, ' ', 0x0F);
                strcpy(cmd, saved_cmd);
                pos = strlen(cmd);
                prompt_col = 9;
                vga_print_at(cmd, prompt_col, prompt_row, 0x0F);
                prompt_col += pos;
            }
            continue;
        }

        // Обычные символы (Enter, Backspace, печатные)
        if (ch == '\n') {
            cmd[pos] = '\0';
            // Сохраняем в историю, если команда не пуста и не совпадает с последней
            if (pos > 0 && (history_count == 0 || strcmp(cmd, history[history_count-1]) != 0)) {
                if (history_count < HISTORY_SIZE) {
                    strcpy(history[history_count++], cmd);
                } else {
                    // Сдвиг
                    for (int i = 0; i < HISTORY_SIZE-1; i++)
                        strcpy(history[i], history[i+1]);
                    strcpy(history[HISTORY_SIZE-1], cmd);
                }
            }
            history_pos = -1;
            pos = 0;
            prompt_row++;
            prompt_col = 0;

            // Парсим и выполняем команду
            char* args[8];
            int argc = split(cmd, args, 8);
            if (argc == 0) {
                // пустая строка
            } else if (strcmp(args[0], "help") == 0) {
                vga_print_at("Available commands:\n", prompt_col, prompt_row, 0x0F); prompt_row++;
                vga_print_at("  help            - this help\n", prompt_col, prompt_row, 0x0F); prompt_row++;
                vga_print_at("  cls             - clear screen\n", prompt_col, prompt_row, 0x0F); prompt_row++;
                vga_print_at("  ver             - show version\n", prompt_col, prompt_row, 0x0F); prompt_row++;
                vga_print_at("  echo <text>     - print text back\n", prompt_col, prompt_row, 0x0F); prompt_row++;
                vga_print_at("  reboot          - reboot system\n", prompt_col, prompt_row, 0x0F); prompt_row++;
                vga_print_at("  mem             - show memory info\n", prompt_col, prompt_row, 0x0F); prompt_row++;
                vga_print_at("  calc <a> <op> <b> - calculate (op: + - * /)\n", prompt_col, prompt_row, 0x0F); prompt_row++;
                vga_print_at("  run <file>      - run .lxe app (stub)\n", prompt_col, prompt_row, 0x0F); prompt_row++;
            } else if (strcmp(args[0], "cls") == 0) {
                clear_with_logo();
                prompt_row = 10;
                prompt_col = 0;
            } else if (strcmp(args[0], "ver") == 0) {
                vga_print_at("LimoOS v0.1 (LimCore kernel)\n", prompt_col, prompt_row, 0x0F);
                prompt_row++;
            } else if (strcmp(args[0], "echo") == 0) {
                if (argc > 1) {
                    // склеиваем аргументы с пробелами
                    char out[128] = "";
                    for (int i = 1; i < argc; i++) {
                        if (i > 1) {
                            int len = strlen(out);
                            out[len] = ' ';
                            out[len+1] = '\0';
                        }
                        strcpy(out + strlen(out), args[i]);
                    }
                    vga_print_at(out, prompt_col, prompt_row, 0x0F);
                }
                vga_print_at("\n", prompt_col, prompt_row, 0x0F);
                prompt_row++;
            } else if (strcmp(args[0], "reboot") == 0) {
                vga_print_at("Rebooting...\n", prompt_col, prompt_row, 0x0F);
                outb(0x64, 0xFE);   // перезагрузка через клавиатурный контроллер
                while (1);
            } else if (strcmp(args[0], "mem") == 0) {
                vga_print_at("Total memory: 64 MB (static info)\n", prompt_col, prompt_row, 0x0F);
                prompt_row++;
            } else if (strcmp(args[0], "calc") == 0) {
                if (argc != 4) {
                    vga_print_at("Usage: calc <a> <op> <b>\n", prompt_col, prompt_row, 0x0F);
                    prompt_row++;
                } else {
                    int a = atoi(args[1]);
                    int b = atoi(args[3]);
                    char op = args[2][0];
                    int result = 0;
                    int ok = 1;
                    switch (op) {
                        case '+': result = a + b; break;
                        case '-': result = a - b; break;
                        case '*': result = a * b; break;
                        case '/': if (b != 0) result = a / b; else ok = 0; break;
                        default: ok = 0;
                    }
                    if (!ok) {
                        vga_print_at("Error: invalid operator or division by zero\n", prompt_col, prompt_row, 0x0F);
                    } else {
                        char buf[16];
                        itoa(result, buf);
                        vga_print_at(buf, prompt_col, prompt_row, 0x0F);
                    }
                    vga_print_at("\n", prompt_col, prompt_row, 0x0F);
                    prompt_row++;
                }
            } else if (strcmp(args[0], "run") == 0) {
                if (argc < 2) {
                    vga_print_at("Usage: run <file.lxe>\n", prompt_col, prompt_row, 0x0F);
                } else {
                    vga_print_at("Running ", prompt_col, prompt_row, 0x0F);
                    vga_print_at(args[1], prompt_col+8, prompt_row, 0x0F);
                    vga_print_at(" (not implemented)\n", prompt_col+8+strlen(args[1]), prompt_row, 0x0F);
                }
                prompt_row++;
            } else if (cmd[0] != '\0') {
                vga_print_at("Unknown command: ", prompt_col, prompt_row, 0x0F);
                vga_print_at(cmd, prompt_col+17, prompt_row, 0x0F);
                vga_print_at("\n", prompt_col+17+strlen(cmd), prompt_row, 0x0F);
                prompt_row++;
            }

            // Выводим новое приглашение
            vga_print_at("LimCore> ", prompt_col, prompt_row, 0x0F);
            prompt_col += 9;
        } else if (ch == '\b') {
            if (pos > 0 && prompt_col > 0) {
                pos--;
                prompt_col--;
                vga_putchar_at(prompt_col, prompt_row, ' ', 0x0F);
            }
        } else if (ch != 0) { // печатный символ
            if (pos < CMD_MAX_LEN) {
                cmd[pos++] = ch;
                vga_putchar_at(prompt_col, prompt_row, ch, 0x0F);
                prompt_col++;
            }
        }
    }
}