#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "html2tex.h"

#ifndef MAX_SPECIAL_TAG_LENGTH
#define MAX_SPECIAL_TAG_LENGTH 8
#endif

/* Check whether a tag is safe to minify by removing surrounding whitespace. */
static int is_safe_to_minify_tag(const char* tag_name) {
    /* read-only tags where whitespace is significant */
    static const TagProperties preserve_whitespace_tags[] = {
        {"pre", 'p', 3}, {"code", 'c', 4}, {"textarea", 't', 8},
        {"script", 's', 6}, {"style", 's', 5}, {NULL, 0, 0}
    };

    return !html2tex_tag_lookup(tag_name,
        preserve_whitespace_tags,
        MAX_SPECIAL_TAG_LENGTH);
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

    /* empty string */
    if (*value == '\0') {
        char* r = (char*)malloc(3);
        if (r) { r[0] = '"'; r[1] = '"'; r[2] = '\0'; }
        return r;
    }

    /* return the exact copy */
    return strdup(value);
}

/* Fast minification function of HTML's DOM tree. */
static HTMLNode* minify_node(HTMLNode* node, int in_preformatted) {
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

    if (node->tag && !is_safe_to_minify_tag(node->tag))
        current_preformatted = 1;

    /* minify the attributes */
    HTMLAttribute* new_attrs = NULL;

    HTMLAttribute** current_attr = &new_attrs;
    HTMLAttribute* old_attr = node->attributes;

    while (old_attr) {
        HTMLAttribute* new_attr = (HTMLAttribute*)malloc(sizeof(HTMLAttribute));

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
        HTMLNode* minified_child = minify_node(old_child, current_preformatted);

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
        if (!is_essential_element(new_node->tag)) {
            html2tex_free_node(new_node);
            return NULL;
        }
    }

    return new_node;
}

HTMLNode* html2tex_minify_html(const HTMLNode* root) {
    if (!root) return NULL;

    /* create a new minified tree */
    HTMLNode* minified_root = (HTMLNode*)malloc(sizeof(HTMLNode));

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
        HTMLNode* minified_child = minify_node(old_child, 0);

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