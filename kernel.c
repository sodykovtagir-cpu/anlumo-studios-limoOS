// (c) 2026 Anlumo Studios
#include <stddef.h>
#include "drivers/vga.h"
#include "kernel/gdt.h"
#include "drivers/keyboard.h"
#include "drivers/io.h"
#include "drivers/vfs.h"

#define HISTORY_SIZE 16
#define CMD_MAX_LEN  63

static char history[HISTORY_SIZE][CMD_MAX_LEN + 1];
static int history_count = 0;
static int history_pos = -1;
static char saved_cmd[CMD_MAX_LEN + 1];

// строковые функции
int strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}
int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}
void strcpy(char* dst, const char* src) {
    while (*src) *dst++ = *src++;
    *dst = '\0';
}
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
    if (n == 0) { buf[0]='0'; buf[1]='\0'; return; }
    int i=0, t=n;
    while(t) { buf[i++]='0'+(t%10); t/=10; }
    for(int j=0;j<i/2;j++) { char c=buf[j]; buf[j]=buf[i-1-j]; buf[i-1-j]=c; }
    buf[i]='\0';
}

void clear_with_logo(void) {
    vga_clear();
    vga_print_at(" _      _                     ____   _____", 0, 0, 0x0E);
    vga_print_at("| |    (_)                   / __ \\ / ____|", 0, 1, 0x0E);
    vga_print_at("| |     _ _ __ ___   ___    | |  | | (___", 0, 2, 0x0E);
    vga_print_at("| |    | | '_ ` _ \\ / _ \\   | |  | |\\___ \\", 0, 3, 0x0E);
    vga_print_at("| |____| | | | | | | (_) |  | |__| |____) |", 0, 4, 0x0E);
    vga_print_at("|______|_|_| |_| |_|\\___/    \\____/|_____/", 0, 5, 0x0E);
    vga_print_at("LimoOS v0.2", 0, 7, 0x0F);
    vga_print_at("(c) 2026 Anlumo Studios", 0, 8, 0x0A);
}

