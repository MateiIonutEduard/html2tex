#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef MAX_INLINE_TAG_LENGTH
#define MAX_INLINE_TAG_LENGTH 6
#endif

/* This helper function to check if element is inline, required for formatting. */
static int is_inline_element_for_formatting(const char* tag_name) {
    static const TagProperties inline_tags[] = {
        {"a", 'a', 1}, {"abbr", 'a', 4}, {"b", 'b', 1}, 
        {"bdi", 'b', 3}, {"bdo", 'b', 3}, {"br", 'b', 2},
        {"cite", 'c', 4}, {"code", 'c', 4}, {"data", 'd', 4}, 
        {"dfn", 'd', 3}, {"em", 'e', 2}, {"font", 'f', 4},
        {"i", 'i', 1}, {"kbd", 'k', 3}, {"mark", 'm', 4}, 
        {"q", 'q', 1}, {"rp", 'r', 2}, {"rt", 'r', 2},
        {"ruby", 'r', 4}, {"samp", 's', 4}, {"small", 's', 5}, 
        {"span", 's', 4}, {"strong", 's', 6}, {"sub", 's', 3}, 
        {"sup", 's', 3}, {"time", 't', 4}, {"u", 'u', 1}, 
        {"var", 'v', 3}, {"wbr", 'w', 3}, {NULL, 0, 0}
    };

    return html2tex_tag_lookup(tag_name, 
        inline_tags, MAX_INLINE_TAG_LENGTH);
}

/* This helper function is used to escape HTML special characters. */
static char* escape_html(const char* text) {
    /* clear any previous error state */
    html2tex_err_clear();

    if (!text) return NULL;

    const char* p = text;
    size_t extra = 0;

    /* count special chars efficiently */
    while (*p) {
        unsigned char c = (unsigned char)*p;
        if (c == '<' || c == '>') extra += 3;

        else if (c == '&') {
            /* check if already escaped */
            if (strncmp(p, "&lt;", 4) && strncmp(p, "&gt;", 4) &&
                strncmp(p, "&amp;", 5) && strncmp(p, "&quot;", 6) &&
                strncmp(p, "&apos;", 6) && strncmp(p, "&#", 2))
                extra += 4;
        }

        else if (c == '"') extra += 5;
        else if (c == '\'') extra += 5;
        p++;
    }

    /* no escaping needed, return copy */
    if (extra == 0) {
        char* result = strdup(text);
        if (!result) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                "Failed to duplicate plain text"
                " for HTML escaping.");
        }
        return result;
    }

    /* allocate once */
    size_t len = p - text;
    char* escaped = (char*)malloc(len + extra + 1);

    if (!escaped) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate escaped HTML"
            " text buffer.");
        return NULL;
    }

    /* copy with escaping */
    char* dest = escaped;
    p = text;

    while (*p) {
        unsigned char c = (unsigned char)*p;

        if (c == '<') {
            memcpy(dest, "&lt;", 4);
            dest += 4;
        }
        else if (c == '>') {
            memcpy(dest, "&gt;", 4);
            dest += 4;
        }
        else if (c == '&') {
            /* check if already escaped */
            if (strncmp(p, "&lt;", 4) && strncmp(p, "&gt;", 4) &&
                strncmp(p, "&amp;", 5) && strncmp(p, "&quot;", 6) &&
                strncmp(p, "&apos;", 6) && strncmp(p, "&#", 2)) {
                memcpy(dest, "&amp;", 5);
                dest += 5;
            }
            else
                *dest++ = *p;
        }
        else if (c == '"') {
            memcpy(dest, "&quot;", 6);
            dest += 6;
        }
        else if (c == '\'') {
            memcpy(dest, "&apos;", 6);
            dest += 6;
        }
        else
            *dest++ = *p;
        p++;
    }

    *dest = '\0';
    return escaped;
}

/* Recursive function to write the prettified HTML code. */
static void write_pretty_node(FILE* file, HTMLNode* node, int indent_level) {
    /* clear any previous error state */
    html2tex_err_clear();

    if (!node || !file) {
        if (!node) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
                "HTML node is NULL for pretty printing.");
        }
        return;
    }

    /* create indentation */
    for (int i = 0; i < indent_level; i++)
        fputs("  ", file);

    if (node->tag) {
        /* element node */
        fputs("<", file);
        fputs(node->tag, file);

        /* write attributes */
        HTMLAttribute* attr = node->attributes;

        while (attr) {
            if (attr->value) {
                char* escaped_value = escape_html(attr->value);

                if (escaped_value) {
                    fputs(" ", file); fputs(attr->key, file); 
                    fputs("=\"", file); fputs(escaped_value, file); 
                    fputs("\"", file); free(escaped_value);
                }
                else if (html2tex_has_error())
                    return;
            }
            else {
                fputs(" ", file);
                fputs(attr->key, file);
            }

            attr = attr->next;
        }

        /* check if this element should be inline */
        int is_inline = is_inline_element_for_formatting(node->tag);

        /* check if this is a self-closing tag or has children */
        if (!node->children && !node->content)
            fputs(" />\n", file);
        else {
            /* write content if present */
            fputs(">", file);

            if (node->content) {
                char* escaped_content = escape_html(node->content);

                if (escaped_content) {
                    fputs(escaped_content, file);
                    free(escaped_content);
                }
                else if (html2tex_has_error())
                    return;
            }

            /* write children with proper indentation */
            if (node->children) {
                if (!is_inline) fputs("\n", file);
                HTMLNode* child = node->children;

                while (child) {
                    write_pretty_node(file, child, indent_level + 1);

                    /* propagate any error from recursive call */
                    if (html2tex_has_error()) return;
                    child = child->next;
                }

                if (!is_inline) {
                    for (int i = 0; i < indent_level; i++)
                        fputs("  ", file);
                }
            }

            fputs("</", file);
            fputs(node->tag, file);
            fputs(">\n", file);
        }
    }
    else {
        /* text node */
        if (node->content) {
            char* escaped_content = escape_html(node->content);

            /* check if this is mostly whitespace */
            if (escaped_content) {
                int all_whitespace = 1;
                char* p = escaped_content;

                while (*p) {
                    if (!isspace((unsigned char)*p)) {
                        all_whitespace = 0;
                        break;
                    }
                    p++;
                }

                if (!all_whitespace) {
                    fputs(escaped_content, file);
                    fputs("\n", file);
                }
                else
                    fputs("\n", file);
                free(escaped_content);
            }
            else if (html2tex_has_error())
                return;
        }
    }
}

