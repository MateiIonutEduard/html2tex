#include <stdlib.h>
#include "html2tex.h"

int queue_enqueue(Queue** front, Queue** rear, void* data) {
    Queue* node = (Queue*)malloc(sizeof(Queue));
    if (!node) return 0;

    node->data = data;
    node->next = NULL;

    if (*rear) (*rear)->next = node;
    else *front = node;

    *rear = node;
    return 1;
}

void* queue_dequeue(Queue** front, Queue** rear) {
    Queue* node = *front;
    if (!node) return NULL;

    void* data = node->data;
    *front = node->next;

    if (!*front)
        *rear = NULL;

    free(node);
    return data;
}

void queue_cleanup(Queue** front, Queue** rear) {
    Queue* current = *front;

    while (current) {
        Queue* next = current->next;
        free(current);
        current = next;
    }

    *front = *rear = NULL;
}