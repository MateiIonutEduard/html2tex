#include "html2tex.h"
#include <stdbool.h>
#include <stdio.h>

static inline bool is_valid_element(const HTMLNode* node) {
    return (node && node->tag
        && node->tag[0] != '\0');
}

int is_supported_element(const HTMLNode* node) {
    if (!is_valid_element(node)) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_UNSUPPORTED,
            "HTML element is unsupported.");
        return 0;
    }

    static const char* const support[] = {
        "p", "div", "h1", "h2", "h3", "h4", "h5",
        "b", "strong", "i", "em", "u", "code",
        "font", "span", "a", "ul", "li", "ol",
        "br", "hr", "img", "table", "th", "td",
        "caption", "thead", "tbody", "tfoot", 
        "tr", NULL
    };

    for (int i = 0; support[i]; i++) {
        if (strcmp(node->tag, support[i]) == 0)
            return 1;
    }

    return 0;
}

static int convert_essential_inline(LaTeXConverter* converter, const HTMLNode* node);
static int finish_essential_inline(LaTeXConverter* converter, const HTMLNode* node);

static int convert_essential_block(LaTeXConverter* converter, const HTMLNode* node);
static int finish_essential_block(LaTeXConverter* converter, const HTMLNode* node);

static int convert_paragraph(LaTeXConverter* converter, const HTMLNode* node);
static int finish_paragraph(LaTeXConverter* converter, const HTMLNode* node);

static int convert_heading(LaTeXConverter* converter, const HTMLNode* node);
static int finish_heading(LaTeXConverter* converter, const HTMLNode* node);

static int convert_inline_bold(LaTeXConverter* converter, const HTMLNode* node);
static int finish_inline_bold(LaTeXConverter* converter, const HTMLNode* node);

static int convert_inline_italic(LaTeXConverter* converter, const HTMLNode* node);
static int finish_inline_italic(LaTeXConverter* converter, const HTMLNode* node);

static int convert_inline_underline(LaTeXConverter* converter, const HTMLNode* node);
static int finish_inline_underline(LaTeXConverter* converter, const HTMLNode* node);

static int convert_inline_anchor(LaTeXConverter* converter, const HTMLNode* node);
static int finish_inline_anchor(LaTeXConverter* converter, const HTMLNode* node);

static int convert_inline_essential(LaTeXConverter* converter, const HTMLNode* node);
static int finish_inline_essential(LaTeXConverter* converter, const HTMLNode* node);

int convert_element(LaTeXConverter* converter, const HTMLNode* node, bool is_starting) {
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
            return convert_essential_block(converter, node);
        return convert_essential_inline(converter, node);
    }
    
    if (is_block_element(node->tag))
        return finish_essential_block(converter, node);
    return finish_essential_inline(converter, node);
}

int convert_paragraph(LaTeXConverter* converter, const HTMLNode* node) {
    /* definitely, not a paragraph */
    if (node->tag[0] != 'p')
        return 0;

    /* is a block element, so write only newline */
    append_string(converter, "\n");
    return 1;
}

int finish_paragraph(LaTeXConverter* converter, const HTMLNode* node) {
    /* definitely, not a paragraph */
    if (node->tag[0] != 'p')
        return 0;

    /* is a block element, so write only newline */
    append_string(converter, "\n\n");
    return 1;
}

/* @brief Converts the essential HTML inline elements into corresponding LaTeX code. */
int convert_essential_block(LaTeXConverter* converter, const HTMLNode* node) {
    static const struct {
        const char* key;
        void (*process_block)(LaTeXConverter*, const HTMLNode*);
    } blocks[] = {
        {"p", &convert_paragraph}, {"div", NULL},
        {"h1", &convert_heading}, {"h2", &convert_heading},
        {"h3", &convert_heading}, {"h4", &convert_heading},
        {"h5", &convert_heading}, {NULL, NULL}
    };

    int i = 0;

    for (i = 0; blocks[i].key != NULL; i++) {
        if (strcmp(node->tag, blocks[i].key) == 0) {
            if (blocks[i].process_block != NULL)
                blocks[i].process_block(converter, node);
            return 1;
        }
    }

    /* no supported block element found */
    return 0;
}

/* @brief Ends the conversion of essential HTML inline elements into corresponding LaTeX code. */
int finish_essential_block(LaTeXConverter* converter, const HTMLNode* node) {
    static const struct {
        const char* key;
        void (*finish_block)(LaTeXConverter*, const HTMLNode*);
    } blocks[] = {
        {"p", &finish_paragraph}, {"div", NULL},
        {"h1", &finish_heading}, {"h2", &finish_heading},
        {"h3", &finish_heading}, {"h4", &finish_heading},
        {"h5", &finish_heading}, {NULL, NULL}
    };

    int i = 0;

    for (i = 0; blocks[i].key != NULL; i++) {
        if (strcmp(node->tag, blocks[i].key) == 0) {
            if (blocks[i].finish_block != NULL)
                blocks[i].finish_block(converter, node);
            return 1;
        }
    }

    /* no supported block element found */
    return 0;
}

