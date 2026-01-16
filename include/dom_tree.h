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

	/**
	 * @brief Parses HTML into a DOM tree with basic tag/attribute extraction.
	 * @param html HTML source string (UTF-8, NULL-terminated)
	 * @return Success: Root DOM node with children (caller owns)
	 * @return Failure: NULL with error set
	 */
	HTMLNode* html2tex_parse(const char* html);

	/**
	 * @brief Extracts document title from DOM using BFS.
	 * @param root DOM tree root (typically from html2tex_parse())
	 * @return Success: Title string (caller must free)
	 * @return Not found: NULL (no error set)
	 * @return Error: NULL with error set
	 */
	char* html2tex_extract_title(HTMLNode* root);

	/**
	 * @brief Parses and minifies HTML in a single pass.
	 * @param html HTML source to parse and minify
	 * @return Success: Minified DOM tree (caller owns)
	 * @return Failure: NULL with error set
	 */
	HTMLNode* html2tex_parse_minified(const char* html);

	/**
	 * @brief Creates deep copy of DOM subtree.
	 * @param node Root node to copy (NULL returns NULL)
	 * @return Success: Independent DOM copy (caller owns)
	 * @return Failure: NULL with error set
	 */
	HTMLNode* dom_tree_copy(HTMLNode* node);

	/**
	 * @brief Recursively frees DOM tree and all contained data.
	 * @param node Root node to free (NULL-safe)
	 */
	void html2tex_free_node(HTMLNode* node);

	/**
	 * @brief Minifies already-parsed DOM tree.
	 * @param root Existing DOM tree to minify
	 * @return Success: New minified tree (caller owns)
	 * @return Failure: NULL with error set
	 */
	HTMLNode* html2tex_minify_html(HTMLNode* root);

	/**
	 * @brief Writes formatted HTML to file with proper indentation.
	 * @param root DOM tree to serialize
	 * @param filename Output file path
	 * @return Success: 1
	 * @return Failure: 0 (check html2tex_has_error())
	 */
	int write_pretty_html(HTMLNode* root, const char* filename);

	/**
	 * @brief Returns DOM tree as formatted HTML string.
	 * @param root DOM tree to serialize
	 * @return Success: Formatted HTML (caller must free)
	 * @return Failure: NULL with error set
	 */
	char* get_pretty_html(HTMLNode* root);

	/**
	 * @brief Determines if HTML element is block-level.
	 * @param tag_name Lowercase tag name (e.g., "div", "p")
	 * @return 1: Block-level element
	 * @return 0: Not block-level
	 */
	int is_block_element(const char* tag_name);

	/**
	 * @brief Determines if HTML element is inline.
	 * @param tag_name Lowercase tag name
	 * @return 1: Inline element
	 * @return 0: Not inline
	 */
	int is_inline_element(const char* tag_name);

	/**
	 * @brief Identifies HTML5 void (self-closing) elements.
	 * @param tag_name Lowercase tag name
	 * @return 1: Void element
	 * @return 0: Not void
	 */
	int is_void_element(const char* tag_name);

	/**
	 * @brief Identifies essential void elements that must be preserved.
	 * @param tag_name Lowercase tag name
	 * @return 1: Essential void element
	 * @return 0: Not essential
	 */
	int is_essential_element(const char* tag_name);

	/**
	 * @brief Determines if tag should be excluded from LaTeX conversion.
	 * @param tag_name Lowercase tag name
	 * @return 1: Exclude from conversion
	 * @return 0: Include in conversion
	 */
	int should_exclude_tag(const char* tag_name);

	/**
	 * @brief Checks if string contains only whitespace characters.
	 * @param text String to check (NULL-safe)
	 * @return 1: Only whitespace (or NULL/empty)
	 * @return 0: Contains non-whitespace
	 */
	int is_whitespace_only(const char* text);

	/**
	 * @brief Detects nested tables that should be skipped in conversion.
	 * @param node Table element to check
	 * @return 1: Contains nested table (should skip)
	 * @return 0: No nested table
	 * @return -1: Error (check html2tex_has_error())
	 */
	int should_skip_nested_table(HTMLNode* node);

	/**
	 * @brief Checks if node is within a table context.
	 * @param node HTML node to check
	 * @return 1: Inside table hierarchy
	 * @return 0: Not inside table
	 * @return -1: Error
	 */
	int is_inside_table(const HTMLNode* node);

	/**
	 * @brief Determines if table contains only <img> elements.
	 * @param node Table element (<table> tag required)
	 * @return 1: Only images (and table structure elements)
	 * @return 0: Contains other content
	 * @return -1: Error or not a table
	 */
	int table_contains_only_images(const HTMLNode* node);

	/**
	 * @brief Compresses HTML by removing unnecessary whitespace.
	 * @param html Raw HTML source code
	 * @return Success: Compressed HTML (caller must free)
	 * @return Failure: NULL with error set
	 */
	char* html2tex_compress_html(const char* html);

	/**
	 * @brief Retrieves attribute value with case-insensitive lookup.
	 * @param attrs Attribute linked list head
	 * @param key Attribute name to find
	 * @return Found: Attribute value (do not free)
	 * @return Not found: NULL
	 * @return NULL with error set
	 */
	const char* get_attribute(HTMLAttribute* attrs, const char* key);

	/**
	 * @brief Detects if processing is inside table cell (td/th).
	 * @param converter Conversion context (may have state)
	 * @param node Current node being processed
	 * @return 1: Inside table cell
	 * @return 0: Not inside table cell
	 * @return -1: Error
	 */
	int is_inside_table_cell(LaTeXConverter* converter, const HTMLNode* node);

	/**
	 * @brief Converts image-only table to LaTeX figure with grid layout.
	 * @param converter Active conversion context
	 * @param node Table element containing only images
	 */
	void convert_image_table(LaTeXConverter* converter, const HTMLNode* node);

#ifndef HTML_TITLE_MAX_SIZE
#define HTML_TITLE_MAX_SIZE 256
#endif
#ifdef __cplusplus
}
#endif

#endif