#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

typedef enum {
    UI_COLOR_BLACK = 0,
    UI_COLOR_BLUE = 1,
    UI_COLOR_GREEN = 2,
    UI_COLOR_CYAN = 3,
    UI_COLOR_RED = 4,
    UI_COLOR_MAGENTA = 5,
    UI_COLOR_BROWN = 6,
    UI_COLOR_LIGHT_GREY = 7,
    UI_COLOR_DARK_GREY = 8,
    UI_COLOR_LIGHT_BLUE = 9,
    UI_COLOR_LIGHT_GREEN = 10,
    UI_COLOR_LIGHT_CYAN = 11,
    UI_COLOR_LIGHT_RED = 12,
    UI_COLOR_LIGHT_MAGENTA = 13,
    UI_COLOR_LIGHT_BROWN = 14,
    UI_COLOR_WHITE = 15
} ui_color_t;

static uint16_t* const ui_buffer = (uint16_t*) VGA_MEMORY;
static size_t ui_row = 0;
static size_t ui_col = 0;
static uint8_t ui_color = 0x0F;

static inline uint8_t ui_make_color(ui_color_t fg, ui_color_t bg) {
    return (uint8_t) fg | (uint8_t) bg << 4;
}

static inline uint16_t ui_make_entry(unsigned char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}

void ui_set_color(ui_color_t fg, ui_color_t bg) {
    ui_color = ui_make_color(fg, bg);
}

void ui_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            ui_buffer[y * VGA_WIDTH + x] = ui_make_entry(' ', ui_color);
        }
    }
    ui_row = 0;
    ui_col = 0;
}

static void ui_scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            ui_buffer[(y - 1) * VGA_WIDTH + x] = ui_buffer[y * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        ui_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = ui_make_entry(' ', ui_color);
    }
    ui_row = VGA_HEIGHT - 1;
}

void ui_putchar_at(char c, size_t x, size_t y) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        return;
    }
    ui_buffer[y * VGA_WIDTH + x] = ui_make_entry((unsigned char) c, ui_color);
}

void ui_putchar(char c) {
    if (c == '\n') {
        ui_col = 0;
        ui_row++;
    } else {
        ui_putchar_at(c, ui_col, ui_row);
        ui_col++;
        if (ui_col == VGA_WIDTH) {
            ui_col = 0;
            ui_row++;
        }
    }

    if (ui_row == VGA_HEIGHT) {
        ui_scroll();
    }
}

void ui_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        ui_putchar(data[i]);
    }
}

static size_t ui_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

void ui_print(const char* data) {
    ui_write(data, ui_strlen(data));
}

void ui_draw_box(size_t x, size_t y, size_t width, size_t height) {
    for (size_t i = 0; i < width; i++) {
        ui_putchar_at('-', x + i, y);
        ui_putchar_at('-', x + i, y + height - 1);
    }
    for (size_t i = 0; i < height; i++) {
        ui_putchar_at('|', x, y + i);
        ui_putchar_at('|', x + width - 1, y + i);
    }
    ui_putchar_at('+', x, y);
    ui_putchar_at('+', x + width - 1, y);
    ui_putchar_at('+', x, y + height - 1);
    ui_putchar_at('+', x + width - 1, y + height - 1);
}

void ui_move_cursor(size_t x, size_t y) {
    if (x < VGA_WIDTH && y < VGA_HEIGHT) {
        ui_col = x;
        ui_row = y;
    }
}

void kernel_main(void) {
    ui_set_color(UI_COLOR_LIGHT_BROWN, UI_COLOR_BLACK);
    ui_clear();

    ui_draw_box(0, 0, VGA_WIDTH, 3);
    ui_move_cursor(2, 1);
    ui_set_color(UI_COLOR_WHITE, UI_COLOR_BLACK);
    ui_print("Ember OS");

    ui_move_cursor(0, 4);
    ui_set_color(UI_COLOR_LIGHT_GREY, UI_COLOR_BLACK);
    ui_print("Kernel loaded successfully.\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
