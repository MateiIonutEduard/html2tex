#include "html2tex.h"
#include <stdlib.h>
#include <ctype.h>

typedef struct TagProperties {
    const char* tag;
    unsigned char first_char;
    const unsigned char length;
} TagProperties;

char* html2tex_compress_html(const char* html) {
    html2tex_err_clear();

    if (!html) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, 
            "HTML input is NULL for compression.");
        return NULL;
    }

    size_t len = strlen(html);

    if (len == 0) {
        char* empty = strdup("");

        if (!empty) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                "Failed to allocate empty string"
                " for HTML compression.");
        }

        return empty;
    }

    char* result = (char*)malloc(len + 1);

    if (!result) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate %zu bytes for"
            " HTML compression buffer.", len + 1);
        return NULL;
    }

    const char* src = html;
    char* dest = result;

    int in_tag = 0;
    int in_comment = 0;
    int in_script_style = 0;
    int in_quotes = 0;
    char quote_char = 0;
    int last_char_was_gt = 0;
    int skip_whitespace = 0;

    while (*src) {
        unsigned char c = (unsigned char)*src;

        /* handle comments */
        if (!in_quotes && !in_tag && !in_script_style) {
            if (c == '<' && strncmp(src, "<!--", 4) == 0)
                in_comment = 1;
            else if (c == '-' && in_comment && 
                strncmp(src, "-->", 3) == 0) {
                in_comment = 0; *dest++ = '-'; 
                *dest++ = '-'; *dest++ = '>';
                src += 2; goto next_char;
            }
        }

        if (in_comment) {
            *dest++ = *src++;
            continue;
        }

        /* detect script/style tags */
        if (!in_quotes && c == '<') {
            const char* tag_start = src + 1;
            while (*tag_start && isspace((unsigned char)*tag_start)) 
                tag_start++;

            if (strncasecmp(tag_start, "script", 6) == 0 ||
                strncasecmp(tag_start, "style", 5) == 0)
                in_script_style = 1;
        }
        /* check for closing script/style tags */
        else if (!in_quotes && c == '<' && strncmp(src, "</", 2) == 0) {
            const char* tag_start = src + 2;
            while (*tag_start && isspace((unsigned char)*tag_start)) tag_start++;

            if (strncasecmp(tag_start, "script", 6) == 0 ||
                strncasecmp(tag_start, "style", 5) == 0)
                in_script_style = 0;
        }

        /* preserve all content inside script/style tags */
        if (in_script_style) {
            *dest++ = *src++;
            continue;
        }

        /* handle quotes in attributes */
        if (in_tag && !in_quotes && 
            (c == '\'' || c == '"')) {
            in_quotes = 1;
            quote_char = c;
        }
        else if (in_quotes && 
            c == quote_char)
            in_quotes = 0;

        /* tag detection */
        if (!in_quotes) {
            if (c == '<') {
                in_tag = 1;
                skip_whitespace = 0;
            }
            else if (c == '>') {
                in_tag = 0;
                last_char_was_gt = 1;
                skip_whitespace = 1;
            }
        }

        /* whitespace handling */
        if (c == ' ' || c == '\t' || 
            c == '\n' || c == '\r' || 
            c == '\f' || c == '\v') {
            if (in_tag || in_quotes)
                *dest++ = c;
            else if (last_char_was_gt)
                skip_whitespace = 1;
            else if (skip_whitespace) {
                /* skip consecutive whitespace between tags */
                /* do nothing, just skip this whitespace */
            }
            else {
                *dest++ = ' ';
                skip_whitespace = 1;
            }
        }
        else {
            /* non-whitespace character */
            if (skip_whitespace && !in_tag && 
                !last_char_was_gt && dest > result) {
                char last = *(dest - 1);
                if (last != ' ' && last != '>' && last != '<')
                    *dest++ = ' ';
            }

            *dest++ = c;
            skip_whitespace = 0;
            last_char_was_gt = 0;
        }

    next_char:
        src++;
    }

    *dest = '\0';

    /* trim to actual size */
    size_t final_len = dest - result;
    char* final_result = (char*)realloc(result, final_len + 1);

    if (!final_result) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to reallocate HTML compression"
            " buffer to %zu bytes.",
            final_len + 1);

        return result;
    }

    return final_result;
}

