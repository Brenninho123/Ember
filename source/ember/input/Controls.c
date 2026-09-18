#include <stdint.h>
#include <stddef.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_BUFFER_SIZE 256

typedef enum {
    KEY_STATE_RELEASED = 0,
    KEY_STATE_PRESSED = 1
} key_state_t;

static char scancode_to_ascii[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static char scancode_to_ascii_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static char key_buffer[KEYBOARD_BUFFER_SIZE];
static size_t buffer_head = 0;
static size_t buffer_tail = 0;

static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;
static int caps_lock = 0;

static key_state_t key_states[128];

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void buffer_push(char c) {
    size_t next = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != buffer_tail) {
        key_buffer[buffer_head] = c;
        buffer_head = next;
    }
}

void controls_init(void) {
    for (int i = 0; i < 128; i++) {
        key_states[i] = KEY_STATE_RELEASED;
    }
    buffer_head = 0;
    buffer_tail = 0;
    shift_pressed = 0;
    ctrl_pressed = 0;
    alt_pressed = 0;
    caps_lock = 0;
}

static char apply_caps(char c) {
    if (caps_lock && c >= 'a' && c <= 'z') {
        return c - 32;
    }
    if (caps_lock && c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

void controls_handle_scancode(uint8_t scancode) {
    int released = scancode & 0x80;
    uint8_t code = scancode & 0x7F;

    if (code >= 128) {
        return;
    }

    key_states[code] = released ? KEY_STATE_RELEASED : KEY_STATE_PRESSED;

    switch (code) {
        case 0x2A:
        case 0x36:
            shift_pressed = released ? 0 : 1;
            return;
        case 0x1D:
            ctrl_pressed = released ? 0 : 1;
            return;
        case 0x38:
            alt_pressed = released ? 0 : 1;
            return;
        case 0x3A:
            if (!released) {
                caps_lock = !caps_lock;
            }
            return;
        default:
            break;
    }

    if (released) {
        return;
    }

    char ascii = shift_pressed ? scancode_to_ascii_shift[code] : scancode_to_ascii[code];
    if (ascii == 0) {
        return;
    }

    if (!shift_pressed) {
        ascii = apply_caps(ascii);
    }

    buffer_push(ascii);
}

void controls_poll(void) {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    if (status & 0x01) {
        uint8_t scancode = inb(KEYBOARD_DATA_PORT);
        controls_handle_scancode(scancode);
    }
}

int controls_has_key(void) {
    return buffer_head != buffer_tail;
}

char controls_get_key(void) {
    if (buffer_head == buffer_tail) {
        return 0;
    }
    char c = key_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

int controls_is_key_pressed(uint8_t scancode) {
    if (scancode >= 128) {
        return 0;
    }
    return key_states[scancode] == KEY_STATE_PRESSED;
}

int controls_is_shift_pressed(void) {
    return shift_pressed;
}

int controls_is_ctrl_pressed(void) {
    return ctrl_pressed;
}

int controls_is_alt_pressed(void) {
    return alt_pressed;
}
