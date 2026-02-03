#ifndef DOM_TREE_VISITOR_H
#define DOM_TREE_VISITOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	/**
	 * @brief Combines a DOM node with its computed CSS properties for contextual processing.
	*/
	typedef struct HTMLElement HTMLElement;
	typedef struct CSSProperties CSSProperties;

	typedef struct HTMLNode HTMLNode;
	typedef int (*DOMTreeVisitor)(const HTMLNode*, const void*);

	/**
	 * @brief Linked list node for element storage.
	*/
	typedef struct HTMLElementList HTMLElementList;

	/**
	 * @brief Optimized list structure with O(1) append operations and size tracking.
	*/
	typedef struct HTMLNodeList HTMLNodeList;

	struct HTMLElement {
		HTMLNode* node; /* Pointer to DOM node (owned by caller) */
		CSSProperties* css_props; /* Computed CSS properties (owned by this structure) */
	};

	struct HTMLElementList {
		HTMLElement* element; /* Contained element */
		struct HTMLElementList* next; /* Next list node */
	};

	struct HTMLNodeList {
		HTMLElementList* head; /* First element in list */
		HTMLElementList** tail; /* Pointer to last node's next pointer */
		size_t node_count; /* Number of elements in list */
	};

	/**
	 * @brief Creates an empty HTMLNodeList with optimized append capabilities.
	 * @return Success: Initialized HTMLNodeList* (caller owns)
	 * @return Failure: NULL with HTML2TEX_ERR_NOMEM
	 */
	HTMLNodeList* html_nodelist_create(void);

	/**
	 * @brief Appends an HTMLElement to the list with O(1) complexity.
	 * @param list Target list (non-NULL)
	 * @param element Element to append (non-NULL, ownership transferred)
	 * @return Success: 1
	 * @return Failure: 0 with error set
	 */
	int html_nodelist_append(HTMLNodeList* list, HTMLElement* element);

	/**
	 * @brief Appends element with move semantics (clears source pointer).
	 * @param list Target list (non-NULL)
	 * @param Pointer to element pointer (non-NULL, becomes NULL on success)
	 * @return Same as html_nodelist_append()
	*/
	int html_nodelist_append_move(HTMLNodeList* list, HTMLElement** element);

	/**
	 * @brief Efficiently transfers all elements from src to dest list.
	 * @param dest Destination list (non-NULL, receives elements)
	 * @param src Source list (non-NULL, becomes empty)
	 * @return Success: 1
	 * @return Failure: 0 with HTML2TEX_ERR_NULL
	*/
	int html_nodelist_extend(HTMLNodeList* dest, HTMLNodeList* src);

	/**
	 * @brief Retrieves element at specified index with bounds checking.
	 * @param list List to query (can be NULL)
	 * @param index Zero-based position
	 * @return Success: HTMLElement* (list retains ownership)
	 * @return Failure: NULL (index out of bounds or NULL list)
	*/
	HTMLElement* html_nodelist_at(const HTMLNodeList* list, size_t index);

	/**
	 * @brief Returns number of elements in list in O(1) time.
	 * @param list List to query (can be NULL)
	 * @return Element count (0 for NULL list)
	*/
	size_t html_nodelist_size(const HTMLNodeList* list);

	/**
	 * @brief Checks if list contains no elements.
	 * @param list List to check (can be NULL)
	 * @return Non-zero if empty or NULL, zero if contains elements
	*/
	int html_nodelist_empty(const HTMLNodeList* list);

	/**
	 * @brief Completely destroys list and all contained elements.
	 * @param list Pointer to list pointer (double pointer, becomes NULL)
	*/
	void html_nodelist_destroy(HTMLNodeList** list);

	/**
	 * @brief Converts list to array and empties (but doesn't destroy) the list.
	 * @param list Pointer to list pointer (list becomes empty)
	 * @return Success: Array of HTMLElement* pointers (caller owns array and elements)
	 * @return Failure: NULL with error set
	*/
	HTMLElement** html_nodelist_dismantle(HTMLNodeList** list);

	/**
	 * @brief Performs depth-first traversal to find all DOM nodes matching a predicate.
	 * @param root Starting node for traversal (inclusive)
	 * @param predicate Callback function that evaluates whether a node matches
	 * @param data User-defined context passed to each predicate invocation
	 * @param inherited_props Base CSS properties for the inheritance chain
	 * @return Success: HTMLNodeList* containing matched elements with computed CSS
	 * @return Failure: NULL with error set
	*/
	HTMLNodeList* html2tex_find_all(const HTMLNode* root, DOMTreeVisitor predicate,
		const void* data, const CSSProperties* inherited_props);

	/**
	 * @brief Finds first DOM node matching predicate with computed CSS inheritance.
	 * @param root Starting node for DFS search
	 * @param predicate Matching function (returns non-zero for match)
	 * @param data User context passed to predicate
	 * @param inherited_props Base CSS properties for inheritance chain
	 * @return Success: HTMLElement with node + computed CSS (caller owns)
	 * @return Failure: NULL with error set
	 */
	HTMLElement* html2tex_search_tree(const HTMLNode* root, DOMTreeVisitor predicate, 
		const void* data, const CSSProperties* inherited_props);

	/**
	 * @brief Safely deallocates HTMLElement structure from html2tex_search_tree().
	 * @param elem Element to destroy (NULL-safe)
	 */
	void html2tex_element_destroy(HTMLElement* elem);

#ifdef __cplusplus
}
#endif

#endif