HTMLElement* search_tree(HTMLNode* root, int (*predicate)(HTMLNode*, void*), void* data, CSSProperties* inherited_props) {
    html2tex_err_clear();

    if (!root) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, 
            "HTML root node is NULL for tree search.");
        return NULL;
    }

    if (!predicate) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, 
            "Predicate function is NULL for"
            " tree search.");
        return NULL;
    }

    Stack* node_stack = NULL;
    Stack* css_stack = NULL;
    HTMLElement* result = NULL;

    /* push root node with initial CSS properties */
    if (!stack_push(&node_stack, (void*)root) ||
        !stack_push(&css_stack, (void*)inherited_props)) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to push initial nodes onto"
            " stack for tree search.");
        goto cleanup;
    }

    /* pop current context */
    while (!stack_is_empty(node_stack)) {
        CSSProperties* current_css = (CSSProperties*)stack_pop(&css_stack);
        HTMLNode* current_node = (HTMLNode*)stack_pop(&node_stack);

        if (!current_node) {
            /* only destroy if we allocated */
            if (current_css && current_css != inherited_props)
                css_properties_destroy(current_css);
            
            continue;
        }

        /* merge CSS properties with inline styles if node has style attribute */
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
                        /* if merge failed, merged_css might be current_css or NULL */
                        if (merged_css && merged_css != current_css)
                            css_properties_destroy(merged_css);
                        
                        if (current_css && current_css != inherited_props)
                            css_properties_destroy(current_css);
                        
                        goto cleanup;
                    }
                }
            }
        }

        /* check current node with predicate */
        if (predicate(current_node, data)) {
            result = (HTMLElement*)malloc(sizeof(HTMLElement));

            if (!result) {
                HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                    "Failed to allocate HTMLElement structure.");

                if (merged_css && merged_css != current_css &&
                    merged_css != inherited_props)
                    css_properties_destroy(merged_css);
                
                if (current_css && current_css != inherited_props)
                    css_properties_destroy(current_css);
                
                goto cleanup;
            }

            result->node = current_node;

            /* copy the CSS properties for the result */
            if (merged_css) result->css_props = css_properties_copy(merged_css);
            else result->css_props = css_properties_create();

            if (!result->css_props || html2tex_has_error()) {
                if (result->css_props)
                    css_properties_destroy(result->css_props);
                
                free(result);
                result = NULL;

                if (merged_css && merged_css != current_css &&
                    merged_css != inherited_props)
                    css_properties_destroy(merged_css);
                
                if (current_css && current_css != inherited_props) 
                    css_properties_destroy(current_css);
                
                goto cleanup;
            }

            /* clean up current node's CSS props */
            if (merged_css && merged_css != current_css &&
                merged_css != inherited_props)
                css_properties_destroy(merged_css);
            
            if (current_css && current_css != inherited_props)
                css_properties_destroy(current_css);

            break;
        }

        /* push children in reverse order */
        if (current_node->children) {
            HTMLNode* last_child = current_node->children;
            while (last_child->next) last_child = last_child->next;
            HTMLNode* child = last_child;

            while (child) {
                CSSProperties* child_css = merged_css;

                /* only copy CSS if it's different from inherited and it need a unique copy */
                if (child_css && child_css != inherited_props) {
                    child_css = css_properties_copy(merged_css);
                    if (!child_css || html2tex_has_error()) {
                        if (merged_css && merged_css != current_css 
                            && merged_css != inherited_props) 
                            css_properties_destroy(merged_css);
                        
                        if (current_css && current_css != inherited_props) 
                            css_properties_destroy(current_css);
                        
                        goto cleanup;
                    }
                }

                /* push child onto stack */
                if (!stack_push(&node_stack, (void*)child) || 
                    !stack_push(&css_stack, (void*)child_css)) {
                    if (child_css && child_css != merged_css && child_css != inherited_props)
                        css_properties_destroy(child_css);

                    if (merged_css && merged_css != current_css && merged_css != inherited_props)
                        css_properties_destroy(merged_css);
                    
                    if (current_css && current_css != inherited_props)
                        css_properties_destroy(current_css);

                    HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                        "Failed to push child node onto stack.");
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

        /* clean up current node's CSS if we haven't passed it to children */
        if (merged_css && merged_css != current_css && merged_css != inherited_props)
            css_properties_destroy(merged_css);
        
        if (current_css && current_css != inherited_props)
            css_properties_destroy(current_css);

        /* check for errors from predicate or other operations */
        if (html2tex_has_error()) goto cleanup;
    }

cleanup:
    /* clean up any remaining CSS properties on the stack */
    while (!stack_is_empty(css_stack)) {
        CSSProperties* css = (CSSProperties*)stack_pop(&css_stack);
        if (css && css != inherited_props)
            css_properties_destroy(css);
    }

    /* clean up any remaining nodes on the stack */
    while (!stack_is_empty(node_stack))
        stack_pop(&node_stack);

    /* clean up stacks */
    stack_cleanup(&node_stack);
    stack_cleanup(&css_stack);

    /* if error occurred but result was allocated, free it */
    if (html2tex_has_error() && result) {
        html_element_destroy(result);
        result = NULL;
    }

    return result;
}

