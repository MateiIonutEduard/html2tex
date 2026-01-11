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

	/**
	 * @brief Adds element to back of queue.
	 * @param front Pointer to front queue pointer
	 * @param rear Pointer to rear queue pointer
	 * @param data User data (ownership transferred)
	 */
	int queue_enqueue(Queue** front, Queue** rear, void* data);

	/**
	 * @brief Removes and returns element from front of queue.
	 * @param front Pointer to front queue pointer
	 * @param Pointer to rear queue pointer
	 */
	void* queue_dequeue(Queue** front, Queue** rear);

	/**
	 * @brief Checks if queue is empty.
	 * @param front Pointer to front queue pointer
	 */
	int queue_is_empty(const Queue* front);

	/**
	 * @brief Returns the queue size.
	 * @param front Pointer to front queue pointer
	 */
	size_t queue_size(const Queue* front);

	/**
	 * @brief Returns front element without removal.
	 * @param front Pointer to front queue pointer
	 */
	void* queue_peek_front(const Queue* front);

	/**
	 * @brief Destroys queue and optionally frees data.
	 * @param front Pointer to front queue pointer
	 * @param rear Pointer to rear queue pointer
	 * @param cleanup Optional callback to free element data
	 */
	void queue_destroy(Queue** front, Queue** rear, void (*cleanup)(void*));

	/**
	 * @brief Recursively frees all queue structures and all HTML DOM nodes contained within the queue.
	 * @param front Pointer to front queue pointer (double pointer, set to NULL)
	 * @param rear Pointer to rear queue pointer (double pointer, set to NULL)
	 */
	void queue_cleanup(Queue** front, Queue** rear);
#ifdef __cplusplus
}
#endif

#endif