#include "html2tex.h"
#include <stdbool.h>
#include <stdio.h>

#ifndef MAX_ALLOWED_TAG_LENGTH
#define MAX_ALLOWED_TAG_LENGTH 7
#endif

static inline bool is_valid_element(const HTMLNode* node) {
    return (node && node->tag
        && node->tag[0] != '\0');
}

int is_supported_element(const HTMLNode* node) {
    static const TagProperties supported_tags[] = {
        {"p", 'p', 1}, {"b", 'b', 1}, {"i", 'i', 1},
        {"u", 'u', 1}, {"a", 'a', 1}, { "h1", 'h', 1 }, 
        {"h2", 'h', 1}, {"h3", 'h', 1}, {"h4", 'h', 1}, 
        {"h5", 'h', 1}, {"em", 'e', 2}, {"ul", 'u', 2},
        {"li", 'l', 2}, {"ol", 'o', 2}, {"br", 'b', 2},
        {"hr", 'h', 2}, {"th", 't', 2}, {"td", 't', 2},
        {"tr", 't', 2}, {"div", 'd', 3}, {"img", 'i', 3},
        {"code", 'c', 4}, {"font", 'f', 4}, {"span", 's', 4},
        {"table", 't', 5}, {"tbody", 't', 5}, {"tfoot", 't', 5},
        {"thead", 't', 5}, {"strong", 's', 6}, 
        {"caption", 'c', 7}, {NULL, 0, 0}
    };

    return html2tex_tag_lookup(node->tag,
        supported_tags, MAX_ALLOWED_TAG_LENGTH);
}