void html_element_destroy(HTMLElement* elem) {
    if (elem) {
        if (elem->css_props)
            css_properties_destroy(elem->css_props);
        free(elem);
    }
}

const char* get_attribute(HTMLAttribute* attrs, const char* key) {
    html2tex_err_clear();

    if (!attrs) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, 
            "Attribute list is NULL.");
        return NULL;
    }

    if (!key) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, 
            "Attribute key is NULL.");
        return NULL;
    }

    if (key[0] == '\0') {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_INVAL, 
            "Attribute key is empty.");
        return NULL;
    }

    size_t key_len = 0;

    /* precompute key length once for fast rejection */
    while (key[key_len]) key_len++;

    for (HTMLAttribute* attr = attrs; attr; attr = attr->next) {
        /* check first char for early rejection */
        if (!attr->key || attr->key[0] != key[0]) continue;

        /* length-based fast rejection */
        size_t attr_len = 0;

        while (attr->key[attr_len]) attr_len++;
        if (attr_len != key_len) continue;

        /* exact case-insensitive comparison */
        int match = 1;

        for (size_t i = 0; i < key_len; i++) {
            char c1 = attr->key[i];
            char c2 = key[i];

            /* fast ASCII case conversion */
            if ((c1 ^ c2) & 0x20) {
                /* convert both to lowercase for comparison */
                c1 |= 0x20;
                c2 |= 0x20;
            }

            if (c1 != c2) {
                match = 0;
                break;
            }
        }

        if (match) return attr->value;
    }

    return NULL;
}

