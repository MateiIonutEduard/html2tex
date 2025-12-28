#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* Parses CSS margin shorthand property and sets individual margin properties. */
static int css_properties_set_margin_shorthand(CSSProperties* props, const char* value) {
    if (!props || !value) return 0;
    if (value[0] == '\0') return 1;

    const char* p = value;
    const char* value_end = value;
    size_t length = 0;
    int important = 0;

    /* find end of value (before !important) and check length */
    while (*p) {
        if (!important &&
            (*p == '!' || (*p == 'i' && p == value))) {

            /* potential !important marker */
            const char* test = p;

            /* skip whitespace before potential !important */
            if (*test == ' ') {
                const char* important_start = test;
                while (important_start > value && isspace((unsigned char)important_start[-1]))
                    important_start--;

                if (important_start > value && important_start[-1] == '!')
                    test = important_start - 1;
            }

            /* check for "!important" (case-insensitive) */
            if (strncasecmp(test, "!important", 10) == 0) {
                const char* after = test + 10;

                if (*after == '\0' || isspace((unsigned char)*after)) {
                    important = 1;
                    value_end = test;

                    /* skip back over whitespace before !important */
                    while (value_end > value && isspace((unsigned char)value_end[-1])) value_end--;

                    break;
                }
            }
        }

        /* track length with reasonable limit */
        if (++length > MAX_REASONABLE_MARGIN_LENGTH) return 0;
        p++;
    }

    /* if !important not found, value_end is at null terminator */
    if (!important) value_end = p;

    /* empty value after stripping !important */
    if (value_end == value)
        return important ? 1 : 0;

    char tokens[4][MAX_MARGIN_TOKEN_LENGTH];
    int token_count = 0;
    int current_token_len = 0;

    p = value;

    /* parse tokens with bounds checking */
    while (p < value_end && token_count < 4) {
        while (p < value_end && isspace((unsigned char)*p)) p++;
        if (p >= value_end) break;

        const char* token_start = p;
        current_token_len = 0;

        /* parse until whitespace or end */
        while (p < value_end && !isspace((unsigned char)*p)) {
            if (current_token_len >= MAX_MARGIN_TOKEN_LENGTH - 1)
                return 0;

            /* validate CSS value characters */
            unsigned char c = (unsigned char)*p;

            if (!(isdigit(c) || c == '.' || c == '-' || c == '+' || c == '%' ||
                (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
                return 0;

            tokens[token_count][current_token_len++] = *p++;
        }

        /* check if we actually parsed a token */
        if (current_token_len > 0) {
            unsigned char first_char = (unsigned char)tokens[token_count][0];

            /* CSS length must start with digit, minus, plus, or dot */
            if (!(isdigit(first_char) || first_char == '-' ||
                first_char == '+' || first_char == '.')) {
                return 0;
            }

            tokens[token_count][current_token_len] = '\0';
            token_count++;
        }

        /* skip trailing whitespace for next token */
        while (p < value_end && isspace((unsigned char)*p))
            p++;
    }

    /* validate we consumed all input */
    while (p < value_end && isspace((unsigned char)*p)) p++;
    if (p != value_end) return 0;

    /* must have at least one token for valid margin */
    if (token_count == 0) return 0;

    /* tokens should be reasonable CSS lengths */
    for (int i = 0; i < token_count; i++) {
        const char* token = tokens[i];
        int has_digit = 0;

        for (int j = 0; token[j]; j++) {
            if (isdigit((unsigned char)token[j])) {
                has_digit = 1;
                break;
            }
        }
        if (!has_digit) {
            /* could be 'auto' or 'inherit', check those */
            if (strcmp(token, "auto") != 0 && strcmp(token, "inherit") != 0)
                return 0;
        }
    }

    char expanded[4][MAX_MARGIN_TOKEN_LENGTH] = { {0}, {0}, {0}, {0} };

    switch (token_count) {
    case 1:
        /* all four margins get the same value */
        strncpy(expanded[0], tokens[0], sizeof(expanded[0]) - 1);
        strncpy(expanded[1], tokens[0], sizeof(expanded[1]) - 1);
        strncpy(expanded[2], tokens[0], sizeof(expanded[2]) - 1);
        strncpy(expanded[3], tokens[0], sizeof(expanded[3]) - 1);
        break;

    case 2:
        /* top/bottom = first, left/right = second */
        strncpy(expanded[0], tokens[0], sizeof(expanded[0]) - 1);
        strncpy(expanded[1], tokens[1], sizeof(expanded[1]) - 1);
        strncpy(expanded[2], tokens[0], sizeof(expanded[2]) - 1);
        strncpy(expanded[3], tokens[1], sizeof(expanded[3]) - 1);
        break;

    case 3:
        /* top = first, left/right = second, bottom = third */
        strncpy(expanded[0], tokens[0], sizeof(expanded[0]) - 1);
        strncpy(expanded[1], tokens[1], sizeof(expanded[1]) - 1);
        strncpy(expanded[2], tokens[2], sizeof(expanded[2]) - 1);
        strncpy(expanded[3], tokens[1], sizeof(expanded[3]) - 1);
        break;

    case 4:
        /* top, right, bottom, left */
        strncpy(expanded[0], tokens[0], sizeof(expanded[0]) - 1);
        strncpy(expanded[1], tokens[1], sizeof(expanded[1]) - 1);
        strncpy(expanded[2], tokens[2], sizeof(expanded[2]) - 1);
        strncpy(expanded[3], tokens[3], sizeof(expanded[3]) - 1);
        break;

    default:
        return 0;
    }

    /* set individual margin properties */
    int success = 1;

    if (!css_properties_set(props, "margin-top", expanded[0], important)) success = 0;
    if (!css_properties_set(props, "margin-right", expanded[1], important)) success = 0;
    if (!css_properties_set(props, "margin-bottom", expanded[2], important)) success = 0;
    if (!css_properties_set(props, "margin-left", expanded[3], important)) success = 0;

    return success;
}

/* Detect if a CSS property is a margin shorthand. */
static int handle_margin_shorthand(CSSProperties* props, const char* key, const char* value) {
    if (!props || !key || !value)
        return 0;

    /* check if this is a margin shorthand property */
    if (strcasecmp(key, "margin") == 0)
        return css_properties_set_margin_shorthand(props, value);

    /* no margin shorthand property */
    return 0;
}

CSSProperties* css_properties_create(void) {
    CSSProperties* props = (CSSProperties*)calloc(1, sizeof(CSSProperties));
    if (!props) return NULL;

    props->head = NULL;
    props->tail = NULL;
    props->count = 0;
    props->mask = 0;

    return props;
}

void css_properties_destroy(CSSProperties* props) {
    if (!props) return;
    CSSProperty* current = props->head;

    while (current) {
        CSSProperty* next = current->next;
        free((void*)current->key);
        free(current->value);

        free(current);
        current = next;
    }

    free(props);
}

static CSSPropertyMask property_to_mask(const char* key) {
    if (!key || key[0] == '\0') return 0;

    static const struct {
        const char* property_name;
        unsigned char first_char;
        const unsigned char length;
        CSSPropertyMask mask;
    } props[] = {
        {"font-weight", 'f', 11, CSS_BOLD}, {"font-style", 'f', 10, CSS_ITALIC},
        {"text-decoration", 't', 15, CSS_UNDERLINE }, {"color", 'c', 5, CSS_COLOR},
        {"background-color", 'b', 16, CSS_BACKGROUND}, {"font-family", 'f', 11, CSS_FONT_FAMILY},
        {"font-size", 'f', 9, CSS_FONT_SIZE}, {"text-align", 't', 10, CSS_TEXT_ALIGN},
        {"border", 'b', 6, CSS_BORDER}, {"margin-left", 'm', 11, CSS_MARGIN_LEFT},
        {"margin-right", 'm', 12, CSS_MARGIN_RIGHT}, {"margin-top", 'm', 10, CSS_MARGIN_TOP},
        {"margin-bottom", 'm', 13, CSS_MARGIN_BOTTOM}, {NULL, 0, 0, 0}
    };

    /* length-based detection */
    size_t len = 0;
    const char* p = key;

    while (*p) {
        len++; p++;

        /* reject unknown property */
        if (len > 16) return 0;
    }

    /* length-based fast rejection */
    switch (len) {
    case 5:  case 6:  case 9:
    case 10: case 11: case 12:
    case 13:  case 15: case 16:
        break;
    default:
        /* length does not match */
        return 0;
    }

    /* extract first char for fast comparison */
    const unsigned char first_char = (unsigned char)tolower(key[0]);

    for (int i = 0; props[i].property_name; i++) {
        /* first character mismatch */
        if (first_char != props[i].first_char) continue;

        /* length mismatch */
        if (props[i].length != len) continue;

        /* exact string match via strcasecmp function */
        if (strcasecmp(key, props[i].property_name) == 0)
            return props[i].mask;
    }

    return 0;
}

int css_properties_set(CSSProperties* props, const char* key, const char* value, int important) {
    /* efficient input validation */
    if (!props || !key || !value)
        return 0;

    /* prevent empty keys and obviously bad input */
    if (key[0] == '\0') return 0;

    /* sanity check for injection attempts in CSS property names */
    const char* dangerous_chars = "<>;\"'";

    for (const char* c = dangerous_chars; *c; c++) {
        if (strchr(key, *c))
            return 0;
    }

    /* length limits to prevent DoS */
    size_t key_len = strlen(key);
    size_t value_len = strlen(value);

    if (key_len > CSS_KEY_PROPERTY_LENGTH || value_len > CSS_MAX_PROPERTY_LENGTH)
        return 0;

    /* handle special case for margin shorthand */
    if (strcasecmp(key, "margin") == 0)
        return css_properties_set_margin_shorthand(props, value);

    /* check if property already exists */
    CSSProperty* current = props->head;
    CSSProperty* prev = NULL;

    while (current) {
        if (strcasecmp(current->key, key) == 0) {
            char* new_value = strdup(value);
            if (!new_value) return 0;

            free(current->value);
            current->value = new_value;
            current->important = important ? 1 : 0;

            /* update bitmask */
            CSSPropertyMask mask = property_to_mask(key);
            if (mask) props->mask |= mask;
            return 1;
        }

        prev = current;
        current = current->next;
    }

    /* create new property */
    CSSProperty* new_prop = NULL;
    char* key_copy = NULL;
    char* value_copy = NULL;

    /* allocate and initialize */
    new_prop = (CSSProperty*)calloc(1, sizeof(CSSProperty));
    if (!new_prop) goto cleanup_failure;

    key_copy = strdup(key);
    if (!key_copy) goto cleanup_failure;

    value_copy = strdup(value);
    if (!value_copy) goto cleanup_failure;

    /* initialize after all allocations succeeded */
    new_prop->key = key_copy;
    new_prop->value = value_copy;
    new_prop->important = important ? 1 : 0;
    new_prop->next = NULL;

    /* insert into list */
    if (!props->head) {
        props->head = new_prop;
        props->tail = new_prop;
    }
    else {
        props->tail->next = new_prop;
        props->tail = new_prop;
    }

    props->count++;

    /* update metadata */
    CSSPropertyMask mask = property_to_mask(key);
    if (mask) props->mask |= mask;
    return 1;

cleanup_failure:
    /* cleanup path for any allocation failure */
    if (key_copy) free(key_copy);
    if (value_copy) free(value_copy);
    if (new_prop) free(new_prop);
    return 0;
}

const char* css_properties_get(const CSSProperties* props, const char* key) {
    if (!props || !key) return NULL;
    CSSProperty* current = props->head;

    while (current) {
        if (strcasecmp(current->key, key) == 0)
            return current->value;

        current = current->next;
    }

    return NULL;
}

int css_properties_has(const CSSProperties* props, const char* key) {
    if (!props || !key) return 0;
    CSSProperty* current = props->head;

    while (current) {
        if (strcasecmp(current->key, key) == 0) return 1;
        current = current->next;
    }

    return 0;
}

CSSProperties* css_properties_copy(const CSSProperties* src) {
    if (!src) return NULL;
    CSSProperties* copy = css_properties_create();

    if (!copy) return NULL;
    CSSProperty* current = src->head;

    while (current) {
        if (!css_properties_set(copy, current->key, current->value, current->important)) {
            css_properties_destroy(copy);
            return NULL;
        }

        current = current->next;
    }

    return copy;
}

CSSProperties* css_properties_merge(const CSSProperties* parent, const CSSProperties* child) {
    /* validate input for edge cases */
    if (!child) return parent ? css_properties_copy(parent) : NULL;
    if (!parent) return css_properties_copy(child);

    /* if child has no inheritable properties, just copy child */
    if ((child->mask & CSS_INHERITABLE_MASK) == 0)
        return css_properties_copy(child);

    /* ownership transferred to caller on success */
    CSSProperties* result = css_properties_create();
    if (!result) return NULL;

    /* copy all parent properties first, will be overridden by child */
    CSSProperty* parent_prop = parent->head;
    while (parent_prop) {
        if (is_css_property_inheritable(parent_prop->key)) {
            if (!css_properties_set(result, parent_prop->key,
                parent_prop->value, parent_prop->important)) {
                goto cleanup_failure;
            }
        }

        parent_prop = parent_prop->next;
    }

    /* apply child properties with correct cascade */
    CSSProperty* child_prop = child->head;

    while (child_prop) {
        CSSProperty* existing = result->head;
        int should_override = 1;

        while (existing) {
            if (strcasecmp(existing->key, child_prop->key) == 0) {
                if (child_prop->important && !existing->important) should_override = 1;
                else if (!child_prop->important && existing->important) should_override = 0;
                else should_override = 1;
                break;
            }

            existing = existing->next;
        }

        if (should_override) {
            if (!css_properties_set(result, child_prop->key,
                child_prop->value, child_prop->important))
                goto cleanup_failure;
        }

        child_prop = child_prop->next;
    }

    return result;

cleanup_failure:
    css_properties_destroy(result);
    return NULL;
}

CSSProperties* parse_css_style(const char* style_str) {
    /* validate  the input first */
    if (!style_str) return NULL;

    /* quick empty check for common case optimization */
    if (style_str[0] == '\0')
        return css_properties_create();

    /* prevent DoS via extremely long style strings */
    size_t total_len = strlen(style_str);

    if (total_len > CSS_MAX_PROPERTY_LENGTH * 4)
        return NULL;

    /* create properties container with error handling */
    CSSProperties* props = css_properties_create();
    if (!props) return NULL;

    /* single-pass fast parsing */
    const char* p = style_str;

    while (*p) {
        /* skip whitespace between declarations */
        while (*p && isspace((unsigned char)*p))
            p++;
        
        if (!*p) break;

        /* parse property name up to colon or semicolon */
        const char* prop_start = p;

        while (*p && *p != ':' && *p != ';')
            p++;

        /* validate if need colon before semicolon */
        if (!*p || *p == ';') {
            if (*p == ';') p++;
            continue;
        }

        /* trim trailing whitespace from property name */
        const char* prop_end = p;
        while (prop_end > prop_start && isspace((unsigned char)prop_end[-1]))
            prop_end--;

        /* skip the colon */
        p++;

        /* skip whitespace before value */
        while (*p && isspace((unsigned char)*p))
            p++;
        
        if (!*p) break;

        /* parse value with !important detection */
        const char* value_start = p;
        const char* last_non_ws = p;
        int important = 0;

        while (*p && *p != ';') {
            if (!important && (unsigned char)*p == '!') {
                if (strncasecmp(p, "!important", 10) == 0) {
                    char after = p[10];

                    if (!after || after == ';' || isspace((unsigned char)after)) {
                        important = 1;
                        last_non_ws = p;
                        p += 10;
                        continue;
                    }
                }
            }

            /* track last non-whitespace for trimming */
            if (!isspace((unsigned char)*p))
                last_non_ws = p + 1;

            p++;
        }

        const char* value_end = last_non_ws;

        /* skip semicolon for next iteration */
        if (*p == ';') p++;

        /* calculate lengths for validation */
        size_t prop_len = prop_end - prop_start;
        size_t value_len = value_end - value_start;

        /* validate property name length and content */
        if (prop_len == 0 || prop_len > CSS_KEY_PROPERTY_LENGTH)
            continue;

        /* validate the value length */
        if (value_len == 0 || value_len > CSS_MAX_PROPERTY_LENGTH)
            continue;

        /* validate property name doesn't contain dangerous chars */
        int valid_property = 1;

        for (const char* c = prop_start; c < prop_end; c++) {
            unsigned char ch = (unsigned char)*c;
            
            if (ch == '<' || ch == '>' || ch == ';' || ch == '"' || ch == '\'') {
                valid_property = 0;
                break;
            }
        }

        /* skip potentially malicious property */
        if (!valid_property) continue;
        char* prop_name = (char*)malloc(prop_len + 1);
        char* prop_value = (char*)malloc(value_len + 1);

        /* cleanup on allocation failure */
        if (!prop_name || !prop_value) {
            if (prop_name) free(prop_name);
            if (prop_value) free(prop_value);
            css_properties_destroy(props);
            return NULL;
        }

        /* copy with bounds guaranteed by previous validation */
        memcpy(prop_name, prop_start, prop_len);
        prop_name[prop_len] = '\0';

        memcpy(prop_value, value_start, value_len);
        prop_value[value_len] = '\0';

        /* add failures handling */
        int success = css_properties_set(props, prop_name, prop_value, important);

        /* clean up temporary strings */
        free(prop_name);
        free(prop_value);

        /* abort entire parsing */
        if (!success) {
            css_properties_destroy(props);
            return NULL;
        }
    }

    return props;
}

static void apply_text_alignment(LaTeXConverter* converter, const char* align) {
    if (!converter || !align) return;

    if (strcmp(align, "center") == 0) {
        append_string(converter, "\\begin{center}\n");
        converter->state.css_environments |= 1;
    }
    else if (strcmp(align, "right") == 0) {
        append_string(converter, "\\begin{flushright}\n");
        converter->state.css_environments |= 2;
    }
    else if (strcmp(align, "left") == 0) {
        append_string(converter, "\\begin{flushleft}\n");
        converter->state.css_environments |= 4;
    }
    else if (strcmp(align, "justify") == 0) {
        append_string(converter, "\\justifying\n");
        converter->state.css_environments |= 8;
    }
}

static void apply_font_weight(LaTeXConverter* converter, const char* weight) {
    if (!converter || !weight) return;

    if (strcmp(weight, "bold") == 0 || strcmp(weight, "bolder") == 0 ||
        atoi(weight) >= 600) {
        if (!(converter->state.applied_props & CSS_BOLD)) {
            append_string(converter, "\\textbf{");
            converter->state.css_braces++;
            converter->state.applied_props |= CSS_BOLD;
        }
    }
    else if (strcmp(weight, "lighter") == 0 || atoi(weight) <= 300) {
        append_string(converter, "\\textmd{");
        converter->state.css_braces++;
    }
}

static void apply_font_style(LaTeXConverter* converter, const char* style) {
    if (!converter || !style) return;

    if (strcmp(style, "italic") == 0) {
        if (!(converter->state.applied_props & CSS_ITALIC)) {
            append_string(converter, "\\textit{");
            converter->state.css_braces++;
            converter->state.applied_props |= CSS_ITALIC;
        }
    }
    else if (strcmp(style, "oblique") == 0) {
        append_string(converter, "\\textsl{");
        converter->state.css_braces++;
    }
    else if (strcmp(style, "normal") == 0) {
        append_string(converter, "\\textup{");
        converter->state.css_braces++;
    }
}

static void apply_text_decoration(LaTeXConverter* converter, const char* decoration) {
    if (!converter || !decoration) return;

    if (strstr(decoration, "underline")) {
        if (!(converter->state.applied_props & CSS_UNDERLINE)) {
            append_string(converter, "\\underline{");
            converter->state.css_braces++;
            converter->state.applied_props |= CSS_UNDERLINE;
        }
    }

    if (strstr(decoration, "line-through")) {
        append_string(converter, "\\sout{");
        converter->state.css_braces++;
    }

    if (strstr(decoration, "overline")) {
        append_string(converter, "\\overline{");
        converter->state.css_braces++;
    }
}

static void apply_font_family(LaTeXConverter* converter, const char* family) {
    if (!converter || !family) return;

    if (strstr(family, "monospace") || strstr(family, "Courier")) {
        if (!(converter->state.applied_props & CSS_FONT_FAMILY)) {
            append_string(converter, "\\texttt{");
            converter->state.css_braces++;
            converter->state.applied_props |= CSS_FONT_FAMILY;
        }
    }
    else if (strstr(family, "sans") || strstr(family, "Arial") ||
        strstr(family, "Helvetica")) {
        if (!(converter->state.applied_props & CSS_FONT_FAMILY)) {
            append_string(converter, "\\textsf{");
            converter->state.css_braces++;
            converter->state.applied_props |= CSS_FONT_FAMILY;
        }
    }
    else if (strstr(family, "serif") || strstr(family, "Times")) {
        if (!(converter->state.applied_props & CSS_FONT_FAMILY)) {
            append_string(converter, "\\textrm{");
            converter->state.css_braces++;
            converter->state.applied_props |= CSS_FONT_FAMILY;
        }
    }
}

static void apply_font_size(LaTeXConverter* converter, const char* size) {
    if (!converter || !size) return;
    int pt = css_length_to_pt(size);

    if (pt > 0) {
        append_string(converter, "{");

        if (pt <= 8) append_string(converter, "\\tiny ");
        else if (pt <= 10) append_string(converter, "\\small ");
        else if (pt <= 12) append_string(converter, "\\normalsize ");
        else if (pt <= 14) append_string(converter, "\\large ");
        else if (pt <= 18) append_string(converter, "\\Large ");
        else if (pt <= 24) append_string(converter, "\\LARGE ");
        else append_string(converter, "\\huge ");

        converter->state.css_braces++;
        converter->state.applied_props |= CSS_FONT_SIZE;
    }
}

void css_properties_apply(LaTeXConverter* converter, const CSSProperties* props, const char* tag_name) {
    if (!converter || !props) return;

    /* cache the critical state variables */
    const int inside_table_cell = converter->state.in_table_cell;
    const int is_block = is_block_element(tag_name);
    CSSPropertyMask* applied = &converter->state.applied_props;

    /* process text alignment first (block elements only) */
    if (is_block && !inside_table_cell && (props->mask & CSS_TEXT_ALIGN)) {
        const char* align = css_properties_get(props, "text-align");
        if (align) apply_text_alignment(converter, align);
    }

    /* process colors efficiently */
    if (!(*applied & CSS_COLOR) && (props->mask & CSS_COLOR)) {
        const char* color = css_properties_get(props, "color");

        if (color) {
            if (!(color[0] == 'b' && strcmp(color, "black") == 0) &&
                !(color[0] == '#' && (strcmp(color, "#000") == 0 ||
                    strcmp(color, "#000000") == 0))) {
                char* hex = css_color_to_hex(color);

                if (hex) {
                    if (strcmp(hex, "000000") != 0) {
                        append_string(converter, "\\textcolor[HTML]{");
                        append_string(converter, hex);
                        append_string(converter, "}{");
                        converter->state.css_braces++;
                        *applied |= CSS_COLOR;
                    }

                    free(hex);
                }
            }
        }
    }

    /* process background color */
    if (!(*applied & CSS_BACKGROUND) && (props->mask & CSS_BACKGROUND)) {
        const char* bg_color = css_properties_get(props, "background-color");
        if (bg_color) {
            if (!(bg_color[0] == 'w' && strcmp(bg_color, "white") == 0) &&
                !(bg_color[0] == 't' && strcmp(bg_color, "transparent") == 0) &&
                !(bg_color[0] == '#' && (strcmp(bg_color, "#fff") == 0 ||
                    strcmp(bg_color, "#ffffff") == 0))) {

                char* hex = css_color_to_hex(bg_color);

                if (hex) {
                    if (strcmp(hex, "FFFFFF") != 0) {
                        if (inside_table_cell)
                            append_string(converter, "\\cellcolor[HTML]{");
                        else
                            append_string(converter, "\\colorbox[HTML]{");

                        append_string(converter, hex);
                        append_string(converter, "}{");
                        converter->state.css_braces++;
                        *applied |= CSS_BACKGROUND;
                    }

                    free(hex);
                }
            }
        }
    }

    /* process margins for block elements */
    if (is_block && !inside_table_cell) {
        if (!(*applied & CSS_MARGIN_TOP) && (props->mask & CSS_MARGIN_TOP)) {
            const char* margin_top = css_properties_get(props, "margin-top");

            if (margin_top) {
                int pt = css_length_to_pt(margin_top);

                if (pt != 0) {
                    char cmd[32];
                    int len = snprintf(cmd, sizeof(cmd), "\\vspace*{%dpt}\n", pt);

                    if (len > 0 && (size_t)len < sizeof(cmd)) {
                        append_string(converter, cmd);
                        *applied |= CSS_MARGIN_TOP;
                    }
                }
            }
        }

        if (!(*applied & CSS_MARGIN_LEFT) && (props->mask & CSS_MARGIN_LEFT)) {
            const char* margin_left = css_properties_get(props, "margin-left");

            if (margin_left) {
                int pt = css_length_to_pt(margin_left);

                if (pt != 0) {
                    char cmd[32];
                    int len = snprintf(cmd, sizeof(cmd), "\\hspace*{%dpt}", pt);

                    if (len > 0 && (size_t)len < sizeof(cmd)) {
                        append_string(converter, cmd);
                        *applied |= CSS_MARGIN_LEFT;
                    }
                }
            }
        }
    }

    /* Process font properties with optimized checks */
    if (props->mask & CSS_BOLD) {
        const char* weight = css_properties_get(props, "font-weight");
        if (weight) apply_font_weight(converter, weight);
    }

    if (props->mask & CSS_ITALIC) {
        const char* style = css_properties_get(props, "font-style");
        if (style) apply_font_style(converter, style);
    }

    if (props->mask & CSS_FONT_FAMILY) {
        const char* family = css_properties_get(props, "font-family");
        if (family) apply_font_family(converter, family);
    }

    if (props->mask & CSS_FONT_SIZE) {
        const char* size = css_properties_get(props, "font-size");
        if (size) apply_font_size(converter, size);
    }

    if (props->mask & CSS_UNDERLINE) {
        const char* decoration = css_properties_get(props, "text-decoration");
        if (decoration) apply_text_decoration(converter, decoration);
    }

    /* process border (limited support) */
    if (!(*applied & CSS_BORDER) && (props->mask & CSS_BORDER)) {
        const char* border = css_properties_get(props, "border");

        if (border && strstr(border, "solid")) {
            append_string(converter, "\\framebox{");
            converter->state.css_braces++;
            *applied |= CSS_BORDER;
        }
    }
}

void css_properties_end(LaTeXConverter* converter, const CSSProperties* props, const char* tag_name) {
    if (!converter || !props) return;
    const int inside_table_cell = converter->state.in_table_cell;
    const int is_block = is_block_element(tag_name);
    CSSPropertyMask* applied = &converter->state.applied_props;

    /* output right and bottom margins for block elements with bitmask optimization */
    if (is_block && !inside_table_cell) {
        if (!(*applied & CSS_MARGIN_RIGHT) && (props->mask & CSS_MARGIN_RIGHT)) {
            const char* margin_right = css_properties_get(props, "margin-right");

            if (margin_right) {
                int pt = css_length_to_pt(margin_right);

                /* validate pt value and check for non-zero */
                if (pt != 0 && pt > -10000 && pt < 10000) {
                    char margin_cmd[32];
                    int len = snprintf(margin_cmd, sizeof(margin_cmd),
                        "\\hspace*{%dpt}", pt);

                    if (len > 0 && (size_t)len < sizeof(margin_cmd)) {
                        append_string(converter, margin_cmd);
                        *applied |= CSS_MARGIN_RIGHT;
                    }
                }
            }
        }

        /* check margin-bottom with bitmask filtering */
        if (!(*applied & CSS_MARGIN_BOTTOM) && (props->mask & CSS_MARGIN_BOTTOM)) {
            const char* margin_bottom = css_properties_get(props, "margin-bottom");

            if (margin_bottom) {
                int pt = css_length_to_pt(margin_bottom);

                if (pt != 0 && pt > -10000 && pt < 10000) {
                    char margin_cmd[32];
                    const char* format = (pt < 0) ? "\\vspace*{%dpt}" : "\\vspace{%dpt}";
                    int len = snprintf(margin_cmd, sizeof(margin_cmd), format, pt);

                    if (len > 0 && (size_t)len < sizeof(margin_cmd)) {
                        append_string(converter, margin_cmd);
                        *applied |= CSS_MARGIN_BOTTOM;
                    }
                }
            }
        }
    }

    /* efficiently close all opened braces with single allocation when possible */
    const unsigned int brace_count = converter->state.css_braces;
    if (brace_count > 0) {
        if (brace_count <= 8) {
            for (unsigned int i = 0; i < brace_count; i++)
                append_string(converter, "}");
        }
        else {
            /* pre-allocate a string to reduce function calls */
            char* braces = (char*)malloc(brace_count + 1);

            if (braces) {
                memset(braces, '}', brace_count);
                braces[brace_count] = '\0';
                append_string(converter, braces);
                free(braces);
            }
            else {
                /* fallback to loop if allocation fails */
                for (unsigned int i = 0; i < brace_count; i++)
                    append_string(converter, "}");
            }
        }
        converter->state.css_braces = 0;
    }

    /* close alignment environments with bitmask checks */
    const unsigned char env_mask = converter->state.css_environments;
    if (env_mask != 0) {
        switch (env_mask) {
        case 1:
            append_string(converter, "\\end{center}\n");
            break;
        case 2:
            append_string(converter, "\\end{flushright}\n");
            break;
        case 4:
            append_string(converter, "\\end{flushleft}\n");
            break;
        default:
            if (env_mask & 1) append_string(converter, "\\end{center}\n");
            if (env_mask & 2) append_string(converter, "\\end{flushright}\n");
            if (env_mask & 4) append_string(converter, "\\end{flushleft}\n");
            break;
        }
        converter->state.css_environments = 0;
    }

    /* reset applied properties mask */
    *applied = 0;
}

int css_length_to_pt(const char* length_str) {
    if (!length_str || length_str[0] == '\0') return 0;

    /* skip leading whitespace */
    const char* p = length_str;
    while (*p && (*p == ' ' || *p == '\t')) p++;
    if (*p == '\0') return 0;

    /* parse value and unit */
    double value = 0.0;
    char unit[8] = "";

    /* parse with sscanf for safe with buffer limit */
    int chars_parsed = sscanf(p, "%lf%7s", &value, unit);

    /* check for !important in the parsed unit */
    if (chars_parsed >= 2 && strncasecmp(unit, "!important", 10) == 0) {
        chars_parsed = sscanf(p, "%lf", &value);
        unit[0] = '\0';
    }

    if (chars_parsed < 1) {
        if (sscanf(p, "%lf", &value) != 1) return 0;
        unit[0] = '\0';
    }

    /* validate the value range */
    if (value < -1000000.0 || value > 1000000.0) 
        return 0;

    int result = 0;
    char first_char = unit[0];

    switch (first_char) {
    case '\0':
        result = (int)(value * 72.0 / 96.0 + 0.5);
        break;

    case 'p': case 'P':
        if ((unit[1] == 'x' || unit[1] == 'X') && unit[2] == '\0')
            result = (int)(value * 72.0 / 96.0 + 0.5);
        else if ((unit[1] == 't' || unit[1] == 'T') && unit[2] == '\0')
            result = (int)(value + 0.5);
        else
            result = (int)(value * 72.0 / 96.0 + 0.5);
        break;

    case 'e': case 'E':
        if ((unit[1] == 'm' || unit[1] == 'M') && unit[2] == '\0')
            result = (int)(value * 10.0 + 0.5);
        else
            result = (int)(value * 72.0 / 96.0 + 0.5);
        break;

    case 'r': case 'R':
        if ((unit[1] == 'e' || unit[1] == 'E') &&
            (unit[2] == 'm' || unit[2] == 'M') && unit[3] == '\0')
            result = (int)(value * 10.0 + 0.5);
        else
            result = (int)(value * 72.0 / 96.0 + 0.5);
        break;

    case '%':
        if (unit[1] == '\0')
            result = (int)(value * 0.01 * 400.0 + 0.5);
        else
            result = (int)(value * 72.0 / 96.0 + 0.5);
        break;

    case 'c': case 'C':
        if ((unit[1] == 'm' || unit[1] == 'M') && unit[2] == '\0')
            result = (int)(value * 28.346 + 0.5);
        else
            result = (int)(value * 72.0 / 96.0 + 0.5);
        break;

    case 'm': case 'M':
        if ((unit[1] == 'm' || unit[1] == 'M') && unit[2] == '\0')
            result = (int)(value * 2.8346 + 0.5);
        else
            result = (int)(value * 72.0 / 96.0 + 0.5);
        break;

    case 'i': case 'I':
        if ((unit[1] == 'n' || unit[1] == 'N') && unit[2] == '\0')
            result = (int)(value * 72.0 + 0.5);
        else
            result = (int)(value * 72.0 / 96.0 + 0.5);
        break;

    default:
        result = (int)(value * 72.0 / 96.0 + 0.5);
        break;
    }

    /* clamp result to reasonable bounds */
    const int MIN_PT = -10000;
    const int MAX_PT = 10000;

    if (result < MIN_PT) result = MIN_PT;
    else if (result > MAX_PT) result = MAX_PT;
    return result;
}

char* css_color_to_hex(const char* color_value) {
    if (!color_value || color_value[0] == '\0') return NULL;
    const char* p = color_value;

    /* skip leading whitespace */
    while (*p && (*p == ' ' || *p == '\t')) p++;
    if (*p == '\0') return NULL;

    /* handle #hex formats first, most common case */
    if (*p == '#') {
        const char* hex_start = p + 1;
        size_t hex_len = 0;

        /* count hex characters */
        while (hex_start[hex_len]) {
            char c = hex_start[hex_len];

            if (!((c >= '0' && c <= '9') || 
                (c >= 'a' && c <= 'f') || 
                (c >= 'A' && c <= 'F')))
                break;
            
            hex_len++;
        }

        if (hex_len == 3) {
            char* result = (char*)malloc(7);
            if (!result) return NULL;

            result[0] = hex_start[0];
            result[1] = hex_start[0];
            result[2] = hex_start[1];
            result[3] = hex_start[1];
            result[4] = hex_start[2];
            result[5] = hex_start[2];
            result[6] = '\0';

            /* convert to uppercase in place */
            for (int i = 0; i < 6; i++) {
                if (result[i] >= 'a' && result[i] <= 'f')
                    result[i] = result[i] - 'a' + 'A';
            }

            return result;

        }
        else if (hex_len == 6) {
            char* result = (char*)malloc(7);
            if (!result) return NULL;

            memcpy(result, hex_start, 6);
            result[6] = '\0';

            /* convert to uppercase in place */
            for (int i = 0; i < 6; i++) {
                if (result[i] >= 'a' && result[i] <= 'f')
                    result[i] = result[i] - 'a' + 'A';
            }

            return result;
        }
    }

    /* check for the rgb()/rgba() formats */
    if ((p[0] == 'r' || p[0] == 'R') &&
        (p[1] == 'g' || p[1] == 'G') &&
        (p[2] == 'b' || p[2] == 'B')) {

        if ((p[3] == '(' || (p[3] == 'a' && p[4] == '('))) {
            int r = 0, g = 0, b = 0;
            float a = 1.0f;
            int parsed = 0;

            if (p[3] == '(') parsed = sscanf(p, "rgb(%d, %d, %d)", &r, &g, &b);
            else parsed = sscanf(p, "rgba(%d, %d, %d, %f)", &r, &g, &b, &a);

            if (parsed >= 3) {
                /* clamp values to valid range */
                if (r < 0) r = 0; else if (r > 255) r = 255;
                if (g < 0) g = 0; else if (g > 255) g = 255;
                if (b < 0) b = 0; else if (b > 255) b = 255;

                char* result = (char*)malloc(7);
                if (!result) return NULL;

                snprintf(result, 7, "%02X%02X%02X", r, g, b);
                return result;
            }
        }
    }

    /* named colors lookup with optimized searching */
    static const struct {
        const char name[12];
        const char hex[7];
    } color_map[] = {
        {"black", "000000"},
        {"white", "FFFFFF"},
        {"red", "FF0000"},
        {"green", "008000"},
        {"blue", "0000FF"},
        {"yellow", "FFFF00"},
        {"cyan", "00FFFF"},
        {"magenta", "FF00FF"},
        {"gray", "808080"},
        {"grey", "808080"},
        {"silver", "C0C0C0"},
        {"maroon", "800000"},
        {"olive", "808000"},
        {"lime", "00FF00"},
        {"aqua", "00FFFF"},
        {"teal", "008080"},
        {"navy", "000080"},
        {"fuchsia", "FF00FF"},
        {"purple", "800080"},
        {"orange", "FFA500"},
        {"transparent", "FFFFFF"},
        {"", ""} 
    };

    /* skip !important suffix for named colors */
    size_t name_len = 0;

    while (p[name_len] && p[name_len] != '!' &&
        !isspace((unsigned char)p[name_len]))
        name_len++;

    /* quick length-based filtering */
    if (name_len >= 3 && name_len <= 11) {
        for (int i = 0; color_map[i].name[0] != '\0'; i++) {
            /* fast length check first */
            if (strlen(color_map[i].name) != name_len) continue;

            /* first character check */
            if ((color_map[i].name[0] | 0x20) != (p[0] | 0x20)) continue;

            /* full case-insensitive comparison */
            int match = 1;

            for (size_t j = 0; j < name_len; j++) {
                if ((color_map[i].name[j] | 0x20) != (p[j] | 0x20)) {
                    match = 0;
                    break;
                }
            }

            if (match)
                return strdup(color_map[i].hex);
        }
    }

    /* default fallback */
    return strdup("000000");
}

int is_css_property_inheritable(const char* property_name) {
    static const struct {
        const char* property_name;
        unsigned char first_char;
        const unsigned char length;
    } inheritable_properties[] = {
        {"font-weight", 'f', 11}, {"font-style", 'f', 10}, {"font-family", 'f', 11},
        {"font-size", 'f', 9}, {"color", 'c', 5}, {"text-align", 't', 10},
        {"text-decoration", 't', 15}, {NULL, 0, 0}
    };

    size_t len = 0;
    const char* p = property_name;

    while (*p) {
        len++; p++;

        /* CSS property name is longer then expected */
        if (len > 15) return 0;
    }

    /* length-based fast rejection */
    switch (len) {
    case 5:  case 9:  case 10:
    case 11: case 15:
        break;
    default:
        /* length does not match any known CSS property */
        return 0;
    }

    /* extract first char once */
    const unsigned char first_char = (unsigned char)property_name[0];

    for (int i = 0; inheritable_properties[i].property_name; i++) {
        /* fast reject: first character mismatch */
        if (first_char != inheritable_properties[i].first_char) continue;

        /* medium reject: length mismatch, cheaper than strcmp */
        if (inheritable_properties[i].length != len) continue;

        /* apply strcmp function for exact string match (only few cases) */
        if (strcmp(property_name, inheritable_properties[i].property_name) == 0)
            return 1;
    }

    return 0;
}