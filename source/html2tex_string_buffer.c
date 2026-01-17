#include "string_buffer.h"
#include "html2tex_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>

#define STRING_BUFFER_MIN_CAPACITY 64
#define STRING_BUFFER_GROWTH_FACTOR 2
#define STRING_BUFFER_MIN_GROW 32
#define STRING_BUFFER_MAX_CAPACITY (SIZE_MAX / 2)

static int string_buffer_grow(StringBuffer* buf, size_t min_capacity) {
    /* clear previous errors */
    html2tex_err_clear();

    /* validate input parameters */
    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for growth operation.");
        return -1;
    }

    /* check buffer error state */
    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return -1;
    }

    /* validate capacity request */
    if (min_capacity == 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_INVAL, "Invalid capacity request: 0 bytes.");
        return -1;
    }

    if (min_capacity > STRING_BUFFER_MAX_CAPACITY) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "Requested capacity %zu exceeds maximum %zu.",
            min_capacity, STRING_BUFFER_MAX_CAPACITY);
        buf->error = 1;
        return -1;
    }

    size_t new_capacity = buf->capacity;

    /* handle initial allocation */
    if (new_capacity == 0) {
        new_capacity = (min_capacity > STRING_BUFFER_MIN_CAPACITY)
            ? min_capacity
            : STRING_BUFFER_MIN_CAPACITY;
    }

    /* exponential growth with overflow protection */
    while (new_capacity <= min_capacity) {
        if (new_capacity > SIZE_MAX / STRING_BUFFER_GROWTH_FACTOR) {
            /* linear growth when exponential would overflow */
            if (min_capacity > SIZE_MAX - STRING_BUFFER_MIN_GROW) {
                HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                    "Capacity calculation overflow for request %zu.", min_capacity);
                buf->error = 1;
                return -1;
            }

            new_capacity = min_capacity + STRING_BUFFER_MIN_GROW;
            break;
        }
        else {
            new_capacity *= STRING_BUFFER_GROWTH_FACTOR;
        }

        /* safety check */
        if (new_capacity > STRING_BUFFER_MAX_CAPACITY) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                "Calculated capacity %zu exceeds maximum.", new_capacity);
            buf->error = 1;
            return -1;
        }
    }

    /* ensure we have enough capacity */
    if (new_capacity < min_capacity) {
        new_capacity = min_capacity;
    }

    char* new_data = (char*)realloc(buf->data, new_capacity);
    if (!new_data) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to reallocate string buffer to %zu bytes.", new_capacity);
        buf->error = 1;
        return -1;
    }

    buf->data = new_data;
    buf->capacity = new_capacity;
    return 0;
}

StringBuffer* string_buffer_create(size_t initial_capacity) {
    /* clear previous errors */
    html2tex_err_clear();

    /* allocate buffer structure */
    StringBuffer* buf = (StringBuffer*)calloc(1, sizeof(StringBuffer));
    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate %zu bytes for StringBuffer structure.",
            sizeof(StringBuffer));
        return NULL;
    }

    /* validate and set initial capacity */
    if (initial_capacity > STRING_BUFFER_MAX_CAPACITY) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "Initial capacity %zu exceeds maximum %zu.",
            initial_capacity, STRING_BUFFER_MAX_CAPACITY);
        free(buf);
        return NULL;
    }

    buf->capacity = (initial_capacity > 0) ? initial_capacity : STRING_BUFFER_MIN_CAPACITY;

    /* allocate data buffer */
    buf->data = (char*)malloc(buf->capacity);
    if (!buf->data) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate %zu bytes for string buffer data.",
            buf->capacity);
        free(buf);
        return NULL;
    }

    /* initialize buffer */
    buf->data[0] = '\0';
    buf->length = 0;
    buf->error = 0;

    return buf;
}

void string_buffer_destroy(StringBuffer* buf) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for destruction.");
        return;
    }

    /* free data if present */
    if (buf->data) {
        free(buf->data);
        buf->data = NULL;
    }

    /* free structure */
    free(buf);
}

int string_buffer_clear(StringBuffer* buf) {
    /* clear previous errors */
    html2tex_err_clear();

    /* validate input */
    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for clear operation.");
        return -1;
    }

    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return -1;
    }

    /* clear buffer contents */
    buf->length = 0;

    if (buf->capacity > 0 && buf->data)
        buf->data[0] = '\0';

    return 0;
}