char* html2tex_extract_title(const HTMLNode* root) {
    html2tex_err_clear();

    if (!root) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, 
            "Root node is NULL for title extraction.");
        return NULL;
    }

    Queue* front = NULL;
    Queue* rear = NULL;
    char* title_text = NULL;

    /* initialize BFS with root's direct children */
    HTMLNode* child = root->children;

    while (child) {
        if (!queue_enqueue(&front, &rear, child)) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                "Failed to enqueue child node for"
                " BFS title search.");
            goto cleanup;
        }
        child = child->next;
    }

    /* BFS search for title element */
    while (front) {
        HTMLNode* current = (HTMLNode*)queue_dequeue(&front, &rear);
        if (!current) continue;

        /* Fast check for title tag, optimized with length check first */
        if (current->tag && current->tag[0] == 't') {
            size_t tag_len = strlen(current->tag);

            /* extract all text content from title element */
            if (tag_len == 5 && strcmp(current->tag, "title") == 0) {
                size_t capacity = HTML_TITLE_MAX_SIZE;
                char* buffer = (char*)malloc(capacity);

                if (!buffer) {
                    HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                        "Failed to allocate %zu bytes for"
                        " title buffer.", capacity);
                    goto cleanup;
                }

                size_t length = 0;
                buffer[0] = '\0';

                /* BFS within title node for text content */
                Queue* title_front = NULL;
                Queue* title_rear = NULL;

                if (!queue_enqueue(&title_front, &title_rear, current)) {
                    free(buffer);
                    HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                        "Failed to initialize BFS for title"
                        " content extraction.");
                    goto cleanup;
                }

                while (title_front) {
                    HTMLNode* title_node = (HTMLNode*)queue_dequeue(&title_front, &title_rear);

                    if (!title_node)
                        continue;

                    /* collect text content */
                    if (!title_node->tag && title_node->content && title_node->content[0]) {
                        size_t text_len = strlen(title_node->content);

                        /* ensure capacity with safe overflow checking */
                        if (length > SIZE_MAX - text_len - 1) {
                            free(buffer);
                            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                                "Title text size exceeds maximum allowed.");
                            queue_cleanup(&title_front, &title_rear);
                            goto cleanup;
                        }

                        if (length + text_len + 1 > capacity) {
                            /* calculate new capacity with overflow protection */
                            size_t new_capacity;

                            /* cannot double, try to add exactly what's needed */
                            if (capacity > SIZE_MAX / 2) {
                                if (SIZE_MAX - length - text_len - 1 < capacity) {
                                    free(buffer);
                                    HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                                        "Title text requires capacity exceeding SIZE_MAX.");
                                    queue_cleanup(&title_front, &title_rear);
                                    goto cleanup;
                                }

                                new_capacity = length + text_len + 1;
                            }
                            else {
                                /* double capacity with overflow check */
                                new_capacity = capacity * 2;

                                if (new_capacity < length + text_len + 1)
                                    new_capacity = length + text_len + 1;
                            }

                            char* new_buffer = (char*)realloc(buffer, new_capacity);
                            if (!new_buffer) {
                                free(buffer);
                                HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                                    "Failed to reallocate title buffer"
                                    " from %zu to %zu bytes.",
                                    capacity, new_capacity);
                                queue_cleanup(&title_front, &title_rear);
                                goto cleanup;
                            }

                            buffer = new_buffer;
                            capacity = new_capacity;
                        }

                        /* safe copy with bounds check */
                        memcpy(buffer + length, title_node->content, text_len);
                        length += text_len;
                        buffer[length] = '\0';
                    }

                    /* enqueue children */
                    HTMLNode* title_child = title_node->children;

                    while (title_child) {
                        if (!queue_enqueue(&title_front, &title_rear, title_child)) {
                            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                                "Failed to enqueue title child node.");
                            free(buffer);
                            queue_cleanup(&title_front, &title_rear);
                            goto cleanup;
                        }

                        title_child = title_child->next;
                    }
                }

                queue_cleanup(&title_front, &title_rear);

                /* process the extracted title text */
                if (length > 0) {
                    /* trim whitespace efficiently */
                    char* start = buffer;
                    char* end = buffer + length - 1;

                    /* trim leading whitespace */
                    while (start <= end && isspace((unsigned char)*start)) {
                        start++;
                    }

                    /* trim trailing whitespace */
                    while (end >= start && isspace((unsigned char)*end))
                        end--;

                    size_t trimmed_len = (end >= start) ? 
                        (size_t)(end - start + 1) : 0;

                    if (trimmed_len > 0) {
                        /* allocate exact size for trimmed title */
                        title_text = (char*)malloc(trimmed_len + 1);

                        if (!title_text) {
                            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                                "Failed to allocate %zu bytes for"
                                " trimmed title.", trimmed_len + 1);
                            free(buffer);
                            goto cleanup;
                        }

                        memcpy(title_text, start, trimmed_len);
                        title_text[trimmed_len] = '\0';
                    }
                    else {
                        /* empty after trimming */
                        HTML2TEX__SET_ERR(HTML2TEX_ERR_PARSE,
                            "Title element contains only whitespace.");
                    }

                    free(buffer);
                }
                else {
                    /* empty title */
                    HTML2TEX__SET_ERR(HTML2TEX_ERR_PARSE,
                        "Title element contains no text content.");
                    free(buffer);
                }

                /* found title, exit BFS */
                break;
            }
        }

        /* check if an error occurred during processing */
        if (html2tex_has_error())
            goto cleanup;

        /* enqueue children for further search */
        HTMLNode* current_child = current->children;

        while (current_child) {
            if (!queue_enqueue(&front, &rear, current_child)) {
                HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                    "Failed to enqueue child node during"
                    " BFS title search.");
                goto cleanup;
            }
            current_child = current_child->next;
        }
    }

cleanup:
    /* clean up BFS queues */
    queue_cleanup(&front, &rear);

    /* if error occurred but title_text was allocated, free it */
    if (html2tex_has_error() && title_text) {
        free(title_text);
        title_text = NULL;
    }

    /* return result */
    return title_text;
}