static int convert_essential_inline(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_essential_inline(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_essential_block(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_essential_block(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_paragraph(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_paragraph(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_div(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_div(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_heading(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_heading(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_inline_bold(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_inline_bold(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_inline_italic(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_inline_italic(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_inline_underline(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_inline_underline(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_inline_anchor(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_inline_anchor(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_inline_essential(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_inline_essential(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_inline_font(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_inline_font(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_inline_span(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_inline_span(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_inline_image(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int convert_unordered_list(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_unordered_list(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_ordered_list(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_ordered_list(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_item_list(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_item_list(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_table(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_table(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_caption(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_caption(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_table_header(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_table_header(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

static int convert_table_cell(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);
static int finish_table_cell(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props);

int convert_element(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props, bool is_starting) {
    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "NULL converter in convert_element().");
        return -1;
    }

    if (!is_valid_element(node)) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_MALFORMED,
            "HTML element tag is NULL or empty.");
        return -1;
    }

    /* error was already setted */
    if (!is_supported_element(node))
        return -1;

    if (is_starting) {
        if (is_block_element(node->tag))
            return convert_essential_block(converter, node, props);
        return convert_essential_inline(converter, node, props);
    }
    
    if (is_block_element(node->tag))
        return finish_essential_block(converter, node, props);
    return finish_essential_inline(converter, node, props);
}

int convert_paragraph(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    /* definitely, not a paragraph */
    if (node->tag[0] != 'p')
        return 0;

    /* apply CSS and open element */
    if (props && node->tag)
        css_properties_apply(converter, props, node->tag);

    /* is a block element, so write only newline */
    append_string(converter, "\n");
    return 1;
}

int finish_paragraph(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    /* definitely, not a paragraph */
    if (node->tag[0] != 'p')
        return 0;

    /* is a block element, so write only newline */
    append_string(converter, "\n\n");

    /* close CSS properties for this element */
    if (props && node->tag)
        css_properties_end(converter, props, node->tag);
    return 1;
}

int convert_div(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if(!node->tag || (node->tag && 
        strcmp(node->tag, "div") != 0))
        return 0;

    /* apply CSS and open element */
    if (props && node->tag)
        css_properties_apply(converter, 
            props, node->tag);
    return 1;
}

int finish_div(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (!node->tag || (node->tag &&
        strcmp(node->tag, "div") != 0))
        return 0;

    /* close CSS properties for this element */
    if (props && node->tag)
        css_properties_end(converter, 
            props, node->tag);
    return 1;
}

/* @brief Converts the essential HTML inline elements into corresponding LaTeX code. */
int convert_essential_block(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    static const struct {
        const char* key;
        int (*process_block)(LaTeXConverter*, const HTMLNode*, const CSSProperties*);
    } blocks[] = {
        {"p", &convert_paragraph}, {"div", &convert_div},
        {"h1", &convert_heading}, {"h2", &convert_heading},
        {"h3", &convert_heading}, {"h4", &convert_heading},
        {"h5", &convert_heading}, {"ul", &convert_unordered_list},
        {"ol", &convert_ordered_list}, {"li", &convert_item_list},
        {"table", &convert_table}, {"caption", &convert_caption},
        {"thead", NULL}, {"tbody", NULL}, {"tfoot", NULL},
        {"tr", &convert_table_header}, {"td", &convert_table_cell},
        {"th", &convert_table_cell}, {NULL, NULL}
    };

    int i = 0;

    for (i = 0; blocks[i].key != NULL; i++) {
        if (strcmp(node->tag, blocks[i].key) == 0) {
            if (blocks[i].process_block != NULL)
                return blocks[i].process_block(converter, node, props);
        }
    }

    /* no supported block element found */
    return 0;
}

/* @brief Ends the conversion of essential HTML inline elements into corresponding LaTeX code. */
int finish_essential_block(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    static const struct {
        const char* key;
        int (*finish_block)(LaTeXConverter*, const HTMLNode*, const CSSProperties*);
    } blocks[] = {
        {"p", &finish_paragraph}, {"div", &finish_div},
        {"h1", &finish_heading}, {"h2", &finish_heading},
        {"h3", &finish_heading}, {"h4", &finish_heading},
        {"h5", &finish_heading}, {"ul", &finish_unordered_list}, 
        {"ol", &finish_ordered_list}, {"li", &finish_item_list},
        {"table", &finish_table}, {"caption", &finish_caption},
        {"thead", NULL}, {"tbody", NULL}, {"tfoot", NULL},
        {"tr", &finish_table_header}, {"td", &finish_table_cell},
        {"th", &finish_table_cell}, {NULL, NULL}
    };

    int i = 0;

    for (i = 0; blocks[i].key != NULL; i++) {
        if (strcmp(node->tag, blocks[i].key) == 0) {
            if (blocks[i].finish_block != NULL)
                return blocks[i].finish_block(converter, node, props);
        }
    }

    /* no supported block element found */
    return 0;
}

int convert_heading(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (node->tag[0] != 'h')
        return -1;

    int level = node->tag[1] - 48;
    static const char* const heading_type[] = {
        "\\chapter{", "\\section{", "\\subsection{",
        "\\subsubsection{", "\\paragraph{"
    };

    /* apply CSS and open element */
    if (props && node->tag)
        css_properties_apply(converter, props, node->tag);

    if (level >= 1 && level <= 5) {
        append_string(converter, heading_type[level - 1]);
        return 1;
    }

    /* no supported heading */
    return 0;
}

int convert_unordered_list(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "ul") != 0)
        return 0;

    /* apply CSS and open element */
    if (props && node->tag)
        css_properties_apply(converter, props, node->tag);

    append_string(converter, "\\begin{itemize}\n");
    return 1;
}

int convert_ordered_list(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "ol") != 0)
        return 0;

    /* apply CSS and open element */
    if (props && node->tag)
        css_properties_apply(converter, props, node->tag);

    append_string(converter, "\\begin{enumerate}\n");
    return 1;
}

int convert_item_list(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "li") != 0)
        return 0;

    /* apply CSS and open element */
    if (props && node->tag)
        css_properties_apply(converter, props, node->tag);

    append_string(converter, "\\item ");
    return 1;
}

int convert_table(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "table") != 0)
        return 0;

    if (table_contains_only_images(node))
        convert_image_table(converter, node);
    else {
        /* reset CSS for table element */
        converter->state.applied_props = 0;
        int columns = count_table_columns(node);
        begin_table(converter, columns);
    }

    return 1;
}

int finish_table(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "table") != 0)
        return 0;

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
    return 1;
}

int convert_caption(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "caption") != 0)
        return 0;

    if (converter->state.in_table) {
        if (converter->state.table_caption) {
            free(converter->state.table_caption);
            converter->state.table_caption = NULL;
        }

        char* raw_caption = extract_caption_text(node);
        const char* style_attr = get_attribute(node->attributes, "style");

        if (raw_caption) {
            CSSProperties* caption_css = NULL;
            if (style_attr) caption_css = parse_css_style(style_attr);

            if (caption_css) {
                /* calculate maximum required buffer size */
                const char* color = css_properties_get(caption_css, "color");
                const char* font_weight = css_properties_get(caption_css, "font-weight");

                size_t color_prefix_len = 0;
                char* hex_color = NULL;

                if (color) {
                    hex_color = css_color_to_hex(color);
                    if (hex_color && strcmp(hex_color, "000000") != 0)
                        color_prefix_len = 19 + 6 + 2;
                }

                int has_bold = (font_weight &&
                    (strcmp(font_weight, "bold") == 0 ||
                        strcmp(font_weight, "bolder") == 0));

                size_t bold_len = has_bold ? 8 : 0;
                size_t raw_len = strlen(raw_caption);
                size_t closing_len = (color_prefix_len ? 1 : 0) + (has_bold ? 1 : 0);

                /* allocate exact size needed */
                size_t buffer_size = color_prefix_len + bold_len + raw_len + closing_len + 1;
                char* formatted_caption = (char*)malloc(buffer_size);

                if (formatted_caption) {
                    char* ptr = formatted_caption;
                    size_t remaining = buffer_size;

                    /* build formatted caption safely */
                    if (color_prefix_len && hex_color) {
                        int written = snprintf(ptr, remaining,
                            "\\textcolor[HTML]{%s}{", hex_color);
                        if (written > 0 && (size_t)written < remaining) {
                            ptr += written;
                            remaining -= written;
                        }
                    }

                    if (has_bold) {
                        int written = snprintf(ptr, remaining, "\\textbf{");
                        if (written > 0 && (size_t)written < remaining) {
                            ptr += written;
                            remaining -= written;
                        }
                    }

                    /* copy raw caption */
                    size_t copy_len = raw_len < remaining 
                        ? raw_len : remaining - 1;

                    memcpy(ptr, raw_caption, copy_len);
                    ptr += copy_len;
                    remaining -= copy_len;

                    /* close formatting in reverse order */
                    if (has_bold) {
                        if (remaining > 1) {
                            *ptr++ = '}';
                            remaining--;
                        }
                    }

                    if (color_prefix_len && hex_color) {
                        if (remaining > 1) {
                            *ptr++ = '}';
                            remaining--;
                        }
                    }

                    *ptr = '\0';
                    converter->state.table_caption = formatted_caption;
                }

                if (hex_color) free(hex_color);
                css_properties_destroy(caption_css);
            }
            else
                converter->state.table_caption = raw_caption;
        }
    }

    return 1;
}

int finish_caption(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    return strcmp(node->tag, "caption") != 0
        ? 0 : 1;
}

int convert_table_header(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "tr") != 0)
        return 0;

    converter->state.current_column = 0;
    converter->state.applied_props = 0;
    begin_table_row(converter);
    return 1;
}

int finish_table_header(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "tr") != 0)
        return 0;

    end_table_row(converter);
    return 1;
}

int convert_table_cell(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "td") != 0
        && strcmp(node->tag, "th") != 0)
        return 0;

    int is_header = (strcmp(node->tag, "th") == 0);
    int colspan = 1;
    
    if (node->attributes != NULL) {
        const char* colspan_attr = get_attribute(node->attributes, 
            "colspan");

        if (colspan_attr) {
            colspan = atoi(colspan_attr);
            if (colspan < 1) colspan = 1;
        }
    }

    if (converter->state.current_column > 0) 
        append_string(converter, " & ");

    /* save current state for cell-internal CSS */
    CSSPropertyMask saved_props = converter->state.applied_props;
    converter->state.applied_props = 0;

    if (is_header)
        append_string(converter, "\\textbf{");

    converter->state.in_table_cell = 1;
    return 1;
}

int finish_table_cell(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "td") != 0
        && strcmp(node->tag, "th") != 0)
        return 0;

    int is_header = (strcmp(node->tag, "th") == 0);
    int colspan = 1;

    if (node->attributes != NULL) {
        const char* colspan_attr = get_attribute(node->attributes, 
            "colspan");

        if (colspan_attr) {
            colspan = atoi(colspan_attr);
            if (colspan < 1) colspan = 1;
        }
    }

    /* close header bold formatting */
    if (is_header) append_string(converter, "}");

    /* close any CSS braces opened within this cell */
    while (converter->state.css_braces > 0) {
        append_string(converter, "}");
        converter->state.css_braces--;
    }

    /* reset CSS properties */
    converter->state.applied_props = 0;
    converter->state.in_table_cell = 0;

    /* handle colspan */
    for (int i = 1; i < colspan; i++) {
        converter->state.current_column++;
        append_string(converter, " & ");
        append_string(converter, " ");
    }

    converter->state.current_column++;
    return 1;
}

int finish_heading(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (node->tag[0] != 'h')
        return -1;

    int level = node->tag[1] - 48;

    if (level >= 1 && level <= 5) {
        append_string(converter, "}\n\n");

        /* close CSS properties for this element */
        if (props && node->tag)
            css_properties_end(converter, props, node->tag);
        return 1;
    }

    /* no supported heading */
    return 0;
}

int finish_unordered_list(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "ul") != 0)
        return 0;

    append_string(converter, "\\end{itemize}\n");

    /* close CSS properties for this element */
    if (props && node->tag)
        css_properties_end(converter, props, node->tag);
    return 1;
}

int finish_ordered_list(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "ol") != 0)
        return 0;

    append_string(converter, "\\end{enumerate}\n");

    /* close CSS properties for this element */
    if (props && node->tag)
        css_properties_end(converter, props, node->tag);
    return 1;
}

int finish_item_list(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "li") != 0)
        return 0;

    append_string(converter, "\n");

    /* close CSS properties for this element */
    if (props && node->tag)
        css_properties_end(converter, props, node->tag);
    return 1;
}

/* @brief Converts the essential HTML inline elements into corresponding LaTeX code. */
int convert_essential_inline(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    static const struct {
        const char* key;
        int (*convert_inline_element)(LaTeXConverter*, const HTMLNode*, const CSSProperties*);
    } entries[] = {
        {"b", &convert_inline_bold}, {"strong", &convert_inline_bold}, {"i", &convert_inline_italic}, 
        {"em", &convert_inline_italic}, {"u", &convert_inline_underline}, {"hr", &convert_inline_essential}, 
        {"code", &convert_inline_essential}, {"font", &convert_inline_font}, {"span", &convert_inline_span}, {"a", &convert_inline_anchor},
        {"br", &convert_inline_essential}, {"img", &convert_inline_image}, { NULL, NULL }
    };

    int i = 0;
    bool found = false;

    /* find if the specified node is special inline element */
    for (i = 0; entries[i].key != NULL; i++) {
        if (strcmp(node->tag, entries[i].key) == 0) {
            found = true;
            break;
        }
    }

    if (found) {
        if (entries[i].convert_inline_element != NULL)
            return entries[i].convert_inline_element(
                converter, node, props);
    }

    /* not found */
    return 0;
}

/* @brief Ends the conversion of essential HTML inline elements into corresponding LaTeX code. */
int finish_essential_inline(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    static const struct {
        const char* key;
        int (*finish_inline_element)(LaTeXConverter*, const HTMLNode*, const CSSProperties*);
    } entries[] = {
        {"b", &finish_inline_bold}, {"strong", &finish_inline_bold}, {"i", &finish_inline_italic}, 
        {"em", &finish_inline_italic}, {"u", &finish_inline_underline}, {"hr", &finish_inline_essential}, 
        {"code", &finish_inline_essential}, {"font", &finish_inline_font}, {"span", &finish_inline_span}, {"a", &finish_inline_anchor},
        {"br", &finish_inline_essential}, {"img", NULL}, { NULL, NULL }
    };

    int i = 0;
    bool found = false;

    /* find if the specified node is special inline element */
    for (i = 0; entries[i].key != NULL; i++) {
        if (strcmp(node->tag, entries[i].key) == 0) {
            found = true;
            break;
        }
    }

    if (found) {
        if (entries[i].finish_inline_element != NULL)
            return entries[i].finish_inline_element(
                converter, node, props);
    }

    /* not found */
    return 0;
}

int convert_inline_bold(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    /* apply CSS properties for this element */
    if (props && node->tag)
        css_properties_apply(converter, props, node->tag);

    if (!(converter->state.applied_props & CSS_BOLD)) {
        append_string(converter, "\\textbf{");
        converter->state.applied_props |= CSS_BOLD;
        return 1;
    }

    /* not found */
    return 0;
}

int finish_inline_bold(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    append_string(converter, "}");

    /* close CSS properties for this element */
    if (props && node->tag)
        css_properties_end(converter, props, node->tag);
    return 1;
}

int convert_inline_italic(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    /* apply CSS properties for this element */
    if (props && node->tag)
        css_properties_apply(converter, props, node->tag);

    if (!(converter->state.applied_props & CSS_ITALIC)) {
        append_string(converter, "\\textit{");
        converter->state.applied_props |= CSS_ITALIC;
        return 1;
    }

    return 0;
}

int finish_inline_italic(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    append_string(converter, "}");

    /* close CSS properties for this element */
    if (props && node->tag)
        css_properties_end(converter, props, node->tag);
    return 1;
}

int convert_inline_underline(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    /* apply CSS properties for this element */
    if (props && node->tag)
        css_properties_apply(converter, props, node->tag);

    if (!(converter->state.applied_props & CSS_UNDERLINE)) {
        append_string(converter, "\\underline{");
        converter->state.applied_props |= CSS_UNDERLINE;
        return 1;
    }

    return 0;
}

int finish_inline_underline(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    append_string(converter, "}");

    /* close CSS properties for this element */
    if (props && node->tag)
        css_properties_end(converter, props, node->tag);
    return 1;
}

int convert_inline_anchor(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    const char* href = get_attribute(node->attributes, "href");

    /* apply CSS properties for this element */
    if (props && node->tag)
        css_properties_apply(converter, props, node->tag);

    if (href) {
        append_string(converter, "\\href{");
        escape_latex(converter, href);
        append_string(converter, "}{");
        return 1;
    }

    return 0;
}

int finish_inline_anchor(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    const char* href = get_attribute(node->attributes, "href");

    if (href) {
        append_string(converter, "}");

        /* close CSS properties for this element */
        if (props && node->tag)
            css_properties_end(converter, props, node->tag);
        return 1;
    }

    return 0;
}

int convert_inline_essential(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    /* apply CSS properties for this element */
    if (props && node->tag)
        css_properties_apply(converter, 
            props, node->tag);

    if (!node->tag)
        return 0;

    if (strcmp(node->tag, "hr") == 0) {
        append_string(converter, "\\hrulefill\n\n");
        return 2;
    }
    else if (strcmp(node->tag, "br") == 0) {
        append_string(converter, "\\\\\n");
        return 2;
    }
    else if (strcmp(node->tag, "code") == 0) {
        append_string(converter, "\\texttt{");
        return 1;
    }

    return 0;
}

int finish_inline_essential(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    /* close CSS properties for this element */
    if (props && node->tag)
        css_properties_end(converter, 
            props, node->tag);

    if (!node->tag)
        return 0;

    if (strcmp(node->tag, "br") == 0 || strcmp(node->tag, "hr") == 0)
        return 2;
    else if (strcmp(node->tag, "code") == 0) {
        append_string(converter, "}");
        return 1;
    }

    return 0;
}

int convert_inline_font(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if(!node->tag || (node->tag && 
        strcmp(node->tag, "font") != 0))
        return 0;
    
    /* apply CSS properties for this element */
    if (props && node->tag)
        css_properties_apply(converter,
            props, node->tag);

    const char* color_attr = get_attribute(node->attributes, "color");
    const char* text_color = css_properties_get(props, "color");

    if (props && !text_color) {
        if (color_attr && !(props->mask & CSS_COLOR))
            apply_color(converter, color_attr, 0);
    }

    return 1;
}

int finish_inline_font(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (!node->tag || (node->tag &&
        strcmp(node->tag, "font") != 0))
        return 0;
    
    /* close CSS properties for this element */
    if (props && node->tag)
        css_properties_end(converter,
            props, node->tag);

    const char* color_attr = get_attribute(node->attributes, "color");
    const char* text_color = css_properties_get(props, "color");

    if (props && !text_color)
        append_string(converter, "}");

    return 1;
}

int convert_inline_span(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (!node->tag || (node->tag &&
        strcmp(node->tag, "span") != 0))
        return 0;

    /* apply CSS properties for the element content */
    if (props && node->tag)
        css_properties_apply(converter,
            props, node->tag);

    return 1;
}

int finish_inline_span(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (!node->tag || (node->tag &&
        strcmp(node->tag, "span") != 0))
        return 0;

    /* ends the CSS properties */
    if (props && node->tag)
        css_properties_end(converter,
            props, node->tag);

    return 1;
}

int convert_inline_image(LaTeXConverter* converter, const HTMLNode* node, const CSSProperties* props) {
    if (strcmp(node->tag, "img") != 0)
        return 0;
    if (is_inside_table(node)) {
        const char* src = get_attribute(node->attributes, "src");
        const char* style_attr = get_attribute(node->attributes, "style");
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
                strstr(image_path, converter->image_output_dir) == image_path)
                escape_latex_special(converter, image_path + 2);
            else
                escape_latex(converter, image_path);
            append_string(converter, "}");
            free(image_path);
        }
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

            if (converter->download_images
                && converter->image_output_dir)
                image_path = download_image_src(src,
                    converter->image_output_dir,
                    converter->image_counter);

            if (!image_path) {
                if (is_base64_image(src) && converter->download_images && converter->image_output_dir)
                    image_path = download_image_src(src, converter->image_output_dir,
                        converter->image_counter);

                if (!image_path) image_path = strdup(src);
            }

            append_string(converter, "\n\n\\begin{figure}[h]\n");
            append_string(converter, "\\centering\n");

            int width_pt = css_length_to_pt(width_attr);
            int height_pt = css_length_to_pt(height_attr);

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
                char text_caption[64] = { 0 };

                char caption_counter[32] = { 0 };
                html2tex_itoa(converter->state.image_internal_counter,
                    caption_counter, 10);

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

                html2tex_itoa(converter->state.image_internal_counter,
                    label_counter, 10);

                strcpy(image_label_id, "image_");
                strcpy(image_label_id + 6, label_counter);
                escape_latex_special(converter, image_label_id);
                append_string(converter, "}\n");
            }

            append_string(converter, "\\end{figure}\n");
            append_string(converter, "\\FloatBarrier\n\n");
        }
    }

    return 2;
}