void kernel_main() {
    gdt_init();
    clear_with_logo();
    keyboard_init();
    vfs_init();

    for (int i = 0; i < HISTORY_SIZE; i++) history[i][0] = '\0';
    history_count = 0; history_pos = -1;

    char cmd[CMD_MAX_LEN+1];
    int pos = 0;
    int prompt_row = 10;
    int prompt_col = 0;
    vga_print_at("LimCore> ", prompt_col, prompt_row, 0x0F);
    prompt_col += 9;

    while (1) {
        uint8_t sc = keyboard_read_scancode();
        char ch = scancode_to_ascii(sc);

        // стрелки
        if (sc == 0x48) {
            if (history_count > 0) {
                if (history_pos == -1) strcpy(saved_cmd, cmd);
                if (history_pos > 0) history_pos--;
                else if (history_pos == -1) history_pos = history_count - 1;
                if (history_pos >= 0) {
                    for (int i = 0; i < pos; i++)
                        vga_putchar_at(prompt_col - 1 - i, prompt_row, ' ', 0x0F);
                    strcpy(cmd, history[history_pos]);
                    pos = strlen(cmd);
                    prompt_col = 9;
                    vga_print_at(cmd, prompt_col, prompt_row, 0x0F);
                    prompt_col += pos;
                }
            }
            continue;
        } else if (sc == 0x50) {
            if (history_pos != -1) {
                if (history_pos < history_count - 1) {
                    history_pos++;
                    for (int i = 0; i < pos; i++)
                        vga_putchar_at(prompt_col - 1 - i, prompt_row, ' ', 0x0F);
                    strcpy(cmd, history[history_pos]);
                    pos = strlen(cmd);
                    prompt_col = 9;
                    vga_print_at(cmd, prompt_col, prompt_row, 0x0F);
                    prompt_col += pos;
                } else {
                    history_pos = -1;
                    for (int i = 0; i < pos; i++)
                        vga_putchar_at(prompt_col - 1 - i, prompt_row, ' ', 0x0F);
                    strcpy(cmd, saved_cmd);
                    pos = strlen(cmd);
                    prompt_col = 9;
                    vga_print_at(cmd, prompt_col, prompt_row, 0x0F);
                    prompt_col += pos;
                }
            }
            continue;
        }

        if (ch == '\n') {
            cmd[pos] = '\0';
            if (pos > 0 && (history_count == 0 || strcmp(cmd, history[history_count-1]) != 0)) {
                if (history_count < HISTORY_SIZE) strcpy(history[history_count++], cmd);
                else {
                    for (int i = 0; i < HISTORY_SIZE-1; i++) strcpy(history[i], history[i+1]);
                    strcpy(history[HISTORY_SIZE-1], cmd);
                }
            }
            history_pos = -1;
            pos = 0;
            prompt_row++; prompt_col = 0;

            char* args[8];
            int argc = split(cmd, args, 8);
            if (argc == 0) {}
            else if (strcmp(args[0],"help")==0) {
                vga_print_at("LimoOS v0.2 commands:\n", prompt_col, prompt_row, 0x0F); prompt_row++;
                vga_print_at("  help, cls, ver, echo, reboot, mem, calc, run\n", prompt_col, prompt_row, 0x0F); prompt_row++;
                vga_print_at("  ls, cd, pwd, mkdir, touch, write, read, rm, format\n", prompt_col, prompt_row, 0x0F); prompt_row++;
            }
            else if (strcmp(args[0],"cls")==0) {
                clear_with_logo();
                prompt_row = 10; prompt_col = 0;
            }
            else if (strcmp(args[0],"ver")==0) {
                vga_print_at("LimoOS v0.2 (LimCore kernel)\n", prompt_col, prompt_row, 0x0F);
                prompt_row++;
            }
            else if (strcmp(args[0],"echo")==0) {
                if (argc>1) {
                    char out[128]="";
                    for(int i=1;i<argc;i++){
                        if(i>1){int l=strlen(out); out[l]=' '; out[l+1]='\0';}
                        strcpy(out+strlen(out), args[i]);
                    }
                    vga_print_at(out, prompt_col, prompt_row, 0x0F);
                }
                vga_print_at("\n", prompt_col, prompt_row, 0x0F);
                prompt_row++;
            }
            else if (strcmp(args[0],"reboot")==0) {
                vga_print_at("Rebooting...\n", prompt_col, prompt_row, 0x0F);
                outb(0x64,0xFE);
                while(1);
            }
            else if (strcmp(args[0],"mem")==0) {
                vga_print_at("Total memory: 64 MB (static info)\n", prompt_col, prompt_row, 0x0F);
                prompt_row++;
            }
            else if (strcmp(args[0],"calc")==0) {
                if (argc!=4) {
                    vga_print_at("Usage: calc <a> <op> <b>\n", prompt_col, prompt_row, 0x0F);
                    prompt_row++;
                } else {
                    int a=atoi(args[1]), b=atoi(args[3]), res=0, ok=1;
                    char op=args[2][0];
                    switch(op){
                        case '+': res=a+b; break;
                        case '-': res=a-b; break;
                        case '*': res=a*b; break;
                        case '/': if(b) res=a/b; else ok=0; break;
                        default: ok=0;
                    }
                    if(!ok) vga_print_at("Error\n", prompt_col, prompt_row, 0x0F);
                    else {
                        char buf[16];
                        itoa(res,buf);
                        vga_print_at(buf, prompt_col, prompt_row, 0x0F);
                    }
                    vga_print_at("\n", prompt_col, prompt_row, 0x0F);
                    prompt_row++;
                }
            }
            else if (strcmp(args[0],"run")==0) {
                vga_print_at("Not implemented yet.\n", prompt_col, prompt_row, 0x0F);
                prompt_row++;
            }                
else if (strcmp(args[0],"ls")==0) {
    int lines = vfs_ls(argc>1 ? args[1] : NULL, prompt_row, prompt_col);
    if (lines > 0) {
        prompt_row += lines;
        prompt_col = 0;
    } else {
        prompt_row++;
        prompt_col = 0;
    }
}
            else if (strcmp(args[0],"cd")==0) {
                if (argc<2) vfs_cd("/");
                else if (vfs_cd(args[1])!=0) {
                    vga_print_at("Directory not found\n", prompt_col, prompt_row, 0x0F);
                    prompt_row++;
                }
            }
            else if (strcmp(args[0],"pwd")==0) {
                vga_print_at(vfs_get_cwd(), prompt_col, prompt_row, 0x0F);
                vga_print_at("\n", prompt_col, prompt_row, 0x0F);
                prompt_row++;
            }
            else if (strcmp(args[0],"mkdir")==0) {
                if (argc<2) vga_print_at("Usage: mkdir <dir>\n", prompt_col, prompt_row, 0x0F);
                else if (vfs_mkdir(args[1])!=0) vga_print_at("Failed\n", prompt_col, prompt_row, 0x0F);
                else vga_print_at("OK\n", prompt_col, prompt_row, 0x0F);
                prompt_row++;
            }
            else if (strcmp(args[0],"touch")==0) {
                if (argc<2) vga_print_at("Usage: touch <file>\n", prompt_col, prompt_row, 0x0F);
                else if (vfs_touch(args[1])!=0) vga_print_at("Failed\n", prompt_col, prompt_row, 0x0F);
                else vga_print_at("OK\n", prompt_col, prompt_row, 0x0F);
                prompt_row++;
            }
            else if (strcmp(args[0],"write")==0) {
                if (argc<3) vga_print_at("Usage: write <file> <text>\n", prompt_col, prompt_row, 0x0F);
                else if (vfs_write(args[1], args[2])!=0) vga_print_at("Failed\n", prompt_col, prompt_row, 0x0F);
                else vga_print_at("OK\n", prompt_col, prompt_row, 0x0F);
                prompt_row++;
            }
            else if (strcmp(args[0],"read")==0) {
                if (argc<2) vga_print_at("Usage: read <file>\n", prompt_col, prompt_row, 0x0F);
                else {
                    char buf[1024];
                    int n = vfs_read(args[1], buf, sizeof(buf)-1);
                    if (n<0) vga_print_at("Failed\n", prompt_col, prompt_row, 0x0F);
                    else {
                        vga_print_at(buf, prompt_col, prompt_row, 0x0F);
                        vga_print_at("\n", prompt_col, prompt_row, 0x0F);
                    }
                }
                prompt_row++;
            }
            else if (strcmp(args[0],"rm")==0) {
                if (argc<2) vga_print_at("Usage: rm <file/dir>\n", prompt_col, prompt_row, 0x0F);
                else if (vfs_rm(args[1])!=0) vga_print_at("Failed\n", prompt_col, prompt_row, 0x0F);
                else vga_print_at("OK\n", prompt_col, prompt_row, 0x0F);
                prompt_row++;
            }
            else if (strcmp(args[0],"format")==0) {
                vfs_format();
                vga_print_at("Virtual disk formatted.\n", prompt_col, prompt_row, 0x0F);
                prompt_row++;
            }
            else if (cmd[0]!='\0') {
                vga_print_at("Unknown command: ", prompt_col, prompt_row, 0x0F);
                vga_print_at(cmd, prompt_col+17, prompt_row, 0x0F);
                vga_print_at("\n", prompt_col+17+strlen(cmd), prompt_row, 0x0F);
                prompt_row++;
            }

            vga_print_at("LimCore> ", prompt_col, prompt_row, 0x0F);
            prompt_col += 9;
        }
        else if (ch == '\b') {
            if (pos>0 && prompt_col>0) {
                pos--; prompt_col--;
                vga_putchar_at(prompt_col, prompt_row, ' ', 0x0F);
            }
        }
        else if (ch != 0) {
            if (pos < CMD_MAX_LEN) {
                cmd[pos++] = ch;
                vga_putchar_at(prompt_col, prompt_row, ch, 0x0F);
                prompt_col++;
            }
        }
    }
}