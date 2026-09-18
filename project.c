#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

static uint16_t* const vga_buffer = (uint16_t*) VGA_MEMORY;
static size_t vga_row = 0;
static size_t vga_col = 0;
static uint8_t vga_color = 0x0F;

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}

void terminal_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', vga_color);
        }
    }
    vga_row = 0;
    vga_col = 0;
}

void terminal_setcolor(uint8_t color) {
    vga_color = color;
}

void terminal_scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(y - 1) * VGA_WIDTH + x] = vga_buffer[y * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', vga_color);
    }
    vga_row = VGA_HEIGHT - 1;
}

void terminal_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else {
        const size_t index = vga_row * VGA_WIDTH + vga_col;
        vga_buffer[index] = vga_entry(c, vga_color);
        vga_col++;
        if (vga_col == VGA_WIDTH) {
            vga_col = 0;
            vga_row++;
        }
    }

    if (vga_row == VGA_HEIGHT) {
        terminal_scroll();
    }
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}

size_t strlen_custom(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen_custom(data));
}

void kernel_main(void) {
    terminal_setcolor(vga_entry_color(14, 0));
    terminal_clear();
    terminal_writestring("Ember OS\n");
    terminal_setcolor(vga_entry_color(15, 0));
    terminal_writestring("Kernel loaded successfully.\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
