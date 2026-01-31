#ifndef DOM_TREE_VISITOR_H
#define DOM_TREE_VISITOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct HTMLElement HTMLElement;
	typedef struct CSSProperties CSSProperties;

	typedef struct HTMLNode HTMLNode;
	typedef int (*DOMTreeVisitor)(const HTMLNode*, const void*);

	struct HTMLElement {
		HTMLNode* node;
		CSSProperties* css_props;
	};

	/**
	 * @brief Finds first DOM node matching predicate with computed CSS inheritance.
	 * @param root Starting node for DFS search
	 * @param predicate Matching function (returns non-zero for match)
	 * @param data User context passed to predicate
	 * @param inherited_props Base CSS properties for inheritance chain
	 * @return Success: HTMLElement with node + computed CSS (caller owns)
	 * @return Failure: NULL with error set
	 */
	HTMLElement* html2tex_search_tree(const HTMLNode* root, DOMTreeVisitor predicate, const void* data, const CSSProperties* inherited_props);

	/**
	 * @brief Safely deallocates HTMLElement structure from html2tex_search_tree().
	 * @param elem Element to destroy (NULL-safe)
	 */
	void html2tex_element_destroy(HTMLElement* elem);

#ifdef __cplusplus
}
#endif

#endif