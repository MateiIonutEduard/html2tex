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

	/* @brief Destroys stack and optionally frees contained data. */
	void stack_destroy(Stack** top, void (*cleanup)(void*));

#ifdef __cplusplus
}
#endif

#endif