int should_skip_nested_table(const HTMLNode* node) {
    html2tex_err_clear();

    if (!node) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, 
            "Node is NULL for nested table check.");
        return -1;
    }

    Queue* front = NULL;
    Queue* rear = NULL;
    int result = 0;

    /* if current node is a table, check for nested tables in descendants */
    if (node->tag && strcmp(node->tag, "table") == 0) {
        if (!node->children) 
            return 0;

        /* enqueue direct children */
        HTMLNode* child = node->children;

        while (child) {
            if (!queue_enqueue(&front, &rear, child)) {
                HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                    "Failed to enqueue child for nested"
                    " table BFS.");
                goto cleanup;
            }

            child = child->next;
        }

        /* BFS for nested tables */
        HTMLNode* current;
        while ((current = (HTMLNode*)queue_dequeue(&front, &rear))) {
            if (current->tag && strcmp(current->tag, "table") == 0) {
                result = 1;
                goto cleanup;
            }

            /* enqueue children for further search */
            HTMLNode* grandchild = current->children;

            while (grandchild) {
                if (!queue_enqueue(&front, &rear, grandchild)) {
                    HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                        "Failed to enqueue grandchild for"
                        " nested table BFS.");
                    goto cleanup;
                }
                grandchild = grandchild->next;
            }

            /* check if error occurred during processing */
            if (html2tex_has_error())
                goto cleanup;
        }
    }

    /* check parent hierarchy for table with nested tables */
    for (HTMLNode* parent = node->parent; parent; parent = parent->parent) {
        if (parent->tag && strcmp(parent->tag, "table") == 0) {
            queue_cleanup(&front, &rear);

            /* enqueue parent's children, excluding current node */
            HTMLNode* sibling = parent->children;

            while (sibling) {
                if (sibling != node && !queue_enqueue(&front, &rear, sibling)) {
                    HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                        "Failed to enqueue sibling for "
                        "parent table BFS.");
                    goto cleanup;
                }

                sibling = sibling->next;
            }

            /* BFS for nested tables in parent's descendants */
            HTMLNode* current;
            while ((current = (HTMLNode*)queue_dequeue(&front, &rear))) {
                if (current->tag && strcmp(current->tag, "table") == 0) {
                    result = 1;
                    goto cleanup;
                }

                /* enqueue children for further search */
                HTMLNode* grandchild = current->children;
                while (grandchild) {
                    if (!queue_enqueue(&front, &rear, grandchild)) {
                        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                            "Failed to enqueue parent descendant for BFS.");
                        goto cleanup;
                    }
                    grandchild = grandchild->next;
                }

                /* check if error occurred during processing */
                if (html2tex_has_error())
                    goto cleanup;
            }

            /* found nested table in parent chain? */
            if (result) break;
        }
    }

cleanup:
    queue_cleanup(&front, &rear);

    /* return -1 on error, otherwise the actual result */
    if (html2tex_has_error())  return -1;
    return result;
}

int table_contains_only_images(const HTMLNode* node) {
    html2tex_err_clear();

    if (!node) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, 
            "Node is NULL for table image check.");
        return -1;
    }

    if (!node->tag || strcmp(node->tag, "table") != 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_INVAL,
            "Node is not a table element for"
            " image-only check.");
        return -1;
    }

    Queue* front = NULL;
    Queue* rear = NULL;
    int has_images = 0;

    /* enqueue direct children */
    HTMLNode* child = node->children;
    while (child) {
        if (!queue_enqueue(&front, &rear, child)) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                "Failed to enqueue table child "
                "for BFS traversal.");
            goto cleanup;
        }
        child = child->next;
    }

    /* BFS traversal */
    HTMLNode* current;

    while ((current = (HTMLNode*)queue_dequeue(&front, &rear))) {
        /* error occurred during processing */
        if (html2tex_has_error()) {
            has_images = 0;
            goto cleanup;
        }

        if (current->tag) {
            /* fast path for image tag detection */
            if (current->tag[0] == 'i' && strcmp(current->tag, "img") == 0) {
                has_images = 1;
                continue;
            }

            /* optimized check for structural table elements */
            char first_char = current->tag[0];
            int is_table_element = 0;

            switch (first_char) {
            case 't':
                /* check specific 't' tags */
                if (strcmp(current->tag, "tbody") == 0 ||
                    strcmp(current->tag, "thead") == 0 ||
                    strcmp(current->tag, "tfoot") == 0 ||
                    strcmp(current->tag, "tr") == 0 ||
                    strcmp(current->tag, "td") == 0 ||
                    strcmp(current->tag, "th") == 0)
                    is_table_element = 1;
                
                break;

            case 'c':
                /* check caption */
                if (strcmp(current->tag, "caption") == 0)
                    is_table_element = 1;
                
                break;

            default:
                break;
            }

            /* enqueue children */
            if (is_table_element) {
                HTMLNode* inner_child = current->children;
                while (inner_child) {
                    if (!queue_enqueue(&front, &rear, inner_child)) {
                        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                            "Failed to enqueue inner child "
                            "during table BFS.");
                        has_images = 0;
                        goto cleanup;
                    }
                    inner_child = inner_child->next;
                }
                continue;
            }

            /* any other tag means failure */
            has_images = 0;
            goto cleanup;
        }
        else if (current->content) {
            /* check for non-whitespace text */
            const char* p = current->content;

            while (*p) {
                if (!isspace((unsigned char)*p)) {
                    has_images = 0;
                    goto cleanup;
                }

                p++;
            }
        }
    }

