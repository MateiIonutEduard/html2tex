#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This helper function is used to escape HTML special characters. */
static char* escape_html(const char* text) {
    if (!text) return NULL;

    size_t len = strlen(text);
    size_t new_len = len;

    /* count special characters to determine buffer size */
    for (const char* p = text; *p; p++) {
        if (*p == '<' || *p == '>' || *p == '&' || *p == '"' || *p == '\'') {
            switch (*p) {
            case '<': case '>': new_len += 3; break;
            case '&': new_len += 4; break;
            case '"': new_len += 5; break;
            case '\'': new_len += 5; break;
            }
        }
    }

    char* escaped = malloc(new_len + 1);
    if (!escaped) return NULL;

    char* dest = escaped;
    for (const char* p = text; *p; p++) {
        switch (*p) {
        case '<': strcpy(dest, "&lt;"); dest += 4; break;
        case '>': strcpy(dest, "&gt;"); dest += 4; break;
        case '&': strcpy(dest, "&amp;"); dest += 5; break;
        case '"': strcpy(dest, "&quot;"); dest += 6; break;
        case '\'': strcpy(dest, "&apos;"); dest += 6; break;
        default: *dest++ = *p; break;
        }
    }

    *dest = '\0';
    return escaped;
}