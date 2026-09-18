#include <stddef.h>
#include <stdint.h>

#define CONTENT_MAX_ENTRIES 128
#define CONTENT_MAX_NAME 256
#define CONTENT_MAX_SIZE 4096

typedef struct {
    char name[CONTENT_MAX_NAME];
    uint8_t data[CONTENT_MAX_SIZE];
    size_t size;
    int used;
} content_entry_t;

static content_entry_t content_table[CONTENT_MAX_ENTRIES];
static int content_count = 0;

static size_t content_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

static int content_strcmp(const char* a, const char* b) {
    size_t i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) {
            return a[i] - b[i];
        }
        i++;
    }
    return a[i] - b[i];
}

static void content_strcpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void content_init(void) {
    for (int i = 0; i < CONTENT_MAX_ENTRIES; i++) {
        content_table[i].used = 0;
        content_table[i].size = 0;
    }
    content_count = 0;
}

static int content_find(const char* name) {
    for (int i = 0; i < CONTENT_MAX_ENTRIES; i++) {
        if (content_table[i].used && content_strcmp(content_table[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int content_find_free(void) {
    for (int i = 0; i < CONTENT_MAX_ENTRIES; i++) {
        if (!content_table[i].used) {
            return i;
        }
    }
    return -1;
}

int content_create(const char* name) {
    if (content_strlen(name) >= CONTENT_MAX_NAME) {
        return -1;
    }

    if (content_find(name) >= 0) {
        return -1;
    }

    int slot = content_find_free();
    if (slot < 0) {
        return -1;
    }

    content_strcpy(content_table[slot].name, name, CONTENT_MAX_NAME);
    content_table[slot].size = 0;
    content_table[slot].used = 1;
    content_count++;

    return slot;
}

int content_write(const char* name, const uint8_t* data, size_t size) {
    int slot = content_find(name);
    if (slot < 0) {
        slot = content_create(name);
        if (slot < 0) {
            return -1;
        }
    }

    if (size > CONTENT_MAX_SIZE) {
        size = CONTENT_MAX_SIZE;
    }

    for (size_t i = 0; i < size; i++) {
        content_table[slot].data[i] = data[i];
    }

    content_table[slot].size = size;
    return (int) size;
}

int content_append(const char* name, const uint8_t* data, size_t size) {
    int slot = content_find(name);
    if (slot < 0) {
        slot = content_create(name);
        if (slot < 0) {
            return -1;
        }
    }

    size_t space = CONTENT_MAX_SIZE - content_table[slot].size;
    if (size > space) {
        size = space;
    }

    for (size_t i = 0; i < size; i++) {
        content_table[slot].data[content_table[slot].size + i] = data[i];
    }

    content_table[slot].size += size;
    return (int) size;
}

int content_read(const char* name, uint8_t* out, size_t max_size) {
    int slot = content_find(name);
    if (slot < 0) {
        return -1;
    }

    size_t size = content_table[slot].size;
    if (size > max_size) {
        size = max_size;
    }

    for (size_t i = 0; i < size; i++) {
        out[i] = content_table[slot].data[i];
    }

    return (int) size;
}

int content_size(const char* name) {
    int slot = content_find(name);
    if (slot < 0) {
        return -1;
    }
    return (int) content_table[slot].size;
}

int content_exists(const char* name) {
    return content_find(name) >= 0;
}

int content_delete(const char* name) {
    int slot = content_find(name);
    if (slot < 0) {
        return -1;
    }

    content_table[slot].used = 0;
    content_table[slot].size = 0;
    content_count--;

    return 0;
}

int content_rename(const char* old_name, const char* new_name) {
    int slot = content_find(old_name);
    if (slot < 0) {
        return -1;
    }

    if (content_find(new_name) >= 0) {
        return -1;
    }

    if (content_strlen(new_name) >= CONTENT_MAX_NAME) {
        return -1;
    }

    content_strcpy(content_table[slot].name, new_name, CONTENT_MAX_NAME);
    return 0;
}

int content_list(char names_out[][CONTENT_MAX_NAME], int max_entries) {
    int count = 0;
    for (int i = 0; i < CONTENT_MAX_ENTRIES && count < max_entries; i++) {
        if (content_table[i].used) {
            content_strcpy(names_out[count], content_table[i].name, CONTENT_MAX_NAME);
            count++;
        }
    }
    return count;
}

int content_get_count(void) {
    return content_count;
}