cleanup:
    queue_cleanup(&front, &rear);
    return has_images;
}

void convert_image_table(LaTeXConverter* converter, const HTMLNode* node) {
    html2tex_err_clear();

    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, 
            "Converter is NULL for image "
            "table conversion.");
        return;
    }

    if (!node) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, 
            "Table node is NULL for image "
            "table conversion.");
        return;
    }

    /* write figure header */
    append_string(converter, "\\begin{figure}[htbp]\n\\centering\n");
    if (html2tex_has_error()) return;

    append_string(converter, "\\setlength{\\fboxsep}{0pt}\n\\setlength{\\tabcolsep}{1pt}\n");
    if (html2tex_has_error()) return;

    /* start tabular */
    int columns = count_table_columns(node);
    if (html2tex_has_error()) return;

    if (columns <= 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_TABLE_STRUCTURE,
            "Invalid column count (%d) for image table.", 
            columns);
        return;
    }

    append_string(converter, "\\begin{tabular}{");
    if (html2tex_has_error()) return;

    for (int i = 0; i < columns; i++) {
        append_string(converter, "c");
        if (html2tex_has_error()) 
            goto cleanup_queues;
    }

    append_string(converter, "}\n");
    if (html2tex_has_error()) goto cleanup_queues;

    /* BFS for table rows */
    Queue* queue = NULL, * rear = NULL;
    Queue* cell_queue = NULL, * cell_rear = NULL;
    int first_row = 1;

    /* enqueue table children */
    HTMLNode* child = node->children;

    while (child) {
        if (!queue_enqueue(&queue, &rear, child)) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                "Failed to enqueue table child "
                "for BFS traversal.");
            goto cleanup_queues;
        }

        child = child->next;
    }

    /* process table */
    HTMLNode* current;
    while ((current = (HTMLNode*)queue_dequeue(&queue, &rear))) {
        if (html2tex_has_error()) goto cleanup_queues;
        if (!current->tag) continue;

        /* optimized tag checking with first char */
        char first_char = current->tag[0];

        /* check specific 't' tags */
        if (first_char == 't') {
            if (strcmp(current->tag, "tr") == 0) {
                if (!first_row) {
                    append_string(converter, " \\\\\n");
                    if (html2tex_has_error()) goto cleanup_queues;
                }

                first_row = 0;

                /* process cells in row */
                HTMLNode* cell = current->children;
                int col_count = 0;

                while (cell) {
                    if (html2tex_has_error())
                        goto cleanup_queues;

                    if (cell->tag && (cell->tag[0] == 't' &&
                        (strcmp(cell->tag, "td") == 0 || strcmp(cell->tag, "th") == 0))) {
                        if (col_count++ > 0) {
                            append_string(converter, " & ");
                            if (html2tex_has_error()) goto cleanup_queues;
                        }

                        /* BFS search for image in cell */
                        cell_queue = cell_rear = NULL;
                        HTMLNode* cell_child = cell->children;

                        while (cell_child) {
                            if (!queue_enqueue(&cell_queue, &cell_rear, cell_child)) {
                                HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                                    "Failed to enqueue cell child for"
                                    " image search.");
                                goto cleanup_queues;
                            }

                            cell_child = cell_child->next;
                        }

                        int img_found = 0;
                        HTMLNode* cell_node;

                        while ((cell_node = (HTMLNode*)queue_dequeue(&cell_queue, &cell_rear)) && !img_found) {
                            if (html2tex_has_error()) {
                                queue_cleanup(&cell_queue, &cell_rear);
                                goto cleanup_queues;
                            }

                            if (cell_node->tag && cell_node->tag[0] == 'i' &&
                                strcmp(cell_node->tag, "img") == 0) {
                                process_table_image(converter, cell_node);

                                if (html2tex_has_error()) {
                                    queue_cleanup(&cell_queue, &cell_rear);
                                    goto cleanup_queues;
                                }

                                img_found = 1;
                            }
                            /* enqueue children for deeper search */
                            else if (cell_node->tag) {
                                HTMLNode* grandchild = cell_node->children;
                                while (grandchild) {
                                    if (!queue_enqueue(&cell_queue, &cell_rear, grandchild)) {
                                        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                                            "Failed to enqueue grandchild for"
                                            " image search.");
                                        queue_cleanup(&cell_queue, &cell_rear);
                                        goto cleanup_queues;
                                    }
                                    grandchild = grandchild->next;
                                }
                            }
                        }

                        if (!img_found) {
                            append_string(converter, " ");
                            if (html2tex_has_error()) {
                                queue_cleanup(&cell_queue, &cell_rear);
                                goto cleanup_queues;
                            }
                        }

                        queue_cleanup(&cell_queue, &cell_rear);
                    }

                    cell = cell->next;
                }
            }
            /* enqueue section children */
            else if (strcmp(current->tag, "tbody") == 0 ||
                strcmp(current->tag, "thead") == 0 ||
                strcmp(current->tag, "tfoot") == 0) {
                HTMLNode* section_child = current->children;

                while (section_child) {
                    if (!queue_enqueue(&queue, &rear, section_child)) {
                        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                            "Failed to enqueue table section child.");
                        goto cleanup_queues;
                    }

                    section_child = section_child->next;
                }
            }
        }
    }

    /* finish tabular and figure */
    append_string(converter, "\n\\end{tabular}\n");
    if (html2tex_has_error()) goto cleanup_queues;

    append_figure_caption(converter, node);
    if (html2tex_has_error()) goto cleanup_queues;
    append_string(converter, "\\end{figure}\n\\FloatBarrier\n\n");

