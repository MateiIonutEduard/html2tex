#include <stdlib.h>
#include "html2tex_queue.h"
#include "html2tex_errors.h"

int queue_enqueue(Queue** front, Queue** rear, void* data) {
    /* clear the error context */
    html2tex_err_clear();

    if (!front || !rear) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Queue pointers are NULL for enqueue operation.");
        return 0;
    }

    Queue* node = (Queue*)malloc(sizeof(Queue));
    
    if (!node) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate queue node (size: %zu bytes).",
            sizeof(Queue));
        return 0;
    }

    node->data = data;
    node->next = NULL;

    if (*rear) (*rear)->next = node;
    else *front = node;

    *rear = node;
    return 1;
}

void* queue_dequeue(Queue** front, Queue** rear) {
    /* clear the error context */
    html2tex_err_clear();

    if (!front || !rear) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Queue pointers are NULL for dequeue operation.");
        return NULL;
    }

    if (!*front) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Cannot dequeue from empty queue.");
        return NULL;
    }

    Queue* node = *front;
    if (!node) return NULL;

    void* data = node->data;
    *front = node->next;

    if (!*front)
        *rear = NULL;

    free(node);
    return data;
}

int queue_is_empty(const Queue* front) {
    return front == NULL;
}

size_t queue_size(const Queue* front) {
    size_t count = 0;
    const Queue* current = front;

    while (current) {
        count++;
        current = current->next;
    }

    return count;
}

void* queue_peek_front(const Queue* front) {
    /* clear the errors */
    html2tex_err_clear();

    if (!front) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Cannot peek from empty queue.");
        return NULL;
    }

    return front->data;
}

void queue_destroy(Queue** front, Queue** rear, void (*cleanup)(void*)) {
    /* clear the errors context */
    html2tex_err_clear();

    if (!front || !rear) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Queue pointers are NULL for destruction.");
        return;
    }

    Queue* current = *front;
    while (current) {
        Queue* next = current->next;

        /* release memory for data container, if the callback is provided */
        if (cleanup && current->data)
            cleanup(current->data);

        free(current);
        current = next;
    }

    *front = NULL;
    *rear = NULL;
}

void queue_cleanup(Queue** front, Queue** rear) {
    /* clear the errors context first */
    html2tex_err_clear();

    if (!front || !rear) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Queue pointers are NULL for cleanup operation.");
        return;
    }

    /* use existing queue destroy function, to keep backward compatibility */
    queue_destroy(front, rear, NULL);
}