char* string_buffer_detach(StringBuffer* buf) {
    /* clear previous errors */
    html2tex_err_clear();

    /* validate input */
    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for detach operation.");
        return NULL;
    }

    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return NULL;
    }

    /* handle empty buffer */
    if (buf->length == 0) {
        char* empty_string = (char*)malloc(1);
        if (!empty_string) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                "Failed to allocate empty string for detach.");
            return NULL;
        }
        empty_string[0] = '\0';

        /* reset buffer */
        if (string_buffer_clear(buf) != 0) {
            free(empty_string);
            return NULL;
        }

        return empty_string;
    }

    /* shrink buffer to exact size */
    char* result = (char*)realloc(buf->data, buf->length + 1);
    if (!result) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to reallocate buffer to %zu bytes for detach.",
            buf->length + 1);
        buf->error = 1;
        return NULL;
    }

    /* allocate new buffer for the structure */
    buf->data = (char*)malloc(STRING_BUFFER_MIN_CAPACITY);
    if (!buf->data) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate new buffer after detach.");

        /* restore original buffer to maintain consistency */
        buf->data = result;
        buf->error = 1;
        return NULL;
    }

    /* reset buffer state */
    buf->data[0] = '\0';
    buf->capacity = STRING_BUFFER_MIN_CAPACITY;
    buf->length = 0;

    return result;
}

int string_buffer_append(StringBuffer* buf, const char* str, size_t len) {
    /* clear previous errors */
    html2tex_err_clear();

    /* validate input */
    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for append operation.");
        return -1;
    }

    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return -1;
    }

    /* NULL string is allowed but does nothing */
    if (!str) return 0;

    /* compute length if not provided */
    if (len == 0) {
        len = strlen(str);
        if (len == 0) return 0;
    }

    /* validate length doesn't cause overflow */
    if (buf->length > SIZE_MAX - len) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "Append length %zu would overflow current length %zu.",
            len, buf->length);
        buf->error = 1;
        return -1;
    }

    /* ensure capacity */
    if (string_buffer_ensure_capacity(buf, buf->length + len + 1) != 0)
        return -1;

    /* copy data with bounds checking */
    if (len > 0) {
        memcpy(buf->data + buf->length, str, len);
        buf->length += len;
        buf->data[buf->length] = '\0';
    }

    return 0;
}

int string_buffer_append_char(StringBuffer* buf, char c) {
    /* clear previous errors */
    html2tex_err_clear();

    /* validate input */
    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for char append.");
        return -1;
    }

    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return -1;
    }

    /* ensure capacity for character + null terminator */
    if (string_buffer_ensure_capacity(buf, buf->length + 2) != 0)
        return -1;

    /* append character */
    buf->data[buf->length++] = c;
    buf->data[buf->length] = '\0';
    return 0;
}

int string_buffer_append_printf(StringBuffer* buf, const char* format, ...) {
    /* clear previous errors */
    html2tex_err_clear();

    /* validate input */
    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for printf append.");
        return -1;
    }

    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return -1;
    }

    if (!format) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "Format string for printf append.");
        return -1;
    }

    /* get variable arguments */
    va_list args;
    va_start(args, format);

    /* determine required size */
    va_list args_copy;
    va_copy(args_copy, args);

    int needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (needed < 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "vsnprintf failed to calculate required size for format: %s.", format);
        va_end(args);
        buf->error = 1;
        return -1;
    }

    /* empty result */
    if (needed == 0) {
        va_end(args);
        return 0;
    }

    /* ensure capacity */
    if (string_buffer_ensure_capacity(buf, buf->length + (size_t)needed + 1) != 0) {
        va_end(args);
        return -1;
    }

    /* perform actual formatting */
    int written = vsnprintf(buf->data + buf->length,
        buf->capacity - buf->length,
        format, args);
    va_end(args);

    if (written < 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "vsnprintf failed to format string for format: %s.", format);
        buf->error = 1;
        return -1;
    }

    if ((size_t)written >= buf->capacity - buf->length) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "Formatted string overflowed buffer capacity.");
        buf->error = 1;
        return -1;
    }

    /* update buffer length */
    buf->length += (size_t)written;
    return written;
}

int string_buffer_append_latex(StringBuffer* buf, const char* str) {
    /* clear previous errors */
    html2tex_err_clear();

    /* validate input */
    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for LaTeX escape.");
        return -1;
    }

    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return -1;
    }

    if (!str) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "String for LaTeX escaping.");
        return -1;
    }

    /* LaTeX special characters lookup table */
    static const unsigned char escape_map[256] = {
        ['\\'] = 1,['{'] = 2,['}'] = 3,['&'] = 4,
        ['%'] = 5,['$'] = 6,['#'] = 7,['_'] = 8,
        ['^'] = 9,['~'] = 10,['<'] = 11,['>'] = 12,
        ['\n'] = 13,['['] = 14,[']'] = 15,['('] = 16,
        [')'] = 17,['|'] = 18
    };

    static const char* const escape_seq[] = {
        NULL,
        "\\textbackslash{}", "\\{", "\\}", "\\&",
        "\\%", "\\$", "\\#", "\\_", "\\^{}",
        "\\~{}", "\\textless{}", "\\textgreater{}",
        "\\\\", "\\lbrack{}", "\\rbrack{}",
        "\\lparen{}", "\\rparen{}", "\\textbar{}"
    };

    const char* p = str;
    const char* start = p;
    size_t segment_count = 0;

    /* calculate required capacity */
    size_t extra_chars = 0;

    while (*p) {
        unsigned char c = (unsigned char)*p;
        unsigned char type = escape_map[c];

        if (type != 0) {
            const char* seq = escape_seq[type];
            extra_chars += strlen(seq) - 1;
        }

        p++;
    }

    /* reset pointer */
    p = str;

    /* ensure capacity for escaped string */
    size_t original_len = strlen(str);
    size_t total_len = original_len + extra_chars;

    if (string_buffer_ensure_capacity(buf, buf->length + total_len + 1) != 0)
        return -1;

    /* process and escape */
    while (*p) {
        unsigned char c = (unsigned char)*p;
        unsigned char type = escape_map[c];

        if (type == 0) {
            p++;
            continue;
        }

        /* copy normal characters before escape sequence */
        if (p > start) {
            size_t normal_len = p - start;

            if (string_buffer_append(buf, start, normal_len) != 0)
                return -1;
        }

        /* append escape sequence */
        const char* seq = escape_seq[type];

        if (string_buffer_append(buf, seq, 0) != 0)
            return -1;

        p++;
        start = p;
        segment_count++;
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
    /* clear previous errors */
    html2tex_err_clear();

    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for cstr operation.");
        return "";
    }

    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return "";
    }

    return buf->data ? buf->data : "";
}