cleanup_queues:
    queue_cleanup(&queue, &rear);
    queue_cleanup(&cell_queue, &cell_rear);
}

int html2tex_tag_lookup(const char* tag_name, const TagProperties* table, size_t max_len) {
    if (!tag_name || tag_name[0] == '\0') 
        return 0;

    size_t len = 0;
    const char* p = tag_name;

    while (*p) {
        if (++len > max_len) 
            return 0;
        p++;
    }

    /* extract first char in advance */
    const unsigned char first_char = (unsigned char)tag_name[0];

    for (int i = 0; table[i].tag; i++) {
        /* fast reject: first character mismatch */
        if (first_char != table[i].first_char) continue;

        /* reject by length mismatch */
        if (len != table[i].length) continue;

        /* final verification by exact string match */
        if (strcmp(tag_name, table[i].tag) == 0) return 1;
    }

    return 0;
}

int is_block_element(const char* tag_name) {
    static const TagProperties block_tags[] = {
        {"div", 'd', 3}, {"p", 'p', 1},
        {"h1", 'h', 2}, {"h2", 'h', 2}, {"h3", 'h', 2},
        {"h4", 'h', 2}, {"h5", 'h', 2}, {"h6", 'h', 2},
        {"ul", 'u', 2}, {"ol", 'o', 2}, {"li", 'l', 2},
        {"table", 't', 5}, {"tr", 't', 2}, {"td", 't', 2}, 
        {"th", 't', 2}, {"blockquote", 'b', 10}, {"section", 's', 7}, 
        {"article", 'a', 7}, {"header", 'h', 6}, {"footer", 'f', 6},
        {"nav", 'n', 3}, {"aside", 'a', 5}, {"main", 'm', 4}, 
        {"figure", 'f', 6}, {"figcaption", 'f', 10}, 
        {"caption", 'c', 7}, {NULL, 0, 0}
    };

    return html2tex_tag_lookup(tag_name, block_tags,
        MAX_SUPPORTED_BLOCK_LENGTH);
}

int is_inline_element(const char* tag_name) {
    static const TagProperties inline_tags[] = {
        {"a", 'a', 1}, {"abbr", 'a', 4}, {"b", 'b', 1},
        {"bdi", 'b', 3}, {"bdo", 'b', 3}, {"cite", 'c', 4},
        {"code", 'c', 4}, {"data", 'd', 4}, {"dfn", 'd', 3},
        {"em", 'e', 2}, {"font", 'f', 4}, {"i", 'i', 1},
        {"kbd", 'k', 3}, {"mark", 'm', 4}, {"q", 'q', 1},
        {"rp", 'r', 2}, {"rt", 'r', 2}, {"ruby", 'r', 4},
        {"samp", 's', 4}, {"small", 's', 5}, {"span", 's', 4},
        {"strong", 's', 6}, {"sub", 's', 3}, {"sup", 's', 3},
        {"time", 't', 4}, {"u", 'u', 1}, {"var", 'v', 3},
        {"wbr", 'w', 3}, {"br", 'b', 2}, {"img", 'i', 3},
        {"map", 'm', 3}, {"object", 'o', 6}, {"button", 'b', 6},
        {"input", 'i', 5}, {"label", 'l', 5}, {"meter", 'm', 5},
        {"output", 'o', 6}, {"progress", 'p', 8}, {"select", 's', 6},
        {"textarea", 't', 8}, {NULL, 0, 0}
    };

    return html2tex_tag_lookup(tag_name, inline_tags,
        MAX_SUPPORTED_INLINE_LENGTH);
}

