#ifndef HTML2TEX_H
#define HTML2TEX_H

#include <stddef.h>
#include "dom_tree.h"
#include "string_buffer.h"
#include "html2tex_stack.h"
#include "html2tex_queue.h"
#include "css_properties.h"
#include "image_utils.h"
#include "html2tex_errors.h"

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

	/* @brief Creates a new LaTeXConverter* and allocates memory. */
	LaTeXConverter* html2tex_create(void);

	/* @brief Returns a copy of the LaTeXConverter* object. */
	LaTeXConverter* html2tex_copy(LaTeXConverter*);

	/* @brief Frees a LaTeXConverter* structure. */
	void html2tex_destroy(LaTeXConverter* converter);

	/* @brief Parses input HTML and converts it to LaTeX. */
	char* html2tex_convert(LaTeXConverter* converter, const char* html);

	/* @brief Returns the error code from the HTML-to-LaTeX conversion. */
	int html2tex_get_error();

	/* @brief Returns the error message from the HTML-to-LaTeX conversion. */
	const char* html2tex_get_error_message();

	/* @brief Append a string to the LaTeX output buffer with optimized copying. */
	void append_string(LaTeXConverter* converter, const char* str);

	/* @brief Recursively converts a DOM child node to LaTeX. */
	void convert_children(LaTeXConverter* converter, HTMLNode* node, CSSProperties* inherited_props);

	/* @brief Sets the download output directory. */
	void html2tex_set_image_directory(LaTeXConverter* converter, const char* dir);

	/* @brief Toggles image downloading according to the enable flag. */
	void html2tex_set_download_images(LaTeXConverter* converter, int enable);

	/* @brief Returns a null-terminated duplicate of the string referenced by str. */
	char* html2tex_strdup(const char* str);

	/* @brief Convert an integer to a null-terminated string using the given radix and store it in buffer. */
	void portable_itoa(int value, char* buffer, int radix);

	/* @brief Process an image node within table context for LaTeX generation. */
	void process_table_image(LaTeXConverter* converter, HTMLNode* img_node);

	/* @brief Generate LaTeX figure caption for a table containing images. */
	void append_figure_caption(LaTeXConverter* converter, HTMLNode* table_node);

	/* @brief Calculate maximum number of columns in an HTML table. */
	int count_table_columns(HTMLNode* node);

	/* @brief Find first DOM node matching criteria with computed CSS. */
	HTMLElement* search_tree(HTMLNode* root, int (*predicate)(HTMLNode*, void*), void* data, CSSProperties* inherited_props);

	/* @brief Safely deallocates an HTMLElement structure. */
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