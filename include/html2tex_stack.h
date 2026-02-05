#ifndef HTML2TEX_STACK
#define HTML2TEX_STACK

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct Stack Stack;

	struct Stack {
		void* data;
		struct Stack* next;
	};

	/* @brief Safely traverses the stack for inspection. */
	typedef int (*StackTraverseFunc)(void* data, void* user_data);

	/**
	 * @brief Pushes a data element onto the stack.
	 * @param top Pointer to stack pointer (double pointer required for empty stack case)
	 * @param data User data to store (ownership transferred to stack)
	 * @return 1: Success
	 * @return 0: Failure (check error state)
	 */
	int stack_push(Stack** top, void* data);

	/**
	 * @brief Removes and returns the top element from the stack.
	 * @param top Pointer to stack pointer
	 * @return Non-NULL - Successfully popped data (ownership transferred to caller)
	 * @return NULL - Stack was empty (not an error condition)
	 * @return NULL with error set - Invalid parameters
	 */
	void* stack_pop(Stack** top);

	/**
	 * @brief Clears all elements from stack while preserving stack structure.
	 * @param top top Pointer to stack pointer (set to NULL after destruction)
	 */
	void stack_cleanup(Stack** top);

	/**
	 * @brief Returns number of elements in stack.
	 * @param top Stack pointer
	 * @return Number of elements (0 for empty/NULL stack)
	 */
	size_t stack_size(const Stack* top);

	/**
	 * @brief Checks if stack contains no elements.
	 * @param top Stack pointer
	 * @return 1 - Stack is empty or NULL
	 * @return 0 - Stack contains elements
	 */
	int stack_is_empty(const Stack* top);

	/**
	 * @brief Returns the top element without removing it.
	 * @param top Stack pointer (single pointer since no modification)
	 * @return Non-NULL - Top element data (stack retains ownership)
	 * @return NULL with error set - Stack is empty or NULL
	 */
	void* stack_peek(const Stack* top);

	/**
	 * @brief Iterates over stack elements from top to bottom.
	 * @param top Stack pointer
	 * @param predicate Callback invoked for each element
	 * @param user_data User context passed to callback
	 */
	int stack_traverse(Stack* top, StackTraverseFunc predicate, void* user_data);

	/**
	 * @brief Destroys stack and optionally frees contained data.
	 * @param top Pointer to stack pointer (set to NULL after destruction)
	 * @param cleanup Optional callback to free element data (NULL if data ownership lies elsewhere)
	 */
	void stack_destroy(Stack** top, void (*cleanup)(void*));

	/**
	 * @brief Atomically converts stack to array while destroying the stack.
	 *
	 * Transforms LIFO stack to FIFO array with guaranteed cleanup. All stack nodes
	 * 
	 * are freed, element ownership transfers to array. Operation is all-or-nothing:
	 * 
	 * returns valid array with destroyed stack, or NULL with destroyed stack and
	 * 
	 * error set.
	 *
	 * @param top Stack to convert/destroy. Set to NULL on completion.
	 * @param count Number of elements extracted. 0 on empty/error.
	 * @param cleanup Optional element destructor. NULL retains ownership.
	 *
	 * @return Success: malloc'd array[*count]. 
	 * @return Empty: array[1]={NULL}.
	 * @return Failure: NULL with error set.
	 */
	void** stack_to_array(Stack** top, size_t* count);

#ifdef __cplusplus
}
#endif

#endif