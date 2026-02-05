#include "html2tex_stack.h"
#include "html2tex_errors.h"
#include <stdlib.h>

int stack_push(Stack** top, void* data) {
    html2tex_err_clear();

    if (!top) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Stack double pointer is NULL "
            "for push operation.");
        return 0;
    }

    Stack* node = (Stack*)malloc(sizeof(Stack));

    if (!node) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate stack node "
            "(size: %zu bytes).",
            sizeof(Stack));
        return 0;
    }

    node->data = data;
    node->next = *top;
    *top = node;

    return 1;
}

void** stack_to_array(Stack** top, size_t* count) {
    /* clear the error context */
    html2tex_err_clear();

    /* validate input parameters */
    if (!top) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Stack double pointer is NULL"
            " for array conversion.");
        return NULL;
    }

    if (!count) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Count pointer is NULL for array"
            " conversion.");
        return NULL;
    }

    /* initialize output count */
    size_t element_count = stack_size(*top);
    if (!element_count) return NULL;
    
    /* allocate array with exact size */
    void** array = (void**)malloc(element_count * sizeof(void*));

    if (!array) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate array of %zu"
            " elements for stack conversion.",
            element_count);
        return NULL;
    }

    /* get a reference to the top stack */
    const Stack* current = *top;

    /* pop elements and fill array */
    size_t index = 0;

    while (current) {
        void* data = current->data;

        if (data) {
            array[element_count - index - 1] = data;
            index++;
        }

        current = current->next;
    }

    /* destroy empty stack nodes (transfer ownership to array) */
    stack_cleanup(top);

    *count = element_count;
    return array;
}

void* stack_pop(Stack** top) {
    /* clear the error context */
    html2tex_err_clear();

    if (!top) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Stack double pointer is NULL "
            "for pop operation.");
        return NULL;
    }

    Stack* node = *top;
    if (!node) return NULL;

    void* data = node->data;
    *top = node->next;
    free(node);

    return data;
}

void stack_cleanup(Stack** top) {
    /* clear the errors */
    html2tex_err_clear();

    if (!top) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Stack double pointer is NULL "
            "for clear operation.");
        return;
    }

    /* destroy stack containers without releasing data */
    stack_destroy(top, NULL);
}

void stack_destroy(Stack** top, void (*cleanup)(void*)) {
    /* clear errors and their context */
    html2tex_err_clear();

    if (!top) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Stack double pointer is NULL for destruction.");
        return;
    }

    Stack* current = *top;

    while (current) {
        Stack* next = current->next;

        /* optionally free the data if the callback provided */
        if (cleanup && current->data)
            cleanup(current->data);

        free(current);
        current = next;
    }

    *top = NULL;
}

size_t stack_size(const Stack* top) {
    size_t count = 0;
    const Stack* current = top;

    while (current) {
        count++;
        current = current->next;
    }

    return count;
}

int stack_is_empty(const Stack* top) {
    return top == NULL;
}

void* stack_peek(const Stack* top) {
    return top ? top->data : NULL;
}

int stack_traverse(Stack* top, StackTraverseFunc predicate, void* user_data) {
    html2tex_err_clear();

    if (!top) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Stack double pointer is NULL "
            "for node traversal.");
        return 0;
    }

    if (!predicate) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Traversal function is NULL.");
        return 0;
    }

    Stack* current = top;

    while (current) {
        int result = predicate(current->data, user_data);
        if (result != 0) return result;
        current = current->next;
    }

    return 1;
}