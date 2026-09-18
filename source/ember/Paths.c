#include <stddef.h>

#define PATH_MAX_LENGTH 256
#define PATH_SEPARATOR '/'

static size_t path_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

static void path_strcpy(char* dest, const char* src) {
    size_t i = 0;
    while (src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

int path_is_absolute(const char* path) {
    if (!path || path[0] == '\0') {
        return 0;
    }
    return path[0] == PATH_SEPARATOR;
}

void path_join(char* result, const char* base, const char* sub) {
    size_t base_len = path_strlen(base);
    size_t sub_len = path_strlen(sub);
    size_t pos = 0;

    for (size_t i = 0; i < base_len && pos < PATH_MAX_LENGTH - 1; i++) {
        result[pos++] = base[i];
    }

    if (pos > 0 && result[pos - 1] != PATH_SEPARATOR && pos < PATH_MAX_LENGTH - 1) {
        result[pos++] = PATH_SEPARATOR;
    }

    for (size_t i = 0; i < sub_len && pos < PATH_MAX_LENGTH - 1; i++) {
        result[pos++] = sub[i];
    }

    result[pos] = '\0';
}

void path_basename(char* result, const char* path) {
    size_t len = path_strlen(path);
    if (len == 0) {
        result[0] = '\0';
        return;
    }

    size_t end = len;
    while (end > 0 && path[end - 1] == PATH_SEPARATOR) {
        end--;
    }

    size_t start = end;
    while (start > 0 && path[start - 1] != PATH_SEPARATOR) {
        start--;
    }

    size_t pos = 0;
    for (size_t i = start; i < end && pos < PATH_MAX_LENGTH - 1; i++) {
        result[pos++] = path[i];
    }
    result[pos] = '\0';
}

void path_dirname(char* result, const char* path) {
    size_t len = path_strlen(path);
    if (len == 0) {
        path_strcpy(result, ".");
        return;
    }

    size_t end = len;
    while (end > 0 && path[end - 1] == PATH_SEPARATOR) {
        end--;
    }

    while (end > 0 && path[end - 1] != PATH_SEPARATOR) {
        end--;
    }

    while (end > 1 && path[end - 1] == PATH_SEPARATOR) {
        end--;
    }

    if (end == 0) {
        if (path[0] == PATH_SEPARATOR) {
            result[0] = PATH_SEPARATOR;
            result[1] = '\0';
        } else {
            path_strcpy(result, ".");
        }
        return;
    }

    size_t pos = 0;
    for (size_t i = 0; i < end && pos < PATH_MAX_LENGTH - 1; i++) {
        result[pos++] = path[i];
    }
    result[pos] = '\0';
}

int path_extension(char* result, const char* path) {
    size_t len = path_strlen(path);
    size_t dot_pos = len;
    int found = 0;

    for (size_t i = len; i > 0; i--) {
        if (path[i - 1] == PATH_SEPARATOR) {
            break;
        }
        if (path[i - 1] == '.') {
            dot_pos = i - 1;
            found = 1;
            break;
        }
    }

    if (!found) {
        result[0] = '\0';
        return 0;
    }

    size_t pos = 0;
    for (size_t i = dot_pos + 1; i < len && pos < PATH_MAX_LENGTH - 1; i++) {
        result[pos++] = path[i];
    }
    result[pos] = '\0';
    return 1;
}

void path_normalize(char* result, const char* path) {
    size_t len = path_strlen(path);
    size_t pos = 0;
    int last_was_sep = 0;

    for (size_t i = 0; i < len && pos < PATH_MAX_LENGTH - 1; i++) {
        char c = path[i];
        if (c == PATH_SEPARATOR) {
            if (last_was_sep) {
                continue;
            }
            last_was_sep = 1;
        } else {
            last_was_sep = 0;
        }
        result[pos++] = c;
    }

    if (pos > 1 && result[pos - 1] == PATH_SEPARATOR) {
        pos--;
    }

    result[pos] = '\0';
}
