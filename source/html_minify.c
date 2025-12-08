#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "html2tex.h"

/* Check whether a tag is safe to minify by removing surrounding whitespace. */
static int is_safe_to_minify_tag(const char* tag_name) {
    if (!tag_name || tag_name[0] == '\0') return 0;

    /* read-only tags where whitespace is significant */
    static const char* const preserve_whitespace_tags[] = {
        "pre", "code", "textarea", "script", "style", NULL
    };

    /* compute length once, because the most tags will fail this check */
    size_t len = 0;

    const char* p = tag_name;
    while (*p) { len++; p++; }

    /* quick length-based rejection */
    switch (len) {
    case 3: case 4: case 5: case 6: case 8:
        break;
    default:
        return 1;
    }

    /* check first character before strcmp */
    char first_char = tag_name[0];

    for (int i = 0; preserve_whitespace_tags[i]; i++) {
        /* fast rejection before expensive strcmp function call */
        if (preserve_whitespace_tags[i][0] != first_char) continue;

        /* apply strcmp function only for the few cases */
        if (strcmp(tag_name, preserve_whitespace_tags[i]) == 0)
            return 0;
    }

    return 1;
}

/* Check if text content is only whitespace. */
static int is_whitespace_only(const char* text) {
    if (!text) return 1;
    const unsigned char* p = (const unsigned char*)text;

    while (*p) {
        unsigned char c = *p++;
        switch (c) {
        case ' ': case '\t': case '\n': case '\v': case '\f': case '\r':
            continue;
        default:
            return 0;
        }
    }
}

/* Remove the unnecessary whitespace from text content. */
static char* minify_text_content(const char* text, int is_in_preformatted) {
    /* quick null check */
    if (!text) return NULL;

    /* preformatted content (copy as-is) */
    if (is_in_preformatted) return strdup(text);
    const unsigned char* src = (const unsigned char*)text;

    /* empty string */
    if (*src == '\0') return NULL;

    /* single character, common in HTML */
    if (src[1] == '\0') {
        /* check if it's whitespace */
        unsigned char c = src[0];

        if (c == ' ' || c == '\t' || c == '\n' ||
            c == '\v' || c == '\f' || c == '\r')
            return NULL;

        /* non-whitespace single char found */
        char* result = (char*)malloc(2);

        if (result) {
            result[0] = c;
            result[1] = '\0';
        }

        return result;
    }

    /* analyze string and compute final size */
    const unsigned char* scan = src;
    size_t final_size = 0;
    int in_whitespace = 0;
    int has_content = 0;

    while (*scan) {
        unsigned char c = *scan;

        /* using ASCII whitespace check, that is faster than isspace */
        if (c == ' ' || c == '\t' || c == '\n' ||
            c == '\v' || c == '\f' || c == '\r') {
            /* only count space if not consecutive and not at start */
            if (!in_whitespace && has_content)
                final_size++;
            
            in_whitespace = 1;
        }
        else {
            /* non-whitespace char */
            final_size++;
            has_content = 1;
            in_whitespace = 0;
        }
        scan++;
    }

    /* all whitespace or empty result */
    if (final_size == 0) return NULL;

    /* adjust for trailing whitespace */
    if (in_whitespace && final_size > 0)
        final_size--;

    /* no whitespace at all */
    if (final_size == (size_t)(scan - src)) {
        /* just copy the string */
        char* result = (char*)malloc(final_size + 1);
        if (!result) return NULL;

        memcpy(result, text, final_size);
        result[final_size] = '\0';
        return result;
    }

    /* alloc exact size needed */
    char* result = (char*)malloc(final_size + 1);
    if (!result) return NULL;

    /* build minified string */
    char* dest = result;
    in_whitespace = 0;
    int at_start = 1;

    while (*src) {
        unsigned char c = *src++;

        if (c == ' ' || c == '\t' || c == '\n' ||
            c == '\v' || c == '\f' || c == '\r') {
            /* collapse multiple whitespace to single space */
            if (!in_whitespace && !at_start)
                *dest++ = ' ';

            in_whitespace = 1;
        }
        else {
            /* copy non-whitespace char */
            *dest++ = c;
            in_whitespace = 0;
            at_start = 0;
        }
    }

    /* remove trailing space if we added one */
    if (in_whitespace && dest > result)
        dest--;

    *dest = '\0';
    return result;
}

/* Minify the attribute value by removing unnecessary quotes when possible. */
static char* minify_attribute_value(const char* value) {
    if (!value) return NULL;
    const unsigned char* src = (const unsigned char*)value;

    /* empty string */
    if (*src == '\0') {
        char* r = (char*)malloc(3);
        if (r) { r[0] = '"'; r[1] = '"'; r[2] = '\0'; }
        return r;
    }

    /* single pass to analyze and optionally build */
    int needs_quotes = 0;
    int has_single = 0;
    int has_double = 0;
    size_t double_count = 0;
    size_t len = 0;

    /* quick check for simple strings */
    while (*src) {
        unsigned char c = *src++;
        len++;

        /* check for char requiring quotes */
        switch (c) {
        case ' ': case '\t': case '\n': case '\r':
        case '\f': case '\v': case '=': case '<':
        case '>': case '`':
            needs_quotes = 1;
            break;
        case '\'':
            has_single = 1;
            break;
        case '"':
            has_double = 1;
            double_count++;
            break;
        }

        /* if need quotes and have both quote types, break early */
        if (needs_quotes && has_single && has_double) break;
    }

    /* reset pointer for potential second pass */
    src = (const unsigned char*)value;

    /* no quotes needed */
    if (!needs_quotes) {
        char* result = (char*)malloc(len + 1);

        if (result) {
            memcpy(result, value, len);
            result[len] = '\0';
        }

        return result;
    }

    /* determine best quote type and build result */
    if (!has_double) {
        /* use double quotes, no escape needed */
        char* result = (char*)malloc(len + 3);
        if (!result) return NULL;

        result[0] = '"';
        memcpy(result + 1, value, len);
        result[len + 1] = '"';
        result[len + 2] = '\0';
        return result;
    }

    if (!has_single) {
        /* use single quotes without escape */
        char* result = (char*)malloc(len + 3);
        if (!result) return NULL;

        result[0] = '\'';
        memcpy(result + 1, value, len);
        result[len + 1] = '\'';
        result[len + 2] = '\0';
        return result;
    }

    /* need to escape double quotes */
    size_t total_len = len + double_count + 3;

    char* result = (char*)malloc(total_len);
    if (!result) return NULL;

    char* dest = result;
    *dest++ = '"';

    while (*src) {
        if (*src == '"') *dest++ = '\\';
        *dest++ = *src++;
    }

    *dest++ = '"';
    *dest = '\0';

    return result;
}

