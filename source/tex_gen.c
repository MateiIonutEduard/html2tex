#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_CAPACITY 1024
#define GROWTH_FACTOR 2

static void ensure_capacity(LaTeXConverter* converter, size_t needed) {
    /* already enough capacity */
    if (converter->output_capacity - converter->output_size > needed)
        return;

    /* initialize if this is the first allocation */
    if (converter->output_capacity == 0) {
        converter->output_capacity = INITIAL_CAPACITY;
        converter->output = malloc(converter->output_capacity);

        if (!converter->output) {
            converter->error_code = 1;
            strncpy(converter->error_message,
                "Memory allocation failed.",
                sizeof(converter->error_message) - 1);
            return;
        }

        converter->output[0] = '\0';
        converter->output_size = 0;

        /* re-check capacity after initialization */
        if (converter->output_capacity - converter->output_size > needed)
            return;
    }

    /* compute new capacity with overflow protection */
    size_t required = converter->output_size + needed + 1;
    size_t new_capacity = converter->output_capacity;

    /* exponential growth with overflow protection */
    while (new_capacity < required) {
        if (new_capacity > SIZE_MAX / GROWTH_FACTOR) {
            /* overflow would occur, fall back to linear growth */
            if (required > SIZE_MAX - new_capacity) {
                converter->error_code = 2;
                strncpy(converter->error_message,
                    "Buffer capacity overflow.",
                    sizeof(converter->error_message) - 1);
                return;
            }

            new_capacity = required;
        }
        else
            new_capacity *= GROWTH_FACTOR;
    }

    /* ensure new capacity is sufficient */
    if (new_capacity < required)
        new_capacity = required;

    /* reallocate the memory */
    void* new_output = realloc(converter->output, new_capacity);

    if (!new_output) {
        /* keep old buffer intact for possible recovery */
        converter->error_code = 3;

        strncpy(converter->error_message,
            "Memory reallocation failed.",
            sizeof(converter->error_message) - 1);
        return;
    }

    converter->output = new_output;
    converter->output_capacity = new_capacity;
}

void append_string(LaTeXConverter* converter, const char* str) {
    if (!converter || !str) {
        if (converter) {
            converter->error_code = 4;
            strncpy(converter->error_message,
                "NULL parameter to append_string() function.",
                sizeof(converter->error_message) - 1);
        }

        return;
    }

    size_t len = strlen(str);
    if (len == 0) return;

    ensure_capacity(converter, len + 1);
    if (converter->error_code) return;

    /* direct memory copy to the current position */
    char* dest = converter->output + converter->output_size;
    memcpy(dest, str, len);

    dest[len] = '\0';
    converter->output_size += len;
}

/* Append a single character to the LaTeX output buffer. */
static void append_char(LaTeXConverter* converter, char c) {
    if (!converter) return;
    ensure_capacity(converter, 2);

    if (converter->error_code) return;
    char* dest = converter->output + converter->output_size;

    *dest++ = c; *dest = '\0';
    converter->output_size++;
}

static void escape_latex_special(LaTeXConverter* converter, const char* text) {
    if (!text || !converter) return;

    /* lookup table for LaTeX special characters */
    static const unsigned char SPECIAL_SP[256] = {
        ['{'] = 1,['}'] = 2,['&'] = 3,['%'] = 4,
        ['$'] = 5,['#'] = 6,['^'] = 7,['~'] = 8,
        ['<'] = 9,['>'] = 10,['\n'] = 11
    };

    static const char* const ESCAPED_SP[] = {
        NULL, "\\{", "\\}", "\\&", "\\%", "\\$", 
        "\\#", "\\^{}", "\\~{}", "\\textless{}", 
        "\\textgreater{}", "\\\\"
    };

    const char* p = text;
    const char* start = p;

    while (*p) {
        unsigned char c = (unsigned char)*p;
        unsigned char type = SPECIAL_SP[c];

        if (type == 0) {
            p++;
            continue;
        }

        if (p > start) {
            /* copy normal characters before special */
            size_t normal_len = p - start;

            ensure_capacity(converter, normal_len);
            if (converter->error_code) return;

            char* dest = converter->output + converter->output_size;
            memcpy(dest, start, normal_len);

            converter->output_size += normal_len;
            converter->output[converter->output_size] = '\0';
        }

        /* append the escaped sequence */
        append_string(converter, ESCAPED_SP[type]);
        if (converter->error_code) return;
        p++; start = p;
    }

    if (p > start) {
        /* copy remaining normal characters */
        size_t remaining_len = p - start;

        ensure_capacity(converter, remaining_len);
        if (converter->error_code) return;

        char* dest = converter->output + converter->output_size;
        memcpy(dest, start, remaining_len);

        converter->output_size += remaining_len;
        converter->output[converter->output_size] = '\0';
    }
}

static void escape_latex(LaTeXConverter* converter, const char* text) {
    if (!text || !converter) return;

    /* lookup tables for LaTeX special characters */
    static const unsigned char key[256] = {
        ['\\'] = 1,['{'] = 2,['}'] = 3,['&'] = 4,
        ['%'] = 5,['$'] = 6,['#'] = 7,['_'] = 8,
        ['^'] = 9,['~'] = 10,['<'] = 11,['>'] = 12,
        ['\n'] = 13
    };

    static const char* const value[] = {
        NULL, "\\textbackslash{}", "\\{", "\\}", "\\&",
        "\\%", "\\$", "\\#", "\\_", "\\^{}", "\\~{}", 
        "\\textless{}", "\\textgreater{}", "\\\\"
    };

    const char* p = text;
    const char* start = p;

    while (*p) {
        unsigned char c = (unsigned char)*p;
        unsigned char type = key[c];

        if (type == 0) {
            p++;
            continue;
        }

        if (p > start) {
            /* copy normal characters before special */
            size_t normal_len = p - start;

            ensure_capacity(converter, normal_len);
            if (converter->error_code) return;

            char* dest = converter->output + converter->output_size;
            memcpy(dest, start, normal_len);

            converter->output_size += normal_len;
            converter->output[converter->output_size] = '\0';
        }

        /* append escaped sequence */
        append_string(converter, value[type]);
        if (converter->error_code) return;
        p++; start = p;
    }

    if (p > start) {
        /* copy remaining normal characters */
        size_t remaining_len = p - start;

        ensure_capacity(converter, remaining_len);
        if (converter->error_code) return;

        char* dest = converter->output + converter->output_size;
        memcpy(dest, start, remaining_len);

        converter->output_size += remaining_len;
        converter->output[converter->output_size] = '\0';
    }
}

