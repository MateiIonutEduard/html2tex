#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 4096

typedef struct {
    const char* input;
    size_t position;
    size_t length;
} ParserState;

static void skip_whitespace(ParserState* state) {
    while (state->position < state->length &&
        isspace(state->input[state->position]))
        state->position++;
}

static char* parse_tag_name(ParserState* state) {
    size_t start = state->position;

    while (state->position < state->length &&
        (isalnum(state->input[state->position]) ||
            state->input[state->position] == '-'))
        state->position++;

    if (state->position == start) return NULL;
    size_t length = state->position - start;
    char* name = malloc(length + 1);

    strncpy(name, state->input + start, length);
    name[length] = '\0';

    /* convert to lowercase for consistency */
    for (size_t i = 0; i < length; i++)
        name[i] = tolower(name[i]);

    return name;
}

static char* parse_quoted_string(ParserState* state) {
    if (state->position >= state->length) 
        return NULL;

    char quote = state->input[state->position];
    if (quote != '"' && quote != '\'') return NULL;

    /* skip opening quote */
    state->position++;

    size_t start = state->position;
    while (state->position < state->length &&
        state->input[state->position] != quote)
        state->position++;

    if (state->position >= state->length) return NULL;
    size_t length = state->position - start;

    char* str = malloc(length + 1);
    strncpy(str, state->input + start, length);

    /* skip closing quote */
    str[length] = '\0';
    state->position++;

    return str;
}

static HTMLAttribute* parse_attributes(ParserState* state) {
    HTMLAttribute* head = NULL;
    HTMLAttribute** current = &head;

    while (state->position < state->length) {
        skip_whitespace(state);

        if (state->input[state->position] == '>' ||
            state->input[state->position] == '/')
            break;

        char* key = parse_tag_name(state);
        if (!key) break;

        skip_whitespace(state);
        char* value = NULL;

        if (state->position < state->length && state->input[state->position] == '=') {
            state->position++;
            skip_whitespace(state);
            value = parse_quoted_string(state);
        }

        HTMLAttribute* attr = malloc(sizeof(HTMLAttribute));
        attr->key = key;

        attr->value = value;
        attr->next = NULL;

        *current = attr;
        current = &(*current)->next;
    }

    return head;
}

static char* parse_text_content(ParserState* state) {
    size_t start = state->position;
    size_t length = 0;

    while (state->position < state->length) {
        if (state->input[state->position] == '<')
            break;

        state->position++;
        length++;
    }

    if (length == 0) return NULL;
    char* text = malloc(length + 1);

    strncpy(text, state->input + start, length);
    text[length] = '\0';

    /* trim whitespace */
    while (length > 0 && isspace(text[length - 1]))
        text[--length] = '\0';

    size_t start_trim = 0;

    while (start_trim < length && isspace(text[start_trim]))
        start_trim++;

    if (start_trim > 0)
        memmove(text, text + start_trim, length - start_trim + 1);

    return text[0] ? text : NULL;
}