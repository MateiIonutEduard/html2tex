#include "html2tex_errors.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
#define HTML2TEX_TLS __declspec(thread)
#else
#define HTML2TEX_TLS __thread
#endif

/* Internal error context structure */
struct HTML2TeXErrorCtx {
    HTML2TeXError code;
    char msg[HTML2TEX_ERROR_MSG_MAX];
    int sys_errno;
    const char* file;
    int line;
    const char* func;
};

/* Thread-local error context */
static HTML2TEX_TLS struct HTML2TeXErrorCtx tls_err = {
    0
};

/* Saved the error state. */
struct HTML2TeXErrorSave {
    struct HTML2TeXErrorCtx ctx;
};

/* static error descriptions via O(1) ops lookup table */
static const char* const err_strings[] = {
    "Success.",
    "Memory allocation failed.",
    "Buffer overflow.",
    "Invalid argument.",
    "NULL pointer argument.",
    "I/O error.",
    "Failed to open file.",
    "Failed to read file.",
    "Failed to write file.",
    "Parsing failed.",
    "HTML syntax error.",
    "CSS syntax error.",
    "Malformed document.",
    "Conversion failed.",
    "Unsupported feature.",
    "CSS processing error.",
    "Invalid CSS value.",
    "Table processing error.",
    "Invalid table structure.",
    "Image processing error.",
    "Failed to download image.",
    "Failed to decode image.",
    "Internal library error.",
    "Assertion failed.",
    "Network error.",
    "Download failed."
};

/* Clear the error context. */
static void err_clear(void) {
    tls_err.code = HTML2TEX_OK;
    tls_err.msg[0] = '\0';
    tls_err.sys_errno = 0;
    tls_err.file = NULL;
    tls_err.line = 0;
    tls_err.func = NULL;
}

/* Safe string copy with guaranteed null termination. */
static void str_copy_safe(char* dest, size_t dest_size, const char* src) {
    if (dest_size == 0) return;

    size_t i = 0;
    while (i < dest_size - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* Format error message with bounds checking. */
static void err_format(char* buf, size_t size, const char* fmt, va_list args) {
    if (size == 0) return;
    int written = vsnprintf(buf, size, fmt, args);

    if (written < 0 || (size_t)written >= size) {
        if (size > 4) {
            str_copy_safe(buf, size - 3, buf);
            str_copy_safe(buf + size - 4, 4, "...");
        }
    }
}

HTML2TeXError html2tex_err_get(void) {
    return tls_err.code;
}

const char* html2tex_err_msg(void) {
    return tls_err.msg;
}

const char* html2tex_err_str(HTML2TeXError err) {
    if (err >= 0 && err < (int)(sizeof(err_strings) / sizeof(err_strings[0])))
        return err_strings[err];
    return "Unknown error";
}

void html2tex_err_clear(void) {
    err_clear();
}

void* html2tex_err_save(void) {
    struct HTML2TeXErrorSave* save = malloc(sizeof(struct HTML2TeXErrorSave));
    if (!save) return NULL;

    save->ctx = tls_err;
    return save;
}

void html2tex_err_restore(void* saved) {
    if (!saved) {
        err_clear();
        return;
    }

    struct HTML2TeXErrorSave* save = (struct HTML2TeXErrorSave*)saved;
    tls_err = save->ctx;
    free(save);
}

int html2tex_err_syserr(void) {
    return tls_err.sys_errno;
}

const char* html2tex_err_file(void) {
    return tls_err.file;
}

int html2tex_err_line(void) {
    return tls_err.line;
}

int html2tex_has_error(void) {
    return tls_err.code != HTML2TEX_OK;
}


void html2tex__err_set(HTML2TeXError err, const char* fmt, ...) {
    tls_err.code = err;
    tls_err.sys_errno = errno;
    tls_err.file = NULL;
    tls_err.line = 0;
    tls_err.func = NULL;

    if (fmt && fmt[0]) {
        va_list args;
        va_start(args, fmt);
        err_format(tls_err.msg, sizeof(tls_err.msg), fmt, args);
        va_end(args);
    }
    else {
        str_copy_safe(tls_err.msg, sizeof(tls_err.msg), html2tex_err_str(err));
    }
}

void html2tex__err_set_loc(HTML2TeXError err, const char* file, int line,
    const char* func, const char* fmt, ...) {
    tls_err.code = err;
    tls_err.sys_errno = errno;
    tls_err.file = file;
    tls_err.line = line;
    tls_err.func = func;

    if (fmt && fmt[0]) {
        char user_msg[HTML2TEX_ERROR_MSG_MAX];
        va_list args; va_start(args, fmt);
        err_format(user_msg, sizeof(user_msg), fmt, args);
        va_end(args);

        if (func && file) {
            snprintf(tls_err.msg, sizeof(tls_err.msg),
                "%s (%s:%d)", user_msg, file, line);
        }
        else
            str_copy_safe(tls_err.msg, sizeof(tls_err.msg), user_msg);
    }
    else {
        const char* base = html2tex_err_str(err);
        if (func && file) {
            snprintf(tls_err.msg, sizeof(tls_err.msg),
                "%s (%s:%d)", base, file, line);
        }
        else
            str_copy_safe(tls_err.msg, sizeof(tls_err.msg), base);
    }
}