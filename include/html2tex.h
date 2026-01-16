#ifndef HTML2TEX_H
#define HTML2TEX_H

#include <stddef.h>
#include "dom_tree.h"
#include "image_utils.h"
#include "string_buffer.h"
#include "html2tex_stack.h"
#include "html2tex_queue.h"
#include "css_properties.h"
#include "html2tex_processor.h"
#include "html2tex_errors.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct ConverterState ConverterState;
	typedef struct LaTeXConverter LaTeXConverter;
	typedef struct HTMLElement HTMLElement;

	struct HTMLElement {
		HTMLNode* node;
		CSSProperties* css_props;
	};

	/* converter configuration */
	struct ConverterState {
		int indent_level;
		int list_level;

		int in_paragraph;
		int in_list;

		int table_internal_counter;
		int figure_internal_counter;
		int image_internal_counter;

		int in_table;
		int in_table_row;

		int in_table_cell;
		int table_columns;

		int current_column;
		char* table_caption;

		/* CSS conversion state */
		unsigned int css_braces : 8;
		unsigned int css_environments : 8;
		CSSPropertyMask applied_props;

		/* flags required for special handling */
		unsigned int skip_nested_table : 1;
		unsigned int table_has_caption : 1;
		unsigned int pending_css_reset : 1;
	};

	/* main converter structure */
	struct LaTeXConverter {
		StringBuffer* buffer;
		ConverterState state;
		CSSProperties* current_css;

		char* image_output_dir;
		int download_images;
		int image_counter;
	};

	/**
	 * @brief Initializes a new conversion context with default settings.
	 * @return Success: Valid LaTeXConverter* ready for configuration
	 * @return Failure: NULL with error set (check html2tex_has_error())
	 */
	LaTeXConverter* html2tex_create(void);

	/**
	 * @brief Creates a deep copy of converter state for branching or checkpointing.
	 * @param converter Valid converter to duplicate (non-NULL)
	 * @return Success: Independent converter copy
	 * @return Failure: NULL with error set
	 */
	LaTeXConverter* html2tex_copy(LaTeXConverter* converter);

	/**
	 * @brief Releases all converter resources and invalidates pointer.
	 * @param converter Converter to destroy (NULL-safe)
	 */
	void html2tex_destroy(LaTeXConverter* converter);

	/**
	 * @brief Converts HTML document to complete LaTeX document with preamble.
	 * @param converter Configured conversion context
	 * @param html HTML source string (UTF-8, NULL-terminated)
	 * @return Success: Complete LaTeX document (caller owns, must free())
	 * @return Failure: NULL with error set
	 */
	char* html2tex_convert(LaTeXConverter* converter, const char* html);

	/**
	 * @brief Retrieves the most recent error code from thread-local storage.
	 * @return Current HTML2TeXError enum value
	 * @return HTML2TEX_OK (0) if no error occurred
	 */
	int html2tex_get_error();

	/**
	 * @brief Returns formatted error description with context.
	 * @return Human-readable error string
	 * @return Static buffer (do not free)
	 * @return Empty string if no error
	 */
	const char* html2tex_get_error_message();

	/**
	 * @brief Appends raw text to converter's output buffer with LaTeX escaping.
	 * @param converter Active conversion context
	 * @param str String to append (NULL-safe, empty string allowed)
	 */
	void append_string(LaTeXConverter* converter, const char* str);

	/**
	 * @brief Escapes and appends text to LaTeX output with proper character escaping.
	 * @param converter Active LaTeX conversion context
	 * @param text Raw text to escape and append (UTF-8, NULL-terminated)
	*/
	void escape_latex(LaTeXConverter* converter, const char* text);

	/**
	 * @brief Escapes only critical LaTeX special characters (minimal escaping).
	 * @param converter Active LaTeX conversion context
	 * @param text Text to minimally escape (NULL-safe)
	*/
	void escape_latex_special(LaTeXConverter* converter, const char* text);

	/**
	 * @brief Converts an HTML DOM subtree to LaTeX using iterative DFS with full CSS inheritance.
	 * @param converter Active conversion context (stateful, non-NULL)
	 * @param node Root DOM node to convert (inclusive traversal)
	 */
	void convert_document(LaTeXConverter* converter, HTMLNode* node);

	/**
	 * @brief Configures output directory for downloaded images.
	 * @param converter Active conversion context
	 * @param dir Directory path (NULL or empty disables download storage)
	 */
	void html2tex_set_image_directory(LaTeXConverter* converter, const char* dir);

	/**
	 * @brief Enables or disables automatic image download and local storage.
	 * @param converter Active conversion context
	 * @param enable Non-zero to enable, zero to disable
	 */
	void html2tex_set_download_images(LaTeXConverter* converter, int enable);

	/**
	 * @brief Portable string duplication with unified error handling.
	 * @param str Source string to duplicate (NULL-safe)
	 * @return Success: New string (caller must free)
	 * @return Failure: NULL with error set
	 */
	char* html2tex_strdup(const char* str);

	/**
	 * @brief Cross-platform integer-to-string conversion.
	 * @param value Integer to convert
	 * @param buffer Output buffer (minimum 33 bytes recommended)
	 * @param radix Base (2-36, typically 10)
	 */
	void portable_itoa(int value, char* buffer, int radix);

	/**
	 * @brief Converts single image within table cell to LaTeX graphics.
	 * @param converter Active conversion context
	 * @param img_node <img> element node (must be inside table)
	 */
	void process_table_image(LaTeXConverter* converter, HTMLNode* img_node);

	void begin_table(LaTeXConverter* converter, int columns);

	void end_table(LaTeXConverter* converter, const char* table_label);

	void begin_table_row(LaTeXConverter* converter);

	void end_table_row(LaTeXConverter* converter);

	char* extract_caption_text(const HTMLNode* node);

	/**
	 * @brief Generates LaTeX caption and label for image-only tables.
	 * @param converter Active conversion context
	 * @param table_node Table containing only images
	 */
	void append_figure_caption(LaTeXConverter* converter, const HTMLNode* table_node);

	/**
	 * @brief Calculates maximum columns in HTML table structure.
	 * @param node Table root node (<table> element)
	 * @return Success: Column count (≥1)
	 * @return Failure: -1 with error set
	 */
	int count_table_columns(const HTMLNode* node);

	/**
	 * @brief Finds first DOM node matching predicate with computed CSS inheritance.
	 * @param root Starting node for BFS search
	 * @param predicate Matching function (returns non-zero for match)
	 * @param data User context passed to predicate
	 * @param inherited_props Base CSS properties for inheritance chain
	 * @return Success: HTMLElement with node + computed CSS (caller owns)
	 * @return Failure: NULL with error set
	 */
	HTMLElement* search_tree(HTMLNode* root, int (*predicate)(HTMLNode*, void*), void* data, CSSProperties* inherited_props);

	/**
	 * @brief Safely deallocates HTMLElement structure from search_tree().
	 * @param elem Element to destroy (NULL-safe)
	 */
	void html_element_destroy(HTMLElement* elem);

#ifdef _MSC_VER
#define strdup html2tex_strdup
#define html2tex_itoa(value, buffer, radix) _itoa((value), (buffer), (radix))
#else
#define html2tex_itoa(value, buffer, radix) portable_itoa((value), (buffer), (radix))
#endif

#ifdef _WIN32
#include <string.h>
#include <direct.h>
#define mkdir(path) _mkdir(path)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include <strings.h>
#include <sys/stat.h>
#define mkdir(path) mkdir(path, 0755)
#endif

#ifdef __cplusplus
}
#endif

#endif