size_t string_buffer_length(const StringBuffer* buf) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for length query.");
        return 0;
    }

    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return 0;
    }

    return buf->length;
}

int string_buffer_has_error(const StringBuffer* buf) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for error check.");
        return -1;
    }

    return buf->error ? 1 : 0;
}

size_t string_buffer_remaining(const StringBuffer* buf) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for remaining query.");
        return 0;
    }

    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return 0;
    }

    if (buf->capacity == 0)
        return 0;

    if (buf->length >= buf->capacity) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "Buffer length %zu exceeds capacity %zu.", buf->length, buf->capacity);
        return 0;
    }

    return buf->capacity - buf->length - 1;
}

int string_buffer_ensure_capacity(StringBuffer* buf, size_t needed) {
    /* clear previous errors */
    html2tex_err_clear();

    /* validate input */
    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for capacity ensure.");
        return -1;
    }

    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return -1;
    }

    /* check if we already have enough capacity */
    if (needed <= buf->capacity)
        return 0;

    /* validate capacity request */
    if (needed > STRING_BUFFER_MAX_CAPACITY) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "Requested capacity %zu exceeds maximum %zu.",
            needed, STRING_BUFFER_MAX_CAPACITY);
        buf->error = 1;
        return -1;
    }

    /* grow buffer */
    return string_buffer_grow(buf, needed);
}

int string_buffer_shrink_to_fit(StringBuffer* buf) {
    /* clear previous errors */
    html2tex_err_clear();

    /* validate input */
    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for shrink operation.");
        return -1;
    }

    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return -1;
    }

    /* calculate optimal capacity */
    size_t optimal_capacity = buf->length + 1;

    if (optimal_capacity < STRING_BUFFER_MIN_CAPACITY)
        optimal_capacity = STRING_BUFFER_MIN_CAPACITY;

    /* check if shrink is needed */
    if (buf->capacity <= optimal_capacity)
        return 0;

    /* perform reallocation */
    char* new_data = (char*)realloc(buf->data, optimal_capacity);

    if (!new_data) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to shrink buffer from %zu to %zu bytes.",
            buf->capacity, optimal_capacity);
        return 0;
    }

    /* update buffer state */
    buf->data = new_data;
    buf->capacity = optimal_capacity;
    return 0;
}

int string_buffer_reserve(StringBuffer* buf, size_t capacity) {
    return string_buffer_ensure_capacity(buf, capacity);
}

int string_buffer_append_buffer(StringBuffer* dest, const StringBuffer* src) {
    /* clear previous errors */
    html2tex_err_clear();

    /* validate input */
    if (!dest) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "Destination buffer for append operation.");
        return -1;
    }

    if (!src) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "Source buffer for append operation.");
        return -1;
    }

    if (dest->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Destination buffer is in error state.");
        return -1;
    }

    if (src->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Source buffer is in error state.");
        return -1;
    }

    /* append source buffer contents */
    return string_buffer_append(dest, src->data, src->length);
}

char string_buffer_get_char(const StringBuffer* buf, size_t index) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for character access.");
        return '\0';
    }

    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return '\0';
    }

    if (index >= buf->length) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "Character index %zu out of bounds (length: %zu).", index, buf->length);
        return '\0';
    }

    return buf->data[index];
}

int string_buffer_set_char(StringBuffer* buf, size_t index, char c) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!buf) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "StringBuffer object for character set.");
        return -1;
    }

    if (buf->error) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer is in error state.");
        return -1;
    }

    if (index >= buf->length) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "Character index %zu out of bounds (length: %zu).", index, buf->length);
        return -1;
    }

    buf->data[index] = c;
    return 0;
}