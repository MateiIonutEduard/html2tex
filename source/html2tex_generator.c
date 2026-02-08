#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void append_string(LaTeXConverter* converter, const char* str) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in append_string().");
        return;
    }

    if (string_buffer_append(converter->buffer, str, 0) != 0) {
        if (!html2tex_has_error()) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                "Failed to append string to buffer.");
        }
    }
}

/* Append a single character to the LaTeX output buffer. */
static void append_char(LaTeXConverter* converter, char c) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in append_char().");
        return;
    }

    if (string_buffer_append_char(converter->buffer, c) != 0) {
        if (!html2tex_has_error()) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                "Failed to append character to buffer.");
        }
    }
}

void escape_latex_special(LaTeXConverter* converter, const char* text) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in escape_latex_special().");
        return;
    }

    /* optimized lookup table for LaTeX special characters (subset) */
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

        /* copy normal characters before special */
        if (p > start) {
            size_t normal_len = p - start;
            if (string_buffer_append(converter->buffer, start, normal_len) != 0) {
                if (!html2tex_has_error()) {
                    HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                        "Failed to append text during escape.");
                }
                return;
            }
        }

        /* append escaped sequence using existing StringBuffer */
        if (string_buffer_append(converter->buffer, ESCAPED_SP[type], 0) != 0) {
            if (!html2tex_has_error()) {
                HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                    "Failed to append escaped sequence.");
            }
            return;
        }

        p++;
        start = p;
    }

    /* copy remaining normal characters */
    if (p > start) {
        size_t remaining_len = p - start;
        if (string_buffer_append(converter->buffer, start, remaining_len) != 0) {
            if (!html2tex_has_error()) {
                HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                    "Failed to append remaining text.");
            }
        }
    }
}

void escape_latex(LaTeXConverter* converter, const char* text) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in escape_latex().");
        return;
    }

    /* use the existing StringBuffer utility for full LaTeX escaping */
    if (string_buffer_append_latex(converter->buffer, text) != 0) {
        if (!html2tex_has_error()) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                "Failed to escape LaTeX text.");
        }
    }
}

static void begin_environment(LaTeXConverter* converter, const char* env) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in begin_environment().");
        return;
    }

    if (!env) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL parameter to begin_environment().");
        return;
    }

    append_string(converter, "\\begin{");
    if (html2tex_has_error()) return;

    append_string(converter, env);
    if (html2tex_has_error()) return;
    append_string(converter, "}\n");
}

static void end_environment(LaTeXConverter* converter, const char* env) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in end_environment().");
        return;
    }

    if (!env) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL parameter to end_environment().");
        return;
    }

    append_string(converter, "\\end{");
    if (html2tex_has_error()) return;
    append_string(converter, env);

    if (html2tex_has_error()) return;
    append_string(converter, "}\n");
}

void apply_color(LaTeXConverter* converter, const char* color_value, int is_background) {
    if (!converter || !color_value || !converter->buffer) return;
    char* hex_color = css_color_to_hex(color_value);

    if (!hex_color) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_CSS_VALUE,
            "Failed to convert color to hex.");
        return;
    }

    char buffer[128];
    int len;

    if (is_background) len = snprintf(buffer, sizeof(buffer), "\\colorbox[HTML]{%s}{", hex_color);
    else len = snprintf(buffer, sizeof(buffer), "\\textcolor[HTML]{%s}{", hex_color);

    if (len > 0 && (size_t)len < sizeof(buffer)) {
        if (string_buffer_append(converter->buffer, buffer, (size_t)len) != 0) {
            free(hex_color);
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                "Failed to apply color.");
            return;
        }
    }

    free(hex_color);
}

void begin_table(LaTeXConverter* converter, int columns) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in begin_table().");
        return;
    }

    if (columns <= 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_TABLE,
            "Invalid column count for table.");
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

    /* use append_string for building table header efficiently */
    append_string(converter, "\\begin{table}[h]\n\\centering\n\\begin{tabular}{|");
    if (html2tex_has_error()) return;

    /* append column specifications using efficient character appending */
    for (int i = 0; i < columns; i++) {
        append_string(converter, "c|");
        if (html2tex_has_error()) return;
    }

    append_string(converter, "}\n\\hline\n");
}