static void convert_node(LaTeXConverter* converter, HTMLNode* node, CSSProperties* inherited_props);

void convert_children(LaTeXConverter* converter, HTMLNode* node, CSSProperties* inherited_props) {
    HTMLNode* child = node->children;

    while (child) {
        convert_node(converter, child, inherited_props);
        child = child->next;
    }
}

static void begin_environment(LaTeXConverter* converter, const char* env) {
    if (!converter || !env) {
        if (converter) {
            converter->error_code = 5;
            strncpy(converter->error_message,
                "NULL parameter to begin_environment() function.",
                sizeof(converter->error_message) - 1);
        }

        return;
    }

    append_string(converter, "\\begin{");
    if (converter->error_code) return;

    append_string(converter, env);
    if (converter->error_code) return;
    append_string(converter, "}\n");
}

static void end_environment(LaTeXConverter* converter, const char* env) {
    if (!converter || !env) {
        if (converter) {
            converter->error_code = 6;
            strncpy(converter->error_message,
                "NULL parameter to end_environment() function.",
                sizeof(converter->error_message) - 1);
        }

        return;
    }

    append_string(converter, "\\end{");
    if (converter->error_code) return;

    append_string(converter, env);
    if (converter->error_code) return;
    append_string(converter, "}\n");
}

static char* extract_color_from_style(const char* style, const char* property) {
    if (!style || !property || !*property) return NULL;
    const char* p = style;

    /* skip leading whitespace */
    size_t prop_len = strlen(property);

    while (*p) {
        while (*p && (*p == ' ' || *p == '\t')) p++;
        if (!*p) break;

        /* find property end */
        const char* prop_start = p;
        while (*p && *p != ':' && *p != ';' && !(*p == ' ' || *p == '\t')) p++;

        /* check if we have a match */
        size_t current_prop_len = p - prop_start;

        if (current_prop_len == prop_len &&
            strncmp(prop_start, property, prop_len) == 0) {

            /* skip to value */
            while (*p && *p != ':') p++;
            if (!*p) break; p++;

            /* skip whitespace before value */
            while (*p && (*p == ' ' || *p == '\t')) p++;
            if (!*p) break;

            /* find value end */
            const char* value_start = p;
            const char* value_end = p;

            while (*p && *p != ';') {
                if (*p != ' ' && *p != '\t')
                    value_end = p + 1;
                
                p++;
            }

            /* remove !important from value */
            const char* important_pos = NULL;

            for (const char* v = value_start; v < value_end; v++) {
                if (v + 9 <= value_end && strncasecmp(v, "!important", 9) == 0) {
                    important_pos = v;
                    break;
                }
            }

            if (important_pos) {
                /* trim trailing whitespace after !important removal */
                value_end = important_pos;
                
                while (value_end > value_start && (value_end[-1] == ' ' || value_end[-1] == '\t'))
                    value_end--;
            }

            /* extract the value */
            if (value_end > value_start) {
                size_t value_len = value_end - value_start;
                char* result = (char*)malloc(value_len + 1);

                if (result) {
                    memcpy(result, value_start, value_len);
                    result[value_len] = '\0';
                    return result;
                }
            }
        }

        /* skip to the next property */
        while (*p && *p != ';') p++;
        if (*p == ';') p++;
    }

    return NULL;
}

static char* color_to_hex(const char* color_value) {
    if (!color_value || !*color_value) return NULL;

    /* check first character quickly */
    char first_char = color_value[0];

    /* #RRGGBB or #RGB format */
    if (first_char == '#') {
        const char* hex_start = color_value + 1;
        size_t hex_len = strlen(hex_start);

        /* validate hex length */
        if (hex_len != 3 && hex_len != 6) return NULL;

        /* validate hex characters */
        for (size_t i = 0; i < hex_len; i++) {
            char c = hex_start[i];

            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                (c >= 'A' && c <= 'F')))
                return NULL;
        }

        char* result = (char*)malloc(7);
        if (!result) return NULL;

        if (hex_len == 3) {
            /* expand #RGB to #RRGGBB */
            snprintf(result, 7, "%c%c%c%c%c%c",
                hex_start[0], hex_start[0],
                hex_start[1], hex_start[1],
                hex_start[2], hex_start[2]);
        }
        else {
            /* #RRGGBB format */
            memcpy(result, hex_start, 6);
            result[6] = '\0';
        }

        /* convert to uppercase during copy to avoid second pass */
        for (size_t i = 0; i < 6; i++)
            result[i] = toupper((unsigned char)result[i]);

        return result;
    }

    /* RGB/RGBA format */
    if (first_char == 'r') {
        /* Quick check for "rgb(" or "rgba(" */
        if (strncmp(color_value, "rgb(", 4) == 0) {
            int r, g, b;
            if (sscanf(color_value, "rgb(%d, %d, %d)", &r, &g, &b) == 3 &&
                r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
                char* result = malloc(7);
                if (result) {
                    snprintf(result, 7, "%02X%02X%02X", r, g, b);
                }
                return result;
            }
        }
        else if (strncmp(color_value, "rgba(", 5) == 0) {
            int r, g, b;
            float a;
            if (sscanf(color_value, "rgba(%d, %d, %d, %f)", &r, &g, &b, &a) == 4 &&
                r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
                char* result = malloc(7);
                if (result) {
                    snprintf(result, 7, "%02X%02X%02X", r, g, b);
                }
                return result;
            }
        }
        return NULL;
    }

    /* named colors lookup table for commonly CSS colors */
    static const struct {
        const char* name;
        const char* hex;
    } named_colors[] = {
        {"black", "000000"}, {"white", "FFFFFF"},
        {"red", "FF0000"}, {"green", "008000"}, {"blue", "0000FF"},
        {"yellow", "FFFF00"}, {"cyan", "00FFFF"}, {"magenta", "FF00FF"},
        {"gray", "808080"}, {"grey", "808080"}, {"silver", "C0C0C0"},
        {"maroon", "800000"}, {"olive", "808000"}, {"lime", "00FF00"},
        {"aqua", "00FFFF"}, {"teal", "008080"}, {"navy", "000080"},
        {"fuchsia", "FF00FF"}, {"purple", "800080"}, {"orange", "FFA500"},
        {"transparent", "000000"}, {NULL, NULL}
    };

    /* case-insensitive lookup for named colors */
    for (int i = 0; named_colors[i].name; i++) {
        if (strcasecmp(color_value, named_colors[i].name) == 0)
            return strdup(named_colors[i].hex);
    }

    /* unknown format, return NULL for safety */
    return NULL;
}

