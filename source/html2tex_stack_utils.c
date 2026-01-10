#include "data_structures.h"
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
    Stack* current = *top;

    while (current) {
        Stack* next = current->next;
        free(current);
        current = next;
    }

    *top = NULL;
}

int stack_is_empty(Stack* top) {
    return top == NULL;
}

void* stack_peek(Stack* top) {
    return top ? top->data : NULL;
}