void end_table(LaTeXConverter* converter, const char* table_label) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in end_table().");
        return;
    }

    /* quick exit if not in table */
    if (!converter->state.in_table) {
        converter->state.in_table = 0;
        converter->state.in_table_row = 0;
        converter->state.in_table_cell = 0;
        return;
    }

    /* end tabular */
    append_string(converter, "\\end{tabular}\n");
    if (html2tex_has_error())
        goto cleanup;

    /* add the text caption */
    append_string(converter, "\\caption{");
    if (html2tex_has_error())
        goto cleanup;

    if (converter->state.table_caption) {
        append_string(converter, converter->state.table_caption);
        if (html2tex_has_error()) goto cleanup;
    }
    else {
        char default_caption[32];
        snprintf(default_caption, sizeof(default_caption),
            "Table %d", converter->state.table_internal_counter);
        append_string(converter, default_caption);

        if (html2tex_has_error())
            goto cleanup;
    }

    append_string(converter, "}\n");
    if (html2tex_has_error())
        goto cleanup;

    /* add label if provided */
    if (table_label && table_label[0] != '\0') {
        append_string(converter, "\\label{tab:");
        if (html2tex_has_error()) goto cleanup;

        append_string(converter, table_label);
        if (html2tex_has_error())
            goto cleanup;

        append_string(converter, "}\n");
        if (html2tex_has_error())
            goto cleanup;
    }

    /* end table environment */
    append_string(converter, "\\end{table}\n\n");
    if (html2tex_has_error()) goto cleanup;

cleanup:
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

void begin_table_row(LaTeXConverter* converter) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in begin_table_row().");
        return;
    }

    converter->state.in_table_row = 1;
    converter->state.current_column = 0;
}

void end_table_row(LaTeXConverter* converter) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in end_table_row().");
        return;
    }

    if (converter->state.in_table_row) {
        append_string(converter, " \\\\ \\hline\n");
        converter->state.in_table_row = 0;
    }
}

static void begin_table_cell(LaTeXConverter* converter, int is_header) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in begin_table_cell().");
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
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in end_table_cell().");
        return;
    }

    if (is_header)
        append_string(converter, "}");

    converter->state.in_table_cell = 0;
}

int count_table_columns(const HTMLNode* node) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!node) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "HTML node is NULL in count_table_columns().");
        return 0;
    }

    int max_columns = 0;
    Queue* front = NULL;
    Queue* rear = NULL;

    /* try to enqueue node to the BFS queue */
    if (!queue_enqueue(&front, &rear, (HTMLNode*)node))
        return 0;

    /* BFS traversal for table structure */
    while (front) {
        HTMLNode* current = (HTMLNode*)queue_dequeue(&front, &rear);
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

char* extract_caption_text(const HTMLNode* node) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!node) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "HTML node is NULL in extract_caption_text().");
        return NULL;
    }

    /* optimized dynamic string buffer */
    size_t capacity = 256;

    char* buffer = (char*)malloc(capacity);
    if (!buffer) return NULL;

    size_t length = 0;
    buffer[0] = '\0';

    /* use generic stack for iterative DFS */
    Stack* stack = NULL;

    /* start with root node */
    if (!stack_push(&stack, (void*)node)) {
        free(buffer);
        return NULL;
    }

    /* process nodes depth-first */
    while (!stack_is_empty(stack)) {
        HTMLNode* current = (HTMLNode*)stack_pop(&stack);
        if (!current) continue;

        /* process text content */
        if (!current->tag && current->content && current->content[0]) {
            const char* text = current->content;
            size_t text_len = strlen(text);

            /* grow buffer if needed using geometric progression */
            if (length + text_len + 1 > capacity) {
                /* double capacity, but ensure minimum growth */
                size_t new_capacity = capacity * 2;
                if (new_capacity < length + text_len + 1)
                    new_capacity = length + text_len + 1;

                /* overflow protection */
                if (new_capacity < capacity || new_capacity > SIZE_MAX / 2) {
                    stack_cleanup(&stack);
                    free(buffer);
                    return NULL;
                }

                char* new_buffer = (char*)realloc(buffer, new_capacity);

                if (!new_buffer) {
                    stack_cleanup(&stack);
                    free(buffer);
                    return NULL;
                }

                buffer = new_buffer;
                capacity = new_capacity;
            }

            /* append text content */
            memcpy(buffer + length, text, text_len);
            length += text_len;
            buffer[length] = '\0';
        }

        /* push children in reverse order for original processing order */
        if (current->children) {
            /* count children for optimal memory */
            int child_count = 0;
            HTMLNode* child = current->children;

            while (child) {
                child_count++;
                child = child->next;
            }

            /* handle single child without temp stack */
            if (child_count == 1) {
                if (!stack_push(&stack, (void*)current->children)) {
                    stack_cleanup(&stack);
                    free(buffer);
                    return NULL;
                }
            }
            else {
                /* use temporary stack for reversing children order */
                Stack* temp_stack = NULL;
                child = current->children;

                /* push all children to temp stack */
                while (child) {
                    if (!stack_push(&temp_stack, (void*)child)) {
                        stack_cleanup(&temp_stack);
                        stack_cleanup(&stack);
                        free(buffer);
                        return NULL;
                    }

                    child = child->next;
                }

                /* transfer from temp to main stack in reverses order */
                while (!stack_is_empty(temp_stack)) {
                    HTMLNode* data = (HTMLNode*)stack_pop(&temp_stack);

                    if (!stack_push(&stack, (void*)data)) {
                        stack_cleanup(&temp_stack);
                        stack_cleanup(&stack);

                        free(buffer);
                        return NULL;
                    }
                }

                stack_cleanup(&temp_stack);
            }
        }
    }

    /* clean up stack */
    stack_cleanup(&stack);

    /* return NULL for empty captions */
    if (length == 0) {
        free(buffer);
        return NULL;
    }

    /* trim to exact size */
    char* result = (char*)realloc(buffer, length + 1);
    return result ? result : buffer;
}

