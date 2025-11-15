#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_CAPACITY 1024
#define GROWTH_FACTOR 2

static void ensure_capacity(LaTeXConverter* converter, size_t needed) {
    if (converter->output_capacity == 0) {
        converter->output_capacity = INITIAL_CAPACITY;
        converter->output = malloc(converter->output_capacity);

        converter->output[0] = '\0';
        converter->output_size = 0;
    }

    if (converter->output_size + needed >= converter->output_capacity) {
        converter->output_capacity *= GROWTH_FACTOR;
        converter->output = realloc(converter->output, converter->output_capacity);
    }
}

static void append_string(LaTeXConverter* converter, const char* str) {
    size_t len = strlen(str);
    ensure_capacity(converter, len + 1);

    strcat(converter->output, str);
    converter->output_size += len;
}

static void append_char(LaTeXConverter* converter, char c) {
    ensure_capacity(converter, 2);
    converter->output[converter->output_size++] = c;
    converter->output[converter->output_size] = '\0';
}

static char* get_attribute(HTMLAttribute* attrs, const char* key) {
    HTMLAttribute* current = attrs;

    while (current) {
        if (strcmp(current->key, key) == 0)
            return current->value;

        current = current->next;
    }

    return NULL;
}