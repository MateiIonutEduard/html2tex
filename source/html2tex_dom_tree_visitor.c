#include "html2tex.h"

static HTMLElementList* element_list_node_create(HTMLElement* element) {
    /* clear previous errors */
    html2tex_err_clear();

    HTMLElementList* node = (HTMLElementList*)malloc(sizeof(HTMLElementList));
    if (!node) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate HTMLElementList node.");
        return NULL;
    }

    node->element = element;
    node->next = NULL;
    return node;
}

HTMLNodeList* html_nodelist_create() {
    /* clear previous errors */
    html2tex_err_clear();
    HTMLNodeList* list = (HTMLNodeList*)calloc(1, sizeof(HTMLNodeList));

    if (!list) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate HTMLNodeList"
            " structure.");
        return NULL;
    }

    /* init tail pointer to point to head pointer */
    list->tail = &list->head;
    list->node_count = 0;
    list->head = NULL;
    return list;
}

int html_nodelist_append(HTMLNodeList* list, HTMLElement* element) {
    /* clear previous errors */
    html2tex_err_clear();

    if (!list) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Cannot append to NULL HTMLNodeList.");
        return 0;
    }

    if (!element) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Cannot append NULL HTMLElement.");
        return 0;
    }

    HTMLElementList* new_node = element_list_node_create(element);
    if (!new_node) return 0;

    /* O(1) append using tail pointer */
    *list->tail = new_node;
    list->tail = &new_node->next;
    list->node_count++;
    return 1;
}

int html_nodelist_append_move(HTMLNodeList* list, HTMLElement** element) {
    /* clear the errors */
    html2tex_err_clear();

    if (!element || !*element) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Cannot move NULL HTMLElement pointer.");
        return 0;
    }

    int result = html_nodelist_append(list, *element);
    if (result) *element = NULL;
    return result;
}

int html_nodelist_extend(HTMLNodeList* dest, HTMLNodeList* src) {
    /* clear the errors */
    html2tex_err_clear();

    if (!dest || !src) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Cannot extend with NULL list.");
        return 0;
    }

    /* source is empty */
    if (!src->head)
        return 1;

    if (!dest->head) {
        dest->head = src->head;
        dest->tail = src->tail;
    }
    else {
        *dest->tail = src->head;
        dest->tail = src->tail;
    }

    dest->node_count += src->node_count;
    src->head = NULL;
    src->tail = &src->head;
    src->node_count = 0;
    return 1;
}

HTMLElement* html_nodelist_at(const HTMLNodeList* list, size_t index) {
    if (!list || index >= list->node_count)
        return NULL;

    /* optimized for sequential access starting from head */
    HTMLElementList* current = list->head;

    for (size_t i = 0; i < index && current; i++)
        current = current->next;

    return current ? current->element : NULL;
}

size_t html_nodelist_size(const HTMLNodeList* list) {
    return list ? list->node_count : 0;
}

int html_nodelist_empty(const HTMLNodeList* list) {
    return !list || !list->head || list->node_count == 0;
}

void html_nodelist_destroy(HTMLNodeList** list) {
    if (!list || !*list)
        return;

    HTMLElementList* current = (*list)->head;
    HTMLElementList* next = NULL;

    /* destroy all elements and list nodes */
    while (current) {
        next = current->next;

        if (current->element)
            html2tex_element_destroy(current->element);

        free(current);
        current = next;
    }

    /* free the list structure */
    free(*list);
    *list = NULL;
}

HTMLElement** html_nodelist_dismantle(HTMLNodeList** list) {
    /* clear the errors */
    html2tex_err_clear();

    if (!list || !*list || !(*list)->head)
        return NULL;

    /* allocate array for elements */
    HTMLNodeList* lst = *list;
    HTMLElement** array = (HTMLElement**)malloc(
        lst->node_count * sizeof(HTMLElement*));

    if (!array) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate element array"
            " for list dismantling.");
        return NULL;
    }

    /* extract elements and free list nodes */
    HTMLElementList* current = lst->head;
    HTMLElementList* next = NULL;
    size_t i = 0;

    while (current) {
        next = current->next;
        array[i++] = current->element;
        free(current);
        current = next;
    }

    /* reset list to empty state */
    lst->head = NULL;
    lst->tail = &lst->head;
    lst->node_count = 0;
    return array;
}

HTMLElement* html2tex_search_tree(const HTMLNode* root, DOMTreeVisitor predicate, const void* data, const CSSProperties* inherited_props) {
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
        html2tex_element_destroy(result);
        result = NULL;
    }

    return result;
}

void html2tex_element_destroy(HTMLElement* elem) {
    if (elem) {
        if (elem->css_props)
            css_properties_destroy(elem->css_props);
        free(elem);
    }
}