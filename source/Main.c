#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

#define VGA_CTRL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5
#define VGA_CURSOR_HIGH 0x0E
#define VGA_CURSOR_LOW 0x0F

typedef enum {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GREY = 7,
    COLOR_DARK_GREY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_LIGHT_BROWN = 14,
    COLOR_WHITE = 15
} vga_color_t;

static uint16_t* const vga_buffer = (uint16_t*) VGA_MEMORY;
static size_t term_row = 0;
static size_t term_col = 0;
static uint8_t term_color = 0x0F;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t make_color(vga_color_t fg, vga_color_t bg) {
    return (uint8_t) fg | (uint8_t) bg << 4;
}

static inline uint16_t make_entry(unsigned char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}

static size_t str_len(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

static void update_cursor(void) {
    uint16_t pos = (uint16_t)(term_row * VGA_WIDTH + term_col);
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_LOW);
    outb(VGA_DATA_REGISTER, (uint8_t)(pos & 0xFF));
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_HIGH);
    outb(VGA_DATA_REGISTER, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_set_color(vga_color_t fg, vga_color_t bg) {
    term_color = make_color(fg, bg);
}

void terminal_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = make_entry(' ', term_color);
        }
    }
    term_row = 0;
    term_col = 0;
    update_cursor();
}

static void terminal_scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(y - 1) * VGA_WIDTH + x] = vga_buffer[y * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = make_entry(' ', term_color);
    }
    term_row = VGA_HEIGHT - 1;
}

void terminal_set_cursor(size_t x, size_t y) {
    if (x < VGA_WIDTH && y < VGA_HEIGHT) {
        term_col = x;
        term_row = y;
        update_cursor();
    }
}

void terminal_putchar(char c) {
    switch (c) {
        case '\n':
            term_col = 0;
            term_row++;
            break;
        case '\r':
            term_col = 0;
            break;
        case '\t':
            term_col = (term_col + 4) & ~(size_t)3;
            break;
        case '\b':
            if (term_col > 0) {
                term_col--;
                vga_buffer[term_row * VGA_WIDTH + term_col] = make_entry(' ', term_color);
            }
            break;
        default:
            vga_buffer[term_row * VGA_WIDTH + term_col] = make_entry((unsigned char) c, term_color);
            term_col++;
            break;
    }

    if (term_col >= VGA_WIDTH) {
        term_col = 0;
        term_row++;
    }

    if (term_row >= VGA_HEIGHT) {
        terminal_scroll();
    }

    update_cursor();
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}

void terminal_writestring(const char* data) {
    terminal_write(data, str_len(data));
}

static void print_uint(unsigned int value, unsigned int base, int uppercase) {
    char buffer[32];
    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int pos = 0;

    if (value == 0) {
        terminal_putchar('0');
        return;
    }

    while (value > 0) {
        buffer[pos++] = digits[value % base];
        value /= base;
    }

    while (pos > 0) {
        terminal_putchar(buffer[--pos]);
    }
}

static void print_int(int value) {
    if (value < 0) {
        terminal_putchar('-');
        print_uint((unsigned int)(-value), 10, 0);
    } else {
        print_uint((unsigned int) value, 10, 0);
    }
}

void terminal_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (size_t i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] != '%') {
            terminal_putchar(fmt[i]);
            continue;
        }

        i++;
        switch (fmt[i]) {
            case 'd': {
                int value = va_arg(args, int);
                print_int(value);
                break;
            }
            case 'u': {
                unsigned int value = va_arg(args, unsigned int);
                print_uint(value, 10, 0);
                break;
            }
            case 'x': {
                unsigned int value = va_arg(args, unsigned int);
                print_uint(value, 16, 0);
                break;
            }
            case 'X': {
                unsigned int value = va_arg(args, unsigned int);
                print_uint(value, 16, 1);
                break;
            }
            case 'c': {
                char value = (char) va_arg(args, int);
                terminal_putchar(value);
                break;
            }
            case 's': {
                const char* value = va_arg(args, const char*);
                terminal_writestring(value);
                break;
            }
            case '%': {
                terminal_putchar('%');
                break;
            }
            default: {
                terminal_putchar('%');
                terminal_putchar(fmt[i]);
                break;
            }
        }
    }

    va_end(args);
}
