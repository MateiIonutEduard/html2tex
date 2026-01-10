#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct Stack Stack;
	typedef struct Queue Queue;

	struct Queue {
		void* data;
		struct Queue* next;
	};

	struct Stack {
		void* data;
		struct Stack* next;
	};

	/* @brief Safely traverses the stack for inspection. */
	typedef int (*StackTraverseFunc)(void* data, void* user_data);

	/* @brief Pushes a data element onto the stack. */
	int stack_push(Stack** top, void* data);

	/* @brief Pops and returns the top element from the stack. */
	void* stack_pop(Stack** top);

	/* @brief Recursively frees all stack nodes and resets the stack to empty. */
	void stack_cleanup(Stack** top);

	/* @brief Returns number of elements in stack. */
	size_t stack_size(const Stack* top);

	/* @brief Tests whether the stack is empty. */
	int stack_is_empty(const Stack* top);

	/* @brief Returns the top element without removing it from the stack. */
	void* stack_peek(const Stack* top);

	/* @brief Iterates over stack elements from top to bottom. */
	int stack_traverse(Stack* top, StackTraverseFunc predicate, void* user_data);

	/* @brief Adds an HTML node to the rear of the queue for breadth-first traversal. */
	int queue_enqueue(Queue** front, Queue** rear, void* data);

	/* @brief Removes and returns the HTML node from the front of the queue. */
	void* queue_dequeue(Queue** front, Queue** rear);

	/* @brief Recursively frees all queue structures and all HTML DOM nodes contained within the queue. */
	void queue_cleanup(Queue** front, Queue** rear);

#ifdef __cplusplus
}
#endif

#endif