/* Handle color application with proper nesting. */
static void apply_color(LaTeXConverter* converter, const char* color_value, int is_background) {
    if (!converter || !color_value) {
        if (converter) {
            converter->error_code = 7;
            strncpy(converter->error_message,
                "NULL parameter to apply_color() function.",
                sizeof(converter->error_message) - 1);
        }

        return;
    }

    char* hex_color = color_to_hex(color_value);

    if (!hex_color) {
        converter->error_code = 8;
        strncpy(converter->error_message,
            "Failed to convert color to hex",
            sizeof(converter->error_message) - 1);

        return;
    }

    /* pre-allocate buffer for the entire command to reduce function calls */
    size_t prefix_len = is_background ? 17 : 16;
    size_t hex_len = strlen(hex_color);

    size_t suffix_len = 2;
    size_t total_len = prefix_len + hex_len + suffix_len;
    ensure_capacity(converter, total_len);

    if (converter->error_code) {
        free(hex_color);
        return;
    }

    char* dest = converter->output + converter->output_size;

    if (is_background) {
        memcpy(dest, "\\colorbox[HTML]{", 16);
        dest += 16;
    }
    else {
        memcpy(dest, "\\textcolor[HTML]{", 17);
        dest += 17;
    }

    memcpy(dest, hex_color, hex_len);
    dest += hex_len;

    memcpy(dest, "}{", 2);
    dest += 2;

    converter->output_size += total_len;
    *dest = '\0'; free(hex_color);
}

static void begin_table(LaTeXConverter* converter, int columns) {
    if (!converter) 
        return;

    if (columns <= 0) {
        converter->error_code = 9;
        strncpy(converter->error_message,
            "Invalid column count for table.",
            sizeof(converter->error_message) - 1);

        return;
    }

    converter->state.table_internal_counter++;
    converter->state.in_table = 1;

    converter->state.table_columns = columns;
    converter->state.current_column = 0;

    /* free existing caption if any */
    if (converter->state.table_caption) {
        free(converter->state.table_caption);
        converter->state.table_caption = NULL;
    }

    /* compute the required buffer size */
    size_t header_len = strlen("\\begin{table}[h]\n\\centering\n\\begin{tabular}{|");
    size_t column_part = (size_t)columns * 2;

    size_t footer_len = strlen("}\n\\hline\n");
    size_t total_len = header_len + column_part + footer_len;

    ensure_capacity(converter, total_len);
    if (converter->error_code) return;

    /* build the entire table header in one go */
    char* dest = converter->output + converter->output_size;

    /* copy the table header */
    memcpy(dest, "\\begin{table}[h]\n\\centering\n\\begin{tabular}{|", header_len);
    dest += header_len;

    /* fill column specifications */
    for (int i = 0; i < columns; i++) {
        *dest++ = 'c';
        *dest++ = '|';
    }

    /* copy the table footer */
    memcpy(dest, "}\n\\hline\n", footer_len);
    dest += footer_len;

    /* update the buffer state */
    converter->output_size += total_len;
    *dest = '\0';
}

static void end_table(LaTeXConverter* converter, const char* table_label) {
    if (!converter) {
        fprintf(stderr, "Error: NULL converter in end_table() function.\n");
        return;
    }

    /* quick exit if not in table */
    if (!converter->state.in_table) {
        converter->state.in_table = 0;
        converter->state.in_table_row = 0;

        converter->state.in_table_cell = 0;
        return;
    }

    /* build table end string efficiently*/
    const char* tabular_end = "\\end{tabular}\n";
    const char* table_end = "\\end{table}\n\n";

    /* pre-compute lengths for optimal memcpy usage */
    size_t total_len = 0;
    total_len += 14;

    char default_caption[32];
    const char* caption_text = NULL;
    size_t caption_len = 0;

    if (converter->state.table_caption) {
        caption_text = converter->state.table_caption;
        caption_len = strlen(caption_text);
        total_len += 9 + caption_len + 2;
    }
    else {
        snprintf(default_caption, sizeof(default_caption), "Table %d",
            converter->state.table_internal_counter);
        caption_text = default_caption;

        caption_len = strlen(caption_text);
        total_len += 9 + caption_len + 2;
    }

    size_t label_len = 0;
    const char* label_text = NULL;

    if (table_label && table_label[0] != '\0') {
        label_text = table_label;
        label_len = strlen(label_text);
        total_len += 11 + label_len + 2;
    }

    total_len += 13;

    /* ensure capacity */
    ensure_capacity(converter, total_len);

    if (converter->error_code) {
        /* clean up on error */
        if (converter->state.table_caption) {
            free(converter->state.table_caption);
            converter->state.table_caption = NULL;
        }

        return;
    }

    /* build complete output in one pass with memcpy() call */
    char* dest = converter->output + converter->output_size;
    memcpy(dest, tabular_end, 14);
    dest += 14;

    memcpy(dest, "\\caption{", 9);
    dest += 9;

    if (caption_len > 0) {
        memcpy(dest, caption_text, caption_len);
        dest += caption_len;
    }

    memcpy(dest, "}\n", 2);
    dest += 2;

    if (label_text && label_len > 0) {
        memcpy(dest, "\\label{tab:", 11);
        dest += 11;

        memcpy(dest, label_text, label_len);
        dest += label_len;

        memcpy(dest, "}\n", 2);
        dest += 2;
    }

    memcpy(dest, table_end, 13);
    dest += 13;

    /* update output size */
    converter->output_size += total_len;

    /* clean up resources */
    if (converter->state.table_caption) {
        free(converter->state.table_caption);
        converter->state.table_caption = NULL;
    }

    /* reset the converter state */
    converter->state.in_table = 0;
    converter->state.in_table_row = 0;
    converter->state.in_table_cell = 0;
}