/* Recursive minification function */
static HTMLNode* minify_node_recursive(HTMLNode* node, int in_preformatted) {
    if (!node) return NULL;

    HTMLNode* new_node = malloc(sizeof(HTMLNode));
    if (!new_node) return NULL;

    /* copy the DOM structure */
    new_node->tag = node->tag ? strdup(node->tag) : NULL;
    new_node->parent = NULL;

    new_node->next = NULL;
    new_node->children = NULL;

    /* handle preformatted context */
    int current_preformatted = in_preformatted;

    if (node->tag) {
        if (strcmp(node->tag, "pre") == 0 || strcmp(node->tag, "code") == 0 ||
            strcmp(node->tag, "textarea") == 0 || strcmp(node->tag, "script") == 0 ||
            strcmp(node->tag, "style") == 0)
            current_preformatted = 1;
    }

    /* minify attributes */
    HTMLAttribute* new_attrs = NULL;

    HTMLAttribute** current_attr = &new_attrs;
    HTMLAttribute* old_attr = node->attributes;

    while (old_attr) {
        HTMLAttribute* new_attr = malloc(sizeof(HTMLAttribute));

        if (!new_attr) {
            html2tex_free_node(new_node);
            return NULL;
        }

        new_attr->key = strdup(old_attr->key);
        if (old_attr->value) new_attr->value = minify_attribute_value(old_attr->value);
        else new_attr->value = NULL;
        
        new_attr->next = NULL;
        *current_attr = new_attr;

        current_attr = &new_attr->next;
        old_attr = old_attr->next;
    }

    new_node->attributes = new_attrs;

    /* minify content */
    if (node->content) {
        if (is_whitespace_only(node->content) && !current_preformatted)
            /* remove whitespace-only text nodes outside preformatted blocks */
            new_node->content = NULL;
        else
            new_node->content = minify_text_content(node->content, current_preformatted);
    }
    else new_node->content = NULL;

    /* recursively minify children */
    HTMLNode* new_children = NULL;

    HTMLNode** current_child = &new_children;
    HTMLNode* old_child = node->children;

    int safe_to_minify = node->tag ? 
        is_safe_to_minify_tag(node->tag) : 1;

    while (old_child) {
        HTMLNode* minified_child = minify_node_recursive(old_child, current_preformatted);

        if (minified_child) {
            /* remove empty text nodes between elements (except in preformatted) */
            if (!minified_child->tag && !minified_child->content)
                html2tex_free_node(minified_child);
            else {
                /* remove whitespace between block elements */
                if (safe_to_minify && !current_preformatted) {
                    if (minified_child->tag && is_block_element(minified_child->tag)) {
                        /* skip whitespace before block elements */
                        HTMLNode* next = old_child->next;

                        /* skip the whitespace node */
                        if (next && !next->tag && is_whitespace_only(next->content))
                            old_child = next;
                    }
                }

                minified_child->parent = new_node;
                *current_child = minified_child;
                current_child = &minified_child->next;
            }
        }

        old_child = old_child->next;
    }

    new_node->children = new_children;

    /* remove empty nodes (except essential ones) */
    if (new_node->tag && !new_node->children && !new_node->content) {
        const char* essential_tags[] = {
            "br", "hr", "img", "input", "meta", "link", NULL
        };

        int is_essential = 0;

        for (int i = 0; essential_tags[i]; i++) {
            if (strcmp(new_node->tag, essential_tags[i]) == 0) {
                is_essential = 1;
                break;
            }
        }

        if (!is_essential) {
            html2tex_free_node(new_node);
            return NULL;
        }
    }

    return new_node;
}

HTMLNode* html2tex_minify_html(HTMLNode* root) {
    if (!root) return NULL;

    /* create a new minified tree */
    HTMLNode* minified_root = malloc(sizeof(HTMLNode));

    if (!minified_root) return NULL;
    minified_root->tag = NULL;

    minified_root->content = NULL;
    minified_root->attributes = NULL;

    minified_root->parent = NULL;
    minified_root->next = NULL;

    /* minify children */
    HTMLNode* new_children = NULL;

    HTMLNode** current_child = &new_children;
    HTMLNode* old_child = root->children;

    while (old_child) {
        HTMLNode* minified_child = minify_node_recursive(old_child, 0);

        if (minified_child) {
            minified_child->parent = minified_root;
            *current_child = minified_child;
            current_child = &minified_child->next;
        }

        old_child = old_child->next;
    }

    minified_root->children = new_children;
    return minified_root;
}