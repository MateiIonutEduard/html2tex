#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#define STRING_BUFFER_MIN_CAPACITY 64
#define STRING_BUFFER_GROWTH_FACTOR 2
#define STRING_BUFFER_MIN_GROW 32

static int string_buffer_grow(StringBuffer* buf, size_t min_capacity) {
    if (!buf || buf->error) return -1;
    size_t new_capacity = buf->capacity;

    /* handle initial allocation */
    if (new_capacity == 0) {
        new_capacity = (min_capacity > STRING_BUFFER_MIN_CAPACITY)
            ? min_capacity
            : STRING_BUFFER_MIN_CAPACITY;
    }

    /* exponential growth with minimum increment */
    while (new_capacity <= min_capacity) {
        if (new_capacity > SIZE_MAX / STRING_BUFFER_GROWTH_FACTOR) {
            if (min_capacity > SIZE_MAX - STRING_BUFFER_MIN_GROW) {
                buf->error = 1;
                return -1;
            }

            new_capacity = min_capacity + STRING_BUFFER_MIN_GROW;
        }
        else
            new_capacity *= STRING_BUFFER_GROWTH_FACTOR;
    }

    char* new_data = (char*)realloc(buf->data, new_capacity);

    if (!new_data) {
        buf->error = 1;
        return -1;
    }

    buf->data = new_data;
    buf->capacity = new_capacity;
    return 0;
}

StringBuffer* string_buffer_create(size_t initial_capacity) {
    StringBuffer* buf = (StringBuffer*)calloc(1, sizeof(StringBuffer));
    if (!buf) return NULL;

    buf->capacity = (initial_capacity > 0) ? initial_capacity : STRING_BUFFER_MIN_CAPACITY;
    buf->data = (char*)malloc(buf->capacity);

    if (!buf->data) {
        free(buf);
        return NULL;
    }

    buf->data[0] = '\0';
    buf->length = 0;
    buf->error = 0;
    return buf;
}

void string_buffer_destroy(StringBuffer* buf) {
    if (!buf) return;
    free(buf->data);
    free(buf);
}

int string_buffer_clear(StringBuffer* buf) {
    if (!buf || buf->error) return -1;
    buf->length = 0;

    if (buf->capacity > 0 && buf->data)
        buf->data[0] = '\0';

    return 0;
}

char* string_buffer_detach(StringBuffer* buf) {
    if (!buf || buf->error) return NULL;

    /* shrink to exact size */
    char* result = (char*)realloc(buf->data, buf->length + 1);

    if (!result) {
        buf->error = 1;
        return NULL;
    }

    /* reset buffer to initial state */
    buf->data = (char*)malloc(STRING_BUFFER_MIN_CAPACITY);

    if (buf->data) {
        buf->data[0] = '\0';
        buf->capacity = STRING_BUFFER_MIN_CAPACITY;
    }
    else {
        buf->capacity = 0;
        buf->error = 1;
    }

    buf->length = 0;
    return result;
}

int string_buffer_append(StringBuffer* buf, const char* str, size_t len) {
    if (!buf || buf->error) return -1;
    if (!str) return 0;

    /* compute the length if not provided */
    if (len == 0) {
        len = strlen(str);
        if (len == 0) return 0;
    }

    /* ensure capacity, including null terminator */
    if (string_buffer_ensure_capacity(buf, buf->length + len + 1) != 0)
        return -1;

    /* copy data to the memory */
    memcpy(buf->data + buf->length, str, len);

    buf->length += len;
    buf->data[buf->length] = '\0';
    return 0;
}

int string_buffer_append_char(StringBuffer* buf, char c) {
    if (!buf || buf->error) return -1;

    /* ensure capacity for char + null */
    if (string_buffer_ensure_capacity(buf, buf->length + 2) != 0)
        return -1;

    buf->data[buf->length++] = c;
    buf->data[buf->length] = '\0';
    return 0;
}

int string_buffer_append_printf(StringBuffer* buf, const char* format, ...) {
    if (!buf || buf->error || !format) return -1;
    va_list args;
    va_start(args, format);

    /* determine required size */
    va_list args_copy;
    va_copy(args_copy, args);

    int needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (needed < 0) {
        va_end(args);
        buf->error = 1;
        return -1;
    }

    /* ensure capacity */
    if (string_buffer_ensure_capacity(buf, buf->length + needed + 1) != 0) {
        va_end(args);
        return -1;
    }

    /* actual formatting */
    int written = vsnprintf(buf->data + buf->length,
        buf->capacity - buf->length,
        format, args);
    va_end(args);

    if (written < 0 || (size_t)written >= buf->capacity - buf->length) {
        buf->error = 1;
        return -1;
    }

    buf->length += written;
    return written;
}

int string_buffer_append_latex(StringBuffer* buf, const char* str) {
    if (!buf || buf->error || !str) return -1;

    static const unsigned char escape_map[256] = {
        ['\\'] = 1,['{'] = 2,['}'] = 3,['&'] = 4,
        ['%'] = 5,['$'] = 6,['#'] = 7,['_'] = 8,
        ['^'] = 9,['~'] = 10,['<'] = 11,['>'] = 12,
        ['\n'] = 13
    };

    static const char* const escape_seq[] = {
        NULL, "\\textbackslash{}", "\\{", "\\}", "\\&",
        "\\%", "\\$", "\\#", "\\_", "\\^{}", "\\~{}",
        "\\textless{}", "\\textgreater{}", "\\\\"
    };

    const char* p = str;
    const char* start = p;

    while (*p) {
        unsigned char c = (unsigned char)*p;
        unsigned char type = escape_map[c];

        if (type == 0) {
            p++;
            continue;
        }

        /* Copy normal characters before escape sequence */
        if (p > start) {
            size_t normal_len = p - start;
            if (string_buffer_append(buf, start, normal_len) != 0) {
                return -1;
            }
        }

        /* append escape sequence */
        if (string_buffer_append(buf, escape_seq[type], 0) != 0)
            return -1;

        p++;
        start = p;
    }

    /* copy any remaining characters */
    if (p > start) {
        size_t remaining_len = p - start;
        if (string_buffer_append(buf, start, remaining_len) != 0)
            return -1;
    }

    return 0;
}

const char* string_buffer_cstr(const StringBuffer* buf) {
    if (!buf || buf->error) return "";
    return buf->data ? buf->data : "";
}

size_t string_buffer_length(const StringBuffer* buf) {
    if (!buf || buf->error) return 0;
    return buf->length;
}

int string_buffer_has_error(const StringBuffer* buf) {
    if (!buf) return -1;
    return buf->error ? 1 : 0;
}

size_t string_buffer_remaining(const StringBuffer* buf) {
    if (!buf || buf->error || buf->capacity == 0) return 0;
    return buf->capacity - buf->length - 1;
}

int string_buffer_ensure_capacity(StringBuffer* buf, size_t needed) {
    if (!buf || buf->error) return -1;
    if (needed < buf->capacity) return 0;
    return string_buffer_grow(buf, needed);
}

int string_buffer_shrink_to_fit(StringBuffer* buf) {
    if (!buf || buf->error) return -1;
    size_t optimal_capacity = buf->length + 1;

    if (optimal_capacity < STRING_BUFFER_MIN_CAPACITY)
        optimal_capacity = STRING_BUFFER_MIN_CAPACITY;

    if (buf->capacity <= optimal_capacity) return 0;
    char* new_data = (char*)realloc(buf->data, optimal_capacity);
    if (!new_data) return 0;

    buf->data = new_data;
    buf->capacity = optimal_capacity;
    return 0;
}