void process_table_image(LaTeXConverter* converter, const HTMLNode* img_node) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in process_table_image().");
        return;
    }

    if (!img_node) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "HTML node is NULL in process_table_image().");
        return;
    }

    const char* src = get_attribute(img_node->attributes, "src");
    if (!src || src[0] == '\0') return;

    char* image_path = NULL;
    int is_downloaded_image = 0;

    if (converter->download_images && converter->image_output_dir) {
        converter->image_counter++;
        image_path = download_image_src(
            &converter->store, src, 
            converter->image_output_dir,
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
    else
        escape_latex(converter, image_path);
    append_string(converter, "}");

    if (has_background)
        append_string(converter, "}");

    if (image_path) free(image_path);
    if (bg_hex_color) free(bg_hex_color);
    if (img_css) css_properties_destroy(img_css);
}

void append_figure_caption(LaTeXConverter* converter, const HTMLNode* table_node) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in append_figure_caption().");
        return;
    }

    if (!table_node) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "HTML node is NULL in append_figure_caption().");
        return;
    }

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

    /* use the existing wrapper functions for consistency */
    append_string(converter, "\\caption{");

    if (html2tex_has_error()) {
        if (caption_text)
            free(caption_text);
        return;
    }

    if (caption_text) {
        escape_latex(converter, caption_text);
        free(caption_text);

        if (html2tex_has_error())
            return;
    }
    else {
        append_string(converter, "Figure ");
        if (html2tex_has_error()) return;

        char counter_str[32];
        snprintf(counter_str, sizeof(counter_str), "%d", figure_counter);
        append_string(converter, counter_str);

        if (html2tex_has_error())
            return;
    }

    append_string(converter, "}\n\\label{fig:");
    if (html2tex_has_error()) return;

    escape_latex_special(converter, figure_label);
    if (html2tex_has_error()) return;
    append_string(converter, "}\n");
}

