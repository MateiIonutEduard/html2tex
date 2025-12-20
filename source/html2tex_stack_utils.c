#include "html2tex.h"
#include <stdlib.h>

int stack_push(Stack** top, void* data) {
    Stack* node = (Stack*)malloc(sizeof(Stack));
    if (!node) return 0;

    node->data = data;
    node->next = *top;
    *top = node;

    return 1;
}

void* stack_pop(Stack** top) {
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