static void begin_table_row(LaTeXConverter* converter) {
    if (!converter) {
        fprintf(stderr, "Error: NULL converter in begin_table_row() function.\n");
        return;
    }

    converter->state.in_table_row = 1;
    converter->state.current_column = 0;
}

static void end_table_row(LaTeXConverter* converter) {
    if (!converter) {
        fprintf(stderr, "Error: NULL converter in end_table_row() function.\n");
        return;
    }

    if (converter->state.in_table_row) {
        append_string(converter, " \\\\ \\hline\n");
        converter->state.in_table_row = 0;
    }
}

static void begin_table_cell(LaTeXConverter* converter, int is_header) {
    if (!converter) {
        fprintf(stderr, "Error: NULL converter in begin_table_cell() function.\n");
        return;
    }

    converter->state.in_table_cell = 1;
    converter->state.current_column++;

    if (converter->state.current_column > 1)
        append_string(converter, " & ");

    if (is_header)
        append_string(converter, "\\textbf{");
}

static void end_table_cell(LaTeXConverter* converter, int is_header) {
    if (!converter) {
        fprintf(stderr, "Error: NULL converter in end_table_cell() function.\n");
        return;
    }

    if (is_header)
        append_string(converter, "}");

    converter->state.in_table_cell = 0;
}

int count_table_columns(HTMLNode* node) {
    /* validate input */
    if (!node) return 1;
    int max_columns = 0;

    NodeQueue* front = NULL;
    NodeQueue* rear = NULL;

    /* initialize BFS queue with the node */
    if (!queue_enqueue(&front, &rear, node))
        return 1;

    /* BFS traversal for table structure */
    while (front) {
        HTMLNode* current = queue_dequeue(&front, &rear);
        if (!current) continue;

        /* process all children of current node */
        HTMLNode* child = current->children;

        while (child) {
            /* skip non-tag nodes */
            if (!child->tag) {
                child = child->next;
                continue;
            }

            /* fast tag identification using first char */
            char first_char = child->tag[0];

            /* skip caption elements immediately */
            if (first_char == 'c' && strcmp(child->tag, "caption") == 0) {
                child = child->next;
                continue;
            }

            /* check for row element */
            if (first_char == 't' && strcmp(child->tag, "tr") == 0) {
                int row_columns = 0;
                HTMLNode* cell = child->children;

                /* count cells in this row with colspan support */
                while (cell) {
                    if (cell->tag) {
                        char cell_first_char = cell->tag[0];

                        /* check for td or th with single comparison */
                        if ((cell_first_char == 't' && (strcmp(cell->tag, "td") == 0 || strcmp(cell->tag, "th") == 0))) {
                            int colspan = 1;

                            /* check for colspan attribute */
                            const char* colspan_attr = get_attribute(cell->attributes, "colspan");

                            /* robust conversion with error checking */
                            if (colspan_attr && colspan_attr[0]) {
                                char* endptr = NULL;
                                long colspan_val = strtol(colspan_attr, &endptr, 10);

                                /* validate conversion result */
                                if (endptr != colspan_attr && *endptr == '\0' &&
                                    colspan_val > 0 && colspan_val <= 1000) {
                                    colspan = (int)colspan_val;
                                }
                                /* handle negative or invalid values safely */
                                else if (colspan_val < 1)
                                    colspan = 1;
                            }

                            /* safe addition with overflow protection */
                            if (row_columns <= INT_MAX - colspan)
                                row_columns += colspan;
                            else
                                /* handle overflow */
                                row_columns = INT_MAX;
                        }
                    }

                    cell = cell->next;
                }

                /* update maximum columns found */
                if (row_columns > max_columns)
                    max_columns = row_columns;
            }
            /* handle table sections with BFS */
            else if (first_char == 't' || first_char == 'h' || first_char == 'f') {
                if (strcmp(child->tag, "thead") == 0 ||
                    strcmp(child->tag, "tbody") == 0 ||
                    strcmp(child->tag, "tfoot") == 0) {

                    /* enqueue section for processing */
                    if (!queue_enqueue(&front, &rear, child)) {
                        queue_cleanup(&front, &rear);
                        return max_columns > 0 ? max_columns : 1;
                    }
                }
            }
            /* handle direct table children that might contain rows */
            else if (first_char == 't' && strcmp(child->tag, "table") == 0) {
                /* Nested table, process it independently */
                int nested_columns = count_table_columns(child);

                if (nested_columns > max_columns)
                    max_columns = nested_columns;
            }

            child = child->next;
        }
    }

    /* cleanup queue */
    queue_cleanup(&front, &rear);

    /* return at least 1 column for valid tables */
    return max_columns > 0 ? max_columns : 1;
}

