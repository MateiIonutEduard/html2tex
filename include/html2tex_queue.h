#ifndef HTML2TEX_QUEUE
#define HTML2TEX_QUEUE

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct Queue Queue;

	struct Queue {
		void* data;
		struct Queue* next;
	};

	/* @brief Adds an HTML node to the rear of the queue for breadth-first traversal. */
	int queue_enqueue(Queue** front, Queue** rear, void* data);

	/* @brief Removes and returns the HTML node from the front of the queue. */
	void* queue_dequeue(Queue** front, Queue** rear);

	/* @brief Checks if queue is empty. */
	int queue_is_empty(const Queue* front);

	/* @brief Returns the queue size. */
	size_t queue_size(const Queue* front);

	/* @brief Returns front element without removal. */
	void* queue_peek_front(const Queue* front);

	/* @brief Destroys queue and optionally frees data. */
	void queue_destroy(Queue** front, Queue** rear, void (*cleanup)(void*));

	/* @brief Recursively frees all queue structures and all HTML DOM nodes contained within the queue. */
	void queue_cleanup(Queue** front, Queue** rear);
#ifdef __cplusplus
}
#endif

#endif