void html2tex_convert_document(LaTeXConverter* converter, const HTMLNode* node) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in convert_document().");
        return;
    }

    if (!node) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "HTML root node is "
            "NULL in convert_document().");
        return;
    }

    /* track HTML nodes, their CSS inline styles, and element state */
    Stack* node_stack = NULL;
    Stack* css_stack = NULL;
    Stack* processed_stack = NULL;
    CSSProperties* inherit_props = NULL;

    /* push root node with initial CSS properties */
    if (!stack_push(&node_stack, (void*)node) ||
        !stack_push(&css_stack, (void*)inherit_props) ||
        !stack_push(&processed_stack, (void*)0))
        goto cleanup;

    /* depth-first traversal */
    while (!stack_is_empty(node_stack)) {
        intptr_t already_processed = (intptr_t)stack_pop(&processed_stack);
        CSSProperties* current_css = (CSSProperties*)stack_pop(&css_stack);
        HTMLNode* current_node = (HTMLNode*)stack_pop(&node_stack);
        
        if (!current_node) {
            if (current_css && current_css != inherit_props)
                css_properties_destroy(current_css);
            continue;
        }

        /* skip excluded elements */
        if (should_exclude_tag(current_node->tag))
            continue;

        /* skip nested tables */
        if (should_skip_nested_table(current_node) > 0)
            continue;

        /* merge CSS properties */
        CSSProperties* merged_css = current_css;
        CSSProperties* inline_css = NULL;

        if (current_node->tag) {
            const char* style_attr = get_attribute(current_node->attributes, "style");

            if (style_attr) {
                inline_css = parse_css_style(style_attr);

                if (inline_css) {
                    merged_css = css_properties_merge(current_css, inline_css);
                    css_properties_destroy(inline_css);

                    if (html2tex_has_error()) {
                        if (merged_css && merged_css != current_css)
                            css_properties_destroy(merged_css);

                        if (current_css && current_css != inherit_props)
                            css_properties_destroy(current_css);
                        goto cleanup;
                    }
                }
            }
        }

        if (!already_processed) {
            /* handle text nodes */
            if (!current_node->tag && current_node->content) {
                if (current_node->parent && strcmp(current_node->parent->tag, "caption") != 0) {
                    escape_latex(converter, current_node->content);

                    /* CSS cleanup for text node context */
                    if (merged_css && current_node->parent && current_node->parent->tag)
                        css_properties_end(converter, merged_css, current_node->parent->tag);

                    /* cleanup CSS and continue */
                    if (merged_css && merged_css != current_css && merged_css != inherit_props)
                        css_properties_destroy(merged_css);

                    if (current_css && current_css != inherit_props)
                        css_properties_destroy(current_css);
                }

                continue;
            }
            else if (is_supported_element(current_node))
                convert_element(converter, current_node, merged_css, true);

            /* push node again for second pass (closing) */
            if (current_node->children || (current_node->tag && !is_void_element(current_node->tag))) {
                if (!stack_push(&node_stack, (void*)current_node) ||
                    !stack_push(&css_stack, (void*)merged_css) ||
                    !stack_push(&processed_stack, (void*)1))
                    goto cleanup;

                /* merged_css will be cleaned up in second pass */
                merged_css = NULL;
            }
        }
        else {
            /* close the element in second pass */
            if (is_supported_element(current_node))
                convert_element(converter, current_node, merged_css, false);
        }

        /* push children in reverse order (first pass only) */
        if (!already_processed && current_node->children) {
            HTMLNode* last_child = current_node->children;

            while (last_child->next)
                last_child = last_child->next;
            HTMLNode* child = last_child;

            while (child) {
                CSSProperties* child_css = merged_css ? merged_css : current_css;

                if (child_css && child_css != inherit_props) {
                    child_css = css_properties_copy(child_css);
                    if (!child_css || html2tex_has_error()) {
                        if (merged_css && merged_css != current_css && merged_css != inherit_props)
                            css_properties_destroy(merged_css);

                        if (current_css && current_css != inherit_props)
                            css_properties_destroy(current_css);
                        goto cleanup;
                    }
                }

                /* push child onto stack (first pass) */
                if (!stack_push(&node_stack, (void*)child) ||
                    !stack_push(&css_stack, (void*)child_css) ||
                    !stack_push(&processed_stack, (void*)0)) {
                    if (child_css && child_css != inherit_props)
                        css_properties_destroy(child_css);
                    goto cleanup;
                }

                /* move to previous sibling */
                if (child == current_node->children)
                    child = NULL;
                else {
                    HTMLNode* prev = current_node->children;
                    while (prev->next != child) prev = prev->next;
                    child = prev;
                }
            }
        }

        /* clean up current node's CSS (second pass or no children case) */
        if (merged_css && merged_css != current_css
            && merged_css != inherit_props)
            css_properties_destroy(merged_css);

        if (current_css && current_css != inherit_props 
            && (already_processed || !current_node->children))
            css_properties_destroy(current_css);
    }

cleanup:
    /* destroy the remaining CSS properties */
    while (!stack_is_empty(css_stack)) {
        CSSProperties* css = (CSSProperties*)stack_pop(&css_stack);
        if (css && css != inherit_props)
            css_properties_destroy(css);
    }

    /* clean up the remaining stacks */
    stack_cleanup(&node_stack);
    stack_cleanup(&css_stack);
    stack_cleanup(&processed_stack);
}