/* Returns the extracted caption text. */
static char* extract_caption_text(HTMLNode* node) {
    if (!node) return NULL;

    /* use a simple dynamic string */
    size_t capacity = 256;
    char* buffer = (char*)malloc(capacity);

    if (!buffer) return NULL;
    size_t length = 0;
    buffer[0] = '\0';

    /* use a stack for DFS */
    HTMLNode* stack[256];
    int stack_top = -1;
    stack[++stack_top] = node;

    while (stack_top >= 0) {
        HTMLNode* current = stack[stack_top--];

        /* if it's a text node, append its content */
        if (current && !current->tag && current->content) {
            const char* text = current->content;
            size_t text_len = strlen(text);

            /* check if we need to grow the buffer */
            if (length + text_len + 1 > capacity) {
                capacity *= GROWTH_FACTOR;
                char* new_buffer = (char*)realloc(buffer, capacity);

                if (!new_buffer) {
                    free(buffer);
                    return NULL;
                }

                buffer = new_buffer;
            }

            memcpy(buffer + length, text, text_len);
            length += text_len;
            buffer[length] = '\0';
        }

        /* push children in reverse order so that they are processed in the original order */
        if (current->children) {
            int child_count = 0;
            HTMLNode* child = current->children;

            while (child) {
                child_count++;
                child = child->next;
            }

            /* allocate array to hold children in order */
            HTMLNode** children = (HTMLNode**)malloc(child_count * sizeof(HTMLNode*));

            if (children) {
                child = current->children;

                for (int i = 0; i < child_count && child; i++) {
                    children[i] = child;
                    child = child->next;
                }

                /* push in reverse order */
                for (int i = child_count - 1; i >= 0; i--) {
                    if (stack_top < 255)
                        stack[++stack_top] = children[i];
                }

                free(children);
            }
        }
    }

    if (length == 0) {
        free(buffer);
        return NULL;
    }

    /* trim the buffer */
    char* result = (char*)realloc(buffer, length + 1);
    return result ? result : buffer;
}

void process_table_image(LaTeXConverter* converter, HTMLNode* img_node) {
    if (!converter || !img_node) return;

    const char* src = get_attribute(img_node->attributes, "src");
    if (!src || src[0] == '\0') return;

    char* image_path = NULL;
    int is_downloaded_image = 0;

    if (converter->download_images && converter->image_output_dir) {
        converter->image_counter++;
        image_path = download_image_src(src, converter->image_output_dir,
            converter->image_counter);

        if (image_path) {
            size_t dir_len = strlen(converter->image_output_dir);
            is_downloaded_image = (strncmp(image_path, converter->image_output_dir,
                dir_len) == 0);
        }
    }

    if (!image_path) {
        image_path = strdup(src);
        if (!image_path) return;
    }

    CSSProperties* img_css = NULL;
    int width_pt = 0, height_pt = 0;
    int has_background = 0;
    char* bg_hex_color = NULL;
    const char* style_attr = get_attribute(img_node->attributes, "style");

    if (style_attr) {
        img_css = parse_css_style(style_attr);
        if (img_css) {
            const char* width = css_properties_get(img_css, "width");
            const char* height = css_properties_get(img_css, "height");
            const char* bg_color = css_properties_get(img_css, "background-color");

            if (width) width_pt = css_length_to_pt(width);
            if (height) height_pt = css_length_to_pt(height);

            if (bg_color) {
                bg_hex_color = css_color_to_hex(bg_color);
                if (bg_hex_color && strcmp(bg_hex_color, "FFFFFF") != 0)
                    has_background = 1;
            }
        }
    }

    if (width_pt == 0) {
        const char* width_attr = get_attribute(img_node->attributes, "width");
        if (width_attr) width_pt = css_length_to_pt(width_attr);
    }

    if (height_pt == 0) {
        const char* height_attr = get_attribute(img_node->attributes, "height");
        if (height_attr) height_pt = css_length_to_pt(height_attr);
    }

    if (has_background && bg_hex_color) {
        append_string(converter, "\\colorbox[HTML]{");
        append_string(converter, bg_hex_color);
        append_string(converter, "}{");
    }

    append_string(converter, "\\includegraphics");

    if (width_pt > 0 || height_pt > 0) {
        append_string(converter, "[");

        if (width_pt > 0) {
            char width_str[32];
            snprintf(width_str, sizeof(width_str), "width=%dpt", width_pt);
            append_string(converter, width_str);
        }

        if (height_pt > 0) {
            if (width_pt > 0) append_string(converter, ",");
            char height_str[32];
            snprintf(height_str, sizeof(height_str), "height=%dpt", height_pt);
            append_string(converter, height_str);
        }

        append_string(converter, "]");
    }

    append_string(converter, "{");

    if (is_downloaded_image) {
        size_t dir_len = strlen(converter->image_output_dir);
        escape_latex_special(converter, image_path + dir_len + 1);
    }
    else {
        escape_latex(converter, image_path);
    }

    append_string(converter, "}");

    if (has_background)
        append_string(converter, "}");

    if (image_path) free(image_path);
    if (bg_hex_color) free(bg_hex_color);
    if (img_css) css_properties_destroy(img_css);
}