int convert_heading(LaTeXConverter* converter, const HTMLNode* node) {
    if (node->tag[0] != 'h')
        return -1;

    int level = node->tag[1] - 48;
    static const char* const heading_type[] = {
        "\\chapter{", "\\section{", "\\subsection{",
        "\\subsubsection{", "\\paragraph{"
    };

    if (level >= 1 && level <= 5) {
        append_string(converter, heading_type[level - 1]);
        return 1;
    }

    /* no supported heading */
    return 0;
}

int finish_heading(LaTeXConverter* converter, const HTMLNode* node) {
    if (node->tag[0] != 'h')
        return -1;

    int level = node->tag[1] - 48;

    if (level >= 1 && level <= 5) {
        append_string(converter, "}\n\n");
        return 1;
    }

    /* no supported heading */
    return 0;
}

/* @brief Converts the essential HTML inline elements into corresponding LaTeX code. */
int convert_essential_inline(LaTeXConverter* converter, const HTMLNode* node) {
    static const struct {
        const char* key;
        int (*convert_inline_element)(LaTeXConverter*, const HTMLNode*);
    } entries[] = {
        {"b", &convert_inline_bold}, {"strong", &convert_inline_bold}, {"i", &convert_inline_italic}, {"em", &convert_inline_italic},
        {"u", &convert_inline_underline}, {"hr", &convert_inline_essential}, {"code", &convert_inline_essential}, {"font", NULL},
        {"span", NULL}, {"a", &convert_inline_anchor}, {"br", &convert_inline_essential}, {NULL, NULL}
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
        if (i >= 7 && i < 9)
            return 1;
        else if (entries[i].convert_inline_element != NULL)
            return entries[i].convert_inline_element(
                converter, node);
    }

    /* not found */
    return 0;
}

/* @brief Ends the conversion of essential HTML inline elements into corresponding LaTeX code. */
int finish_essential_inline(LaTeXConverter* converter, const HTMLNode* node) {
    static const struct {
        const char* key;
        int (*finish_inline_element)(LaTeXConverter*, const HTMLNode*);
    } entries[] = {
        {"b", &finish_inline_bold}, {"strong", &finish_inline_bold}, {"i", &finish_inline_italic}, {"em", &finish_inline_italic},
        {"u", &finish_inline_underline}, {"hr", &finish_inline_essential}, {"code", &finish_inline_essential}, {"font", NULL},
        {"span", NULL}, {"a", &finish_inline_anchor}, {"br", &finish_inline_essential}, {NULL, NULL}
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
        if (i >= 7 && i < 9)
            return 1;
        else if (entries[i].finish_inline_element != NULL)
            return entries[i].finish_inline_element(
                converter, node);
    }

    /* not found */
    return 0;
}

int convert_inline_bold(LaTeXConverter* converter, const HTMLNode* node) {
    if (!(converter->state.applied_props & CSS_BOLD)) {
        append_string(converter, "\\textbf{");
        converter->state.applied_props |= CSS_BOLD;
        return 1;
    }

    /* not found */
    return 0;
}

int finish_inline_bold(LaTeXConverter* converter, const HTMLNode* node) {
    append_string(converter, "}");
    return 1;
}

int convert_inline_italic(LaTeXConverter* converter, const HTMLNode* node) {
    if (!(converter->state.applied_props & CSS_ITALIC)) {
        append_string(converter, "\\textit{");
        converter->state.applied_props |= CSS_ITALIC;
        return 1;
    }

    return 0;
}

int finish_inline_italic(LaTeXConverter* converter, const HTMLNode* node) {
    append_string(converter, "}");
    return 1;
}

int convert_inline_underline(LaTeXConverter* converter, const HTMLNode* node) {
    if (!(converter->state.applied_props & CSS_UNDERLINE)) {
        append_string(converter, "\\underline{");
        converter->state.applied_props |= CSS_UNDERLINE;
        return 1;
    }

    return 0;
}

int finish_inline_underline(LaTeXConverter* converter, const HTMLNode* node) {
    append_string(converter, "}");
    return 1;
}

int convert_inline_anchor(LaTeXConverter* converter, const HTMLNode* node) {
    const char* href = get_attribute(node->attributes, "href");

    if (href) {
        append_string(converter, "\\href{");
        escape_latex(converter, href);
        append_string(converter, "}{");
        return 1;
    }

    return 0;
}

int finish_inline_anchor(LaTeXConverter* converter, const HTMLNode* node) {
    const char* href = get_attribute(node->attributes, "href");

    if (href) {
        append_string(converter, "}");
        return 1;
    }

    return 0;
}

int convert_inline_essential(LaTeXConverter* converter, const HTMLNode* node) {
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

int finish_inline_essential(LaTeXConverter* converter, const HTMLNode* node) {
    if (strcmp(node->tag, "br") == 0 || strcmp(node->tag, "hr") == 0)
        return 2;
    else if (strcmp(node->tag, "code") == 0) {
        append_string(converter, "}");
        return 1;
    }

    return 0;
}