int write_pretty_html(const HTMLNode* root, const char* filename) {
    /* clear any previous error state */
    html2tex_err_clear();

    if (!root) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Root node is NULL for HTML file writing.");
        return 0;
    }

    if (!filename) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Filename is NULL for HTML file writing.");
        return 0;
    }

    FILE* file = fopen(filename, "w");
    if (!file) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IO,
            "Failed to open file '%s' for writing.", filename);
        return 0;
    }

    /* write HTML header */
    fputs("<html>\n<head>\n", file);
    fputs("  <meta charset=\"UTF-8\">\n", file);
    char* html_title = html2tex_extract_title(root);

    if (html2tex_has_error()) {
        fclose(file);
        return 0;
    }

    fputs("  <title>", file);
    if (!html_title) 
        fputs("Parsed HTML Output", file);
    else {
        fputs(html_title, file);
        free(html_title);
    }

    fputs("</title>\n", file);
    fputs("</head>\n<body>\n", file);

    /* write the parsed content */
    HTMLNode* child = root->children;

    while (child) {
        write_pretty_node(file, child, 1);

        /* check for errors from write_pretty_node function */
        if (html2tex_has_error()) {
            fclose(file);
            return 0;
        }

        child = child->next;
    }

    /* write HTML footer and close the stream */
    fputs("</body>\n</html>\n", file);

    if (fclose(file) != 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IO,
            "Failed to close file '%s' "
            "after writing.", filename);
        return 0;
    }

    return 1;
}

char* get_pretty_html(const HTMLNode* root) {
    if (!root) return NULL;

#ifndef _WIN32
    /* use open_memstream for fastest, no disk I/O */
    char* buffer = NULL;

    size_t size = 0;
    FILE* stream = open_memstream(&buffer, &size);

    if (stream) {
        /* write HTML to memory stream */
        fputs("<html>\n<head>\n", stream);
        fputs("  <meta charset=\"UTF-8\">\n", stream);
        char* html_title = html2tex_extract_title(root);
        fputs("  <title>", stream);

        if (!html_title)
            fputs("Parsed HTML Output", stream);
        else {
            fputs(html_title, stream);
            free(html_title);
        }

        fputs("</title>\n", stream);
        fputs("</head>\n<body>\n", stream);

        /* write content using our buffered function */
        HTMLNode* child = root->children;

        while (child) {
            write_pretty_node(stream, child, 1);
            child = child->next;
        }

        fputs("</body>\n</html>\n", stream);
        fclose(stream);
        return buffer;
    }
#endif
    FILE* temp_file = tmpfile();
    if (!temp_file) return NULL;

    /* write to temp file */
    fputs("<html>\n<head>\n", temp_file);
    fputs("  <meta charset=\"UTF-8\">\n", temp_file);
    char* html_title = html2tex_extract_title(root);
    fputs("  <title>", temp_file);

    if (!html_title)
        fputs("Parsed HTML Output", temp_file);
    else {
        fputs(html_title, temp_file);
        free(html_title);
    }

    fputs("</title>\n", temp_file);
    fputs("</head>\n<body>\n", temp_file);
    HTMLNode* child = root->children;

    while (child) {
        write_pretty_node(temp_file, child, 1);
        child = child->next;
    }

    fputs("</body>\n</html>\n", temp_file);

    /* get the file size and read back */
    if (fseek(temp_file, 0, SEEK_END) != 0) {
        fclose(temp_file);
        return NULL;
    }

    long file_size = ftell(temp_file);

    if (file_size <= 0) {
        fclose(temp_file);
        return strdup("");
    }

    if (fseek(temp_file, 0, SEEK_SET) != 0) {
        fclose(temp_file);
        return NULL;
    }

    char* html_string = (char*)malloc(file_size + 1);

    if (!html_string) {
        fclose(temp_file);
        return NULL;
    }

    size_t bytes_read = fread(html_string, 1, file_size, temp_file);
    html_string[bytes_read] = '\0';

    fclose(temp_file);
    return html_string;
}