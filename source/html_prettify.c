#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* This helper function to check if element is inline, required for formatting. */
static int is_inline_element_for_formatting(const char* tag_name) {
    if (!tag_name || tag_name[0] == '\0') return 0;

    static const struct {
        const char* tag;
        unsigned char first_char;
        const unsigned char length;
    } inline_tags[] = {
        {"a", 'a', 1}, {"abbr", 'a', 4}, {"b", 'b', 1}, {"bdi", 'b', 3}, {"bdo", 'b', 3}, {"br", 'b', 2},
        {"cite", 'c', 4}, {"code", 'c', 4}, {"data", 'd', 4}, {"dfn", 'd', 3}, {"em", 'e', 2}, {"font", 'f', 4},
        {"i", 'i', 1}, {"kbd", 'k', 3}, {"mark", 'm', 4}, {"q", 'q', 1}, {"rp", 'r', 2}, {"rt", 'r', 2},
        {"ruby", 'r', 4}, {"samp", 's', 4}, {"small", 's', 5}, {"span", 's', 4}, {"strong", 's', 6},
        {"sub", 's', 3}, {"sup", 's', 3}, {"time", 't', 4}, {"u", 'u', 1}, {"var", 'v', 3}, {"wbr", 'w', 3}, {NULL, 0, 0}
    };

    /* compute length once, because the most tags will fail this check */
    size_t len = 0;

    const char* p = tag_name;
    while (*p) { len++; p++; }

    /* quick length-based rejection */
    switch (len) {
    case 1: case 2: case 3:
    case 4: case 5: case 6:
        break;
    default:
        return 0;
    }

    /* check first character */
    unsigned char first_char = (unsigned char)tag_name[0];

    for (int i = 0; inline_tags[i].tag; i++) {
        /* fast rejection for first char mismatch */
        if (inline_tags[i].first_char != first_char) continue;

        /* fast reject by length first */
        if (inline_tags[i].length != len) continue;

        /* compare the rest of tag name */
        if (strcmp(tag_name, inline_tags[i].tag) == 0)
            return 1;
    }

    return 0;
}

/* This helper function is used to escape HTML special characters. */
static char* escape_html(const char* text) {
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
    if (extra == 0) return strdup(text);

    /* allocate once */
    size_t len = p - text;

    char* escaped = (char*)malloc(len + extra + 1);
    if (!escaped) return NULL;

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
    if (!node || !file) return;

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
                fputs(" ", file); fputs(attr->key, file); fputs("=\"", file);

                fputs(escaped_value ? escaped_value : attr->value, file);
                fputs("\"", file);

                if (escaped_value) 
                    free(escaped_value);
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
            }

            /* write children with proper indentation */
            if (node->children) {
                if (!is_inline) fputs("\n", file);
                HTMLNode* child = node->children;

                while (child) {
                    write_pretty_node(file, child, indent_level + 1);
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

            if (escaped_content) {
                /* check if this is mostly whitespace */
                int all_whitespace = 1;

                for (char* p = escaped_content; *p; p++) {
                    if (!isspace(*p)) {
                        all_whitespace = 0;
                        break;
                    }
                }

                if (!all_whitespace) {
                    fputs(escaped_content, file);
                    fputs("\n", file);
                }
                else
                    /* for whitespace-only nodes, just output a newline */
                    fputs("\n", file);

                free(escaped_content);
            }
        }
    }
}

int write_pretty_html(const HTMLNode* root, const char* filename) {
    if (!root || !filename) return 0;
    FILE* file = fopen(filename, "w");

    if (!file) {
        fputs("Error: Could not open file ", stderr);
        fputs(filename, stderr);
        fputs(" for writing.\n", stderr);
        return 0;
    }

    /* write HTML header */
    fputs("<html>\n<head>\n", file);
    fputs("  <meta charset=\"UTF-8\">\n", file);
    char* html_title = html2tex_extract_title(root);
    int default_title = !html_title ? 1 : 0;
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
        child = child->next;
    }

    /* write HTML footer and close the stream */
    fputs("</body>\n</html>\n", file);
    fclose(file);
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