void append_figure_caption(LaTeXConverter* converter, HTMLNode* table_node) {
    if (!converter || !table_node) return;

    /* increment the counter */
    converter->state.figure_internal_counter++;
    int figure_counter = converter->state.figure_internal_counter;

    /* find caption */
    HTMLNode* caption = NULL;
    HTMLNode* child = table_node->children;

    while (child) {
        if (child->tag && child->tag[0] == 'c' &&
            strcmp(child->tag, "caption") == 0) {
            caption = child;
            break;
        }

        child = child->next;
    }

    char* caption_text = NULL;
    if (caption) caption_text = extract_caption_text(caption);

    /* build the label */
    const char* fig_id = get_attribute(table_node->attributes, "id");
    char figure_label[64];

    /* safe copy with bounds checking */
    if (fig_id && fig_id[0] != '\0') {
        size_t len = strlen(fig_id);
        size_t copy_len = (len < sizeof(figure_label) - 1) ? 
            len : sizeof(figure_label) - 1;

        strncpy(figure_label, fig_id, copy_len);
        figure_label[copy_len] = '\0';
    }
    else {
        snprintf(figure_label, sizeof(figure_label), 
            "figure_%d", figure_counter);
    }

    /* build complete string in one buffer */
    size_t max_len = 256;

    if (caption_text) max_len += (strlen(caption_text) << 4);
    ensure_capacity(converter, max_len);

    if (converter->error_code) {
        if (caption_text) free(caption_text);
        return;
    }

    /* minimize append_string function calls */
    append_string(converter, "\\caption{");

    if (caption_text) {
        escape_latex(converter, caption_text);
        free(caption_text);
    }
    else {
        append_string(converter, "Figure ");
        char counter_str[32];

        snprintf(counter_str, sizeof(counter_str), "%d", figure_counter);
        append_string(converter, counter_str);
    }

    append_string(converter, "}\n\\label{fig:");
    escape_latex_special(converter, figure_label);
    append_string(converter, "}\n");
}

void convert_node(LaTeXConverter* converter, HTMLNode* node, CSSProperties* inherited_props) {
    if (!node) return;

    /* skip nested tables */
    if (should_skip_nested_table(node))
        return;

    /* handle text nodes */
    if (!node->tag && node->content) {
        escape_latex(converter, node->content);
        return;
    }

    if (!node->tag) return;

    /* skip excluded elements */
    if (should_exclude_tag(node->tag))
        return;

    /* parse inline CSS if present */
    CSSProperties* inline_props = NULL;

    const char* style_attr = get_attribute(node->attributes, "style");
    if (style_attr) inline_props = parse_css_style(style_attr);

    /* merge inherited and inline properties */
    CSSProperties* merged_props = css_properties_merge(inherited_props, inline_props);

    /* apply CSS properties before element content */
    if (merged_props) css_properties_apply(converter, merged_props, node->tag);

    /* handle different HTML tags */
    if (strcmp(node->tag, "p") == 0) {
        append_string(converter, "\n");
        convert_children(converter, node, merged_props);
        append_string(converter, "\n\n");
    }
    else if (strcmp(node->tag, "h1") == 0) {
        append_string(converter, "\\section{");
        convert_children(converter, node, merged_props);
        append_string(converter, "}\n\n");
    }
    else if (strcmp(node->tag, "h2") == 0) {
        append_string(converter, "\\subsection{");
        convert_children(converter, node, merged_props);
        append_string(converter, "}\n\n");
    }
    else if (strcmp(node->tag, "h3") == 0) {
        append_string(converter, "\\subsubsection{");
        convert_children(converter, node, merged_props);
        append_string(converter, "}\n\n");
    }
    else if (strcmp(node->tag, "h4") == 0) {
        append_string(converter, "\\paragraph{");
        convert_children(converter, node, merged_props);
        append_string(converter, "}\n\n");
    }
    else if (strcmp(node->tag, "b") == 0 || strcmp(node->tag, "strong") == 0) {
        /* only apply bold if CSS hasn't already applied it */
        if (!(converter->state.applied_props & CSS_BOLD)) {
            append_string(converter, "\\textbf{");
            converter->state.applied_props |= CSS_BOLD;

            convert_children(converter, node, merged_props);
            append_string(converter, "}");
        }
        else
            convert_children(converter, node, merged_props);
    }
    else if (strcmp(node->tag, "i") == 0 || strcmp(node->tag, "em") == 0) {
        /* only apply italic if CSS hasn't already applied it */
        if (!(converter->state.applied_props & CSS_ITALIC)) {
            append_string(converter, "\\textit{");
            converter->state.applied_props |= CSS_ITALIC;

            convert_children(converter, node, merged_props);
            append_string(converter, "}");
        }
        else
            convert_children(converter, node, merged_props);
    }
    else if (strcmp(node->tag, "u") == 0) {
        if (!(converter->state.applied_props & CSS_UNDERLINE)) {
            append_string(converter, "\\underline{");
            converter->state.applied_props |= CSS_UNDERLINE;

            convert_children(converter, node, merged_props);
            append_string(converter, "}");
        }
        else
            convert_children(converter, node, merged_props);
    }
    else if (strcmp(node->tag, "code") == 0) {
        append_string(converter, "\\texttt{");
        convert_children(converter, node, merged_props);
        append_string(converter, "}");
    }
    else if (strcmp(node->tag, "font") == 0) {
        const char* color_attr = get_attribute(node->attributes, "color");
        char* text_color = NULL;

        if (style_attr)
            text_color = extract_color_from_style(style_attr, "color");

        if (merged_props && text_color)
            convert_children(converter, node, merged_props);
        else if (merged_props && !text_color) {
            if (color_attr) apply_color(converter, color_attr, 0);
            convert_children(converter, node, merged_props);
            append_string(converter, "}");
        }

        if (text_color) free(text_color);
    }
    else if (strcmp(node->tag, "span") == 0)
        convert_children(converter, node, merged_props);
    else if (strcmp(node->tag, "a") == 0) {
        const char* href = get_attribute(node->attributes, "href");

        if (href) {
            append_string(converter, "\\href{");
            escape_latex(converter, href);
            append_string(converter, "}{");

            convert_children(converter, node, merged_props);
            append_string(converter, "}");
        }
        else
            convert_children(converter, node, merged_props);
    }
    else if (strcmp(node->tag, "ul") == 0) {
        append_string(converter, "\\begin{itemize}\n");
        convert_children(converter, node, merged_props);
        append_string(converter, "\\end{itemize}\n");
    }
    else if (strcmp(node->tag, "ol") == 0) {
        append_string(converter, "\\begin{enumerate}\n");
        convert_children(converter, node, merged_props);
        append_string(converter, "\\end{enumerate}\n");
    }
    else if (strcmp(node->tag, "li") == 0) {
        append_string(converter, "\\item ");
        convert_children(converter, node, merged_props);
        append_string(converter, "\n");
    }
    else if (strcmp(node->tag, "br") == 0)
        append_string(converter, "\\\\\n");
    else if (strcmp(node->tag, "hr") == 0)
        append_string(converter, "\\hrulefill\n\n");
    else if (strcmp(node->tag, "div") == 0)
        convert_children(converter, node, merged_props);
    /* image support */
    else if (strcmp(node->tag, "img") == 0) {
        if (is_inside_table(node)) {
            const char* src = get_attribute(node->attributes, "src");
            const char* width_attr = get_attribute(node->attributes, "width");
            const char* height_attr = get_attribute(node->attributes, "height");

            if (src) {
                char* image_path = NULL;

                if (converter->download_images && converter->image_output_dir) {
                    converter->image_counter++;
                    image_path = download_image_src(src, converter->image_output_dir,
                        converter->image_counter);
                }

                if (!image_path) image_path = strdup(src);
                append_string(converter, "\\includegraphics");

                int width_pt = 0;
                int height_pt = 0;

                if (width_attr) width_pt = css_length_to_pt(width_attr);
                if (height_attr) height_pt = css_length_to_pt(height_attr);

                if (style_attr) {
                    CSSProperties* img_css = parse_css_style(style_attr);

                    if (img_css) {
                        const char* width = css_properties_get(img_css, "width");
                        const char* height = css_properties_get(img_css, "height");

                        if (width) width_pt = css_length_to_pt(width);
                        if (height) height_pt = css_length_to_pt(height);
                        css_properties_destroy(img_css);
                    }
                }

                if (width_pt > 0 || height_pt > 0) {
                    append_string(converter, "[");

                    if (width_pt > 0) {
                        char width_str[32];
                        snprintf(width_str, sizeof(width_str), "width=%dpt", width_pt);
                        append_string(converter, width_str);
                    }

                    if (height_pt > 0) {
                        if (width_pt > 0) append_string(converter, ",");
                        char height_str[32];
                        snprintf(height_str, sizeof(height_str), "height=%dpt", height_pt);
                        append_string(converter, height_str);
                    }

                    append_string(converter, "]");
                }

                append_string(converter, "{");
                if (converter->download_images && converter->image_output_dir &&
                    strstr(image_path, converter->image_output_dir) == image_path) {
                    escape_latex_special(converter, image_path + 2);
                }
                else
                    escape_latex(converter, image_path);
                append_string(converter, "}");
                free(image_path);
            }

            if (merged_props)
                css_properties_end(converter, merged_props, node->tag);

            /* cleanup CSS property holders */
            if (merged_props) css_properties_destroy(merged_props);
            if (inline_props) css_properties_destroy(inline_props);
            return;
        }
        else {
            converter->image_counter++;
            converter->state.image_internal_counter++;

            const char* src = get_attribute(node->attributes, "src");
            const char* alt = get_attribute(node->attributes, "alt");

            const char* width_attr = get_attribute(node->attributes, "width");
            const char* height_attr = get_attribute(node->attributes, "height");
            const char* image_id_attr = get_attribute(node->attributes, "id");

            if (src) {
                char* image_path = NULL;

                if (converter->download_images && converter->image_output_dir)
                    image_path = download_image_src(src, converter->image_output_dir,
                        converter->image_counter);

                if (!image_path) {
                    if (is_base64_image(src) && converter->download_images && converter->image_output_dir)
                        image_path = download_image_src(src, converter->image_output_dir,
                            converter->image_counter);

                    if (!image_path) image_path = strdup(src);
                }

                append_string(converter, "\n\n\\begin{figure}[h]\n");
                append_string(converter, "\\centering\n");

                int width_pt = 0;
                int height_pt = 0;

                if (merged_props) {
                    const char* width = css_properties_get(merged_props, "width");
                    const char* height = css_properties_get(merged_props, "height");

                    if (width) width_pt = css_length_to_pt(width);
                    if (height) height_pt = css_length_to_pt(height);
                }

                if (width_pt == 0 && width_attr) width_pt = css_length_to_pt(width_attr);
                if (height_pt == 0 && height_attr) height_pt = css_length_to_pt(height_attr);
                append_string(converter, "\\includegraphics");

                if (width_pt > 0 || height_pt > 0) {
                    append_string(converter, "[");

                    if (width_pt > 0) {
                        char width_str[32];
                        snprintf(width_str, sizeof(width_str), "width=%dpt", width_pt);
                        append_string(converter, width_str);
                    }

                    if (height_pt > 0) {
                        if (width_pt > 0) append_string(converter, ",");
                        char height_str[32];

                        snprintf(height_str, sizeof(height_str), "height=%dpt", height_pt);
                        append_string(converter, height_str);
                    }

                    append_string(converter, "]");
                }

                append_string(converter, "{");

                if (converter->download_images && converter->image_output_dir
                    && strstr(image_path, converter->image_output_dir) == image_path)
                    escape_latex_special(converter, image_path + 2);
                else
                    escape_latex(converter, image_path);
                append_string(converter, "}\n");

                if (alt && alt[0] != '\0') {
                    append_string(converter, "\n");
                    append_string(converter, "\\caption{");

                    escape_latex(converter, alt);
                    append_string(converter, "}\n");
                }
                else {
                    append_string(converter, "\\caption{");
                    char text_caption[64];

                    char caption_counter[32];
                    html2tex_itoa(converter->state.image_internal_counter, caption_counter, 10);

                    strcpy(text_caption, "Image ");
                    strcpy(text_caption + 6, caption_counter);

                    escape_latex(converter, text_caption);
                    append_string(converter, "}\n");
                }

                if (image_id_attr && image_id_attr[0] != '\0') {
                    append_string(converter, "\\label{fig:");
                    escape_latex(converter, image_id_attr);
                    append_string(converter, "}\n");
                }
                else {
                    append_string(converter, "\\label{fig:");
                    char image_label_id[64];

                    char label_counter[32];
                    html2tex_itoa(converter->state.image_internal_counter, label_counter, 10);

                    strcpy(image_label_id, "image_");
                    strcpy(image_label_id + 6, label_counter);

                    escape_latex_special(converter, image_label_id);
                    append_string(converter, "}\n");
                }

                append_string(converter, "\\end{figure}\n");
                append_string(converter, "\\FloatBarrier\n\n");
            }
        }
    }
    /* table support */
    else if (strcmp(node->tag, "table") == 0) {
        if (table_contains_only_images(node)) {
            convert_image_table(converter, node);

            if (merged_props)
                css_properties_end(converter, merged_props, node->tag);

            /* cleanup resources */
            if (merged_props) css_properties_destroy(merged_props);
            if (inline_props) css_properties_destroy(inline_props);
            return;
        }
        else {
            /* Rreset CSS for table element */
            converter->state.applied_props = 0;
            int columns = count_table_columns(node);

            begin_table(converter, columns);
            HTMLNode* child = node->children;

            while (child) {
                convert_node(converter, child, NULL);
                child = child->next;
            }

            const char* table_id = get_attribute(node->attributes, "id");
            if (table_id && table_id[0] != '\0') end_table(converter, table_id);
            else {
                char table_label[64];
                char label_counter[32];

                html2tex_itoa(converter->state.table_internal_counter,
                    label_counter, 10);

                strcpy(table_label, "table_");
                strcpy(table_label + 6, label_counter);
                end_table(converter, table_label);
            }

            converter->state.applied_props = 0;
        }
    }
    else if (strcmp(node->tag, "caption") == 0) {
        if (converter->state.in_table) {
            if (converter->state.table_caption) {
                free(converter->state.table_caption);
                converter->state.table_caption = NULL;
            }

            char* raw_caption = extract_caption_text(node);

            if (raw_caption) {
                CSSProperties* caption_css = NULL;
                if (style_attr) caption_css = parse_css_style(style_attr);

                if (caption_css) {
                    size_t buffer_size = strlen(raw_caption) * 2 + 256;
                    char* formatted_caption = malloc(buffer_size);

                    if (formatted_caption) {
                        formatted_caption[0] = '\0';
                        const char* color = css_properties_get(caption_css, "color");

                        if (color) {
                            char* hex_color = css_color_to_hex(color);

                            if (hex_color && strcmp(hex_color, "000000") != 0) {
                                strcat(formatted_caption, "\\textcolor[HTML]{");
                                strcat(formatted_caption, hex_color);

                                strcat(formatted_caption, "}{");
                                free(hex_color);
                            }
                        }

                        int has_bold = 0;
                        const char* font_weight = css_properties_get(caption_css, "font-weight");

                        if (font_weight &&
                            (strcmp(font_weight, "bold") == 0 ||
                                strcmp(font_weight, "bolder") == 0)) {
                            strcat(formatted_caption, "\\textbf{");
                            has_bold = 1;
                        }

                        strcat(formatted_caption, raw_caption);
                        if (has_bold) strcat(formatted_caption, "}");

                        if (color) {
                            char* hex_color = css_color_to_hex(color);

                            if (hex_color && strcmp(hex_color, "000000") != 0) {
                                strcat(formatted_caption, "}");
                                free(hex_color);
                            }
                        }

                        converter->state.table_caption = formatted_caption;
                    }

                    css_properties_destroy(caption_css);
                }
                else
                    converter->state.table_caption = raw_caption;
            }

            /* cleanup memory containers */
            if (merged_props) css_properties_destroy(merged_props);
            if (inline_props) css_properties_destroy(inline_props);
            return;
        }
        else
            convert_children(converter, node, merged_props);
    }
    else if (strcmp(node->tag, "thead") == 0 ||
        strcmp(node->tag, "tbody") == 0 ||
        strcmp(node->tag, "tfoot") == 0)
        convert_children(converter, node, merged_props);
    else if (strcmp(node->tag, "tr") == 0) {
        converter->state.current_column = 0;
        converter->state.applied_props = 0;
        begin_table_row(converter);

        convert_children(converter, node, merged_props);
        end_table_row(converter);
    }
    else if (strcmp(node->tag, "td") == 0 || strcmp(node->tag, "th") == 0) {
        int is_header = (strcmp(node->tag, "th") == 0);
        const char* colspan_attr = get_attribute(node->attributes, "colspan");
        int colspan = 1;

        if (colspan_attr) {
            colspan = atoi(colspan_attr);
            if (colspan < 1) colspan = 1;
        }

        if (converter->state.current_column > 0) append_string(converter, " & ");
        int saved_css_braces = converter->state.css_braces;
        if (merged_props) css_properties_apply(converter, merged_props, node->tag);

        if (is_header && !(converter->state.applied_props & CSS_BOLD))
            append_string(converter, "\\textbf{");

        converter->state.in_table_cell = 1;
        convert_children(converter, node, merged_props);
        converter->state.in_table_cell = 0;

        if (is_header && !(converter->state.applied_props & CSS_BOLD)) append_string(converter, "}");
        int braces_opened_in_this_cell = converter->state.css_braces - saved_css_braces;

        for (int i = 0; i < braces_opened_in_this_cell; i++)
            append_string(converter, "}");
        
        converter->state.css_braces = saved_css_braces;
        if (merged_props) css_properties_end(converter, merged_props, node->tag);
        converter->state.current_column += colspan;

        for (int i = 1; i < colspan; i++) {
            converter->state.current_column++;

            if (converter->state.current_column > 0)
                append_string(converter, " & ");

            append_string(converter, " ");
        }
    }
    else
        convert_children(converter, node, merged_props);

    /* end of CSS properties */
    if (merged_props)
        css_properties_end(converter, merged_props, node->tag);

    /* release memory allocated for CSS properties containers */
    if (merged_props) css_properties_destroy(merged_props);
    if (inline_props) css_properties_destroy(inline_props);
}