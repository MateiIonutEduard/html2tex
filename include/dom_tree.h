#ifndef DOM_TREE_H
#define DOM_TREE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct HTMLNode HTMLNode;
	typedef struct HTMLAttribute HTMLAttribute;
	typedef struct LaTeXConverter LaTeXConverter;

	/* HTML node structure */
	struct HTMLNode {
		char* tag;
		char* content;
		HTMLAttribute* attributes;
		HTMLNode* children;
		HTMLNode* next;
		HTMLNode* parent;
	};

	/* HTML attribute structure */
	struct HTMLAttribute {
		char* key;
		char* value;
		HTMLAttribute* next;
	};

	/* @brief Parse the virtual DOM tree without optimizations. */
	HTMLNode* html2tex_parse(const char* html);

	/* @brief Extracts the HTML document title using Breadth-First Search (BFS). */
	char* html2tex_extract_title(HTMLNode* root);

	/* @brief Parse HTML and return a minified DOM tree. */
	HTMLNode* html2tex_parse_minified(const char* html);

	/* @brief Creates a new instance from the input DOM tree. */
	HTMLNode* dom_tree_copy(HTMLNode* node);

	/* @brief Frees the memory for the HTMLNode* instance. */
	void html2tex_free_node(HTMLNode* node);

	/* @brief Return the DOM tree after minification. */
	HTMLNode* html2tex_minify_html(HTMLNode* root);

	/* @brief Writes formatted HTML to the output file. */
	int write_pretty_html(HTMLNode* root, const char* filename);

	/* @brief Returns the HTML as a formatted string. */
	char* get_pretty_html(HTMLNode* root);

	/* @brief Checks if an element is block-level. */
	int is_block_element(const char* tag_name);

	/* @brief Checks if an element is inline. */
	int is_inline_element(const char* tag_name);

	/* @brief Determines if an HTML element is a self-closing element per HTML5 spec. */
	int is_void_element(const char* tag_name);

	/* @brief Determines if an HTML element is an essential self-closing element per HTML5 spec. */
	int is_essential_element(const char* tag_name);

	/* @brief Determine if an HTML tag should be excluded from LaTeX conversion. */
	int should_exclude_tag(const char* tag_name);

	/* @brief Check if string contains only whitespace. */
	int is_whitespace_only(const char* text);

	/* @brief Check whether the given table element contains nested tables. */
	int should_skip_nested_table(HTMLNode* node);

	/* @brief Check if an HTML node is inside a table element. */
	int is_inside_table(HTMLNode* node);

	/* @brief Check whether the HTML element is a table containing only images. */
	int table_contains_only_images(HTMLNode* node);

	/* @brief Removes whitespace between HTML tags while preserving text content. */
	char* html2tex_compress_html(const char* html);

	/* @brief Retrieve attribute value from linked list with case-insensitive matching. */
	const char* get_attribute(HTMLAttribute* attrs, const char* key);

	/* @brief Detect if we're inside a table cell by checking parent hierarchy. */
	int is_inside_table_cell(LaTeXConverter* converter, HTMLNode* node);

	/* @brief Convert a table containing only img nodes by parsing the DOM tree. */
	void convert_image_table(LaTeXConverter* converter, HTMLNode* node);

#ifndef HTML_TITLE_MAX_SIZE
#define HTML_TITLE_MAX_SIZE 256
#endif
#ifdef __cplusplus
}
#endif

#endif