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

#define EMBER_VERSION_MAJOR 0
#define EMBER_VERSION_MINOR 1
#define EMBER_VERSION_PATCH 0

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
static uint8_t term_color_stack[16];
static int term_color_stack_top = 0;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t make_color(vga_color_t fg, vga_color_t bg) {
    return (uint8_t) fg | (uint8_t) bg << 4;
}

static inline uint16_t make_entry(unsigned char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}

void* mem_set(void* dest, int value, size_t count) {
    uint8_t* d = (uint8_t*) dest;
    for (size_t i = 0; i < count; i++) {
        d[i] = (uint8_t) value;
    }
    return dest;
}

void* mem_copy(void* dest, const void* src, size_t count) {
    uint8_t* d = (uint8_t*) dest;
    const uint8_t* s = (const uint8_t*) src;
    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
    return dest;
}

int str_cmp(const char* a, const char* b) {
    size_t i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) {
            return a[i] - b[i];
        }
        i++;
    }
    return a[i] - b[i];
}

size_t str_len(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

char* str_copy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

static void update_cursor(void) {
    uint16_t pos = (uint16_t)(term_row * VGA_WIDTH + term_col);
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_LOW);
    outb(VGA_DATA_REGISTER, (uint8_t)(pos & 0xFF));
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_HIGH);
    outb(VGA_DATA_REGISTER, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_hide_cursor(void) {
    outb(VGA_CTRL_REGISTER, 0x0A);
    outb(VGA_DATA_REGISTER, 0x20);
}

void terminal_show_cursor(uint8_t start, uint8_t end) {
    outb(VGA_CTRL_REGISTER, 0x0A);
    outb(VGA_DATA_REGISTER, start);
    outb(VGA_CTRL_REGISTER, 0x0B);
    outb(VGA_DATA_REGISTER, end);
}

void terminal_set_color(vga_color_t fg, vga_color_t bg) {
    term_color = make_color(fg, bg);
}

void terminal_push_color(vga_color_t fg, vga_color_t bg) {
    if (term_color_stack_top < 16) {
        term_color_stack[term_color_stack_top++] = term_color;
    }
    term_color = make_color(fg, bg);
}

void terminal_pop_color(void) {
    if (term_color_stack_top > 0) {
        term_color = term_color_stack[--term_color_stack_top];
    }
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

void terminal_clear_line(size_t y) {
    if (y >= VGA_HEIGHT) {
        return;
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[y * VGA_WIDTH + x] = make_entry(' ', term_color);
    }
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

size_t terminal_get_row(void) {
    return term_row;
}

size_t terminal_get_col(void) {
    return term_col;
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

void terminal_putchar_at(char c, size_t x, size_t y) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        return;
    }
    vga_buffer[y * VGA_WIDTH + x] = make_entry((unsigned char) c, term_color);
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}

void terminal_writestring(const char* data) {
    terminal_write(data, str_len(data));
}

void terminal_write_centered(const char* data, size_t y) {
    size_t len = str_len(data);
    size_t x = len < VGA_WIDTH ? (VGA_WIDTH - len) / 2 : 0;
    for (size_t i = 0; i < len && x + i < VGA_WIDTH; i++) {
        terminal_putchar_at(data[i], x + i, y);
    }
}

void terminal_draw_hline(size_t y, char c) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_putchar_at(c, x, y);
    }
}

void terminal_draw_box(size_t x, size_t y, size_t width, size_t height) {
    for (size_t i = 0; i < width; i++) {
        terminal_putchar_at('-', x + i, y);
        terminal_putchar_at('-', x + i, y + height - 1);
    }
    for (size_t i = 0; i < height; i++) {
        terminal_putchar_at('|', x, y + i);
        terminal_putchar_at('|', x + width - 1, y + i);
    }
    terminal_putchar_at('+', x, y);
    terminal_putchar_at('+', x + width - 1, y);
    terminal_putchar_at('+', x, y + height - 1);
    terminal_putchar_at('+', x + width - 1, y + height - 1);
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

void terminal_print_banner(void) {
    terminal_push_color(COLOR_LIGHT_BROWN, COLOR_BLACK);
    terminal_draw_hline(0, '=');
    terminal_write_centered("Ember OS", 1);
    terminal_printf("Version %d.%d.%d\n", EMBER_VERSION_MAJOR, EMBER_VERSION_MINOR, EMBER_VERSION_PATCH);
    terminal_draw_hline(2, '=');
    terminal_pop_color();
    terminal_set_cursor(0, 3);
}

void terminal_panic(const char* message) {
    terminal_set_color(COLOR_WHITE, COLOR_RED);
    terminal_clear();
    terminal_write_centered("KERNEL PANIC", 10);
    terminal_set_cursor(0, 12);
    terminal_printf("  %s\n", message);
    __asm__ volatile ("cli");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