int is_void_element(const char* tag_name) {
    static const TagProperties void_tags[] = { 
        {"area", 'a', 4}, {"base", 'b', 4}, {"br", 'b', 2}, {"col", 'c', 3}, 
        {"embed", 'e', 5}, {"hr", 'h', 2}, {"img", 'i', 3}, {"input", 'i', 5}, 
        {"link", 'l', 4}, {"meta", 'm', 4}, {"param", 'p', 5}, {"source", 's', 6}, 
        {"track", 't', 5}, {"wbr", 'w', 3}, {NULL, 0, 0} 
    };

    return html2tex_tag_lookup(tag_name, void_tags, 
        MAX_SUPPORTED_VOID_LENGTH);
}

int is_essential_element(const char* tag_name) {
    static const TagProperties essential_tags[] = {
        {"br", 'b', 2}, {"hr", 'h', 2}, {"img", 'i', 3}, {"input", 'i', 5}, 
        {"meta", 'm', 4}, {"link", 'l', 4}, {NULL, 0, 0}
    };

    return html2tex_tag_lookup(tag_name, essential_tags, 
        MAX_SUPPORTED_ESSENTIAL_LENGTH);
}

int should_exclude_tag(const char* tag_name) {
    static const TagProperties excluded_tags[] = {
        {"script", 's', 6}, {"style", 's', 5}, {"link", 'l', 4},
        {"meta", 'm', 4}, {"head", 'h', 4}, {"noscript", 'n', 8},
        {"template", 't', 8}, {"iframe", 'i', 6}, {"form", 'f', 4},
        {"input", 'i', 5}, {"label", 'l', 5}, {"canvas", 'c', 6},
        {"svg", 's', 3}, {"video", 'v', 5}, {"source", 's', 6},
        {"audio", 'a', 5}, {"object", 'o', 6}, {"button", 'b', 6},
        {"map", 'm', 3}, {"area", 'a', 4}, {"frame", 'f', 5},
        {"frameset", 'f', 8}, {"noframes", 'n', 8}, {"nav", 'n', 3},
        {"picture", 'p', 7}, {"progress", 'p', 8}, {"select", 's', 6},
        {"option", 'o', 6}, {"param", 'p', 5}, {"search", 's', 6},
        {"samp", 's', 4}, {"track", 't', 5}, {"var", 'v', 3},
        {"wbr", 'w', 3}, {"mark", 'm', 4}, {"meter", 'm', 5},
        {"optgroup", 'o', 8}, {"q", 'q', 1}, {"blockquote", 'b', 10},
        {"bdo", 'b', 3}, {NULL, 0, 0}
    };

    return html2tex_tag_lookup(tag_name, excluded_tags,
        MAX_UNSUPPORTED_ELEMENT_LENGTH);
}

int is_whitespace_only(const char* text) {
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

    return 1;
}

int is_inside_table_cell(LaTeXConverter* converter, const HTMLNode* node) {
    html2tex_err_clear();

    /* validate inputs with appropriate error messages */
    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, 
            "Converter is NULL for table cell"
            " check.");
        return -1;
    }

    /* check converter state first */
    if (converter->state.in_table_cell) return 1;
    if (!node) return 0;

    /* check node's parent hierarchy for table cells */
    HTMLNode* current = node->parent;

    while (current) {
        if (current->tag) {
            if (current->tag[0] == 't') {
                if (strcmp(current->tag, "td") == 0 || 
                    strcmp(current->tag, "th") == 0)
                    return 1;
            }
        }

        current = current->parent;
    }

    return 0;
}

int is_inside_table(const HTMLNode* node) {
    if (!node) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, 
            "Node is NULL for table check.");
        return -1;
    }

    HTMLNode* current = node->parent;

    while (current) {
        if (current->tag && 
            strcmp(current->tag, "table") == 0)
            return 1;

        current = current->parent;
    }

    return 0;
}