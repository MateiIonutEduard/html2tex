#ifndef HTML2TEX_H
#define HTML2TEX_H

#include <stddef.h>
#include "string_buffer.h"
#include "dom_tree.h"
#ifdef __cplusplus
extern "C" {
#endif

	typedef struct ConverterState ConverterState;
	typedef struct LaTeXConverter LaTeXConverter;

	typedef struct CSSProperty CSSProperty;
	typedef enum CSSPropertyMask CSSPropertyMask;
	typedef struct CSSPropertyDef CSSPropertyDef;

	typedef struct CSSProperties CSSProperties;
	typedef struct HTMLElement HTMLElement;

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

	/* CSS property bitmask for fast presence checking.
	   Each bit represents whether a specific property type is applied.
	 */
	enum CSSPropertyMask {
		CSS_BOLD = 1 << 0,
		CSS_ITALIC = 1 << 1,
		CSS_UNDERLINE = 1 << 2,
		CSS_COLOR = 1 << 3,
		CSS_BACKGROUND = 1 << 4,
		CSS_FONT_FAMILY = 1 << 5,
		CSS_FONT_SIZE = 1 << 6,
		CSS_TEXT_ALIGN = 1 << 7,
		CSS_BORDER = 1 << 8,
		CSS_MARGIN_LEFT = 1 << 9,
		CSS_MARGIN_RIGHT = 1 << 10,
		CSS_MARGIN_TOP = 1 << 11,
		CSS_MARGIN_BOTTOM = 1 << 12
	};

	/* Compact CSS property storage using key-value pairs.
	   Only stores properties that are actually set.
	 */
	struct CSSProperty {
		const char* key;
		char* value;
		unsigned int important : 1;
		struct CSSProperty* next;
	};

	struct CSSProperties {
		CSSProperty* head;
		CSSProperty* tail;
		size_t count;
		CSSPropertyMask mask;
	};

	struct CSSPropertyDef {
		const char* key;
		unsigned int index;
		unsigned char inheritable;
		unsigned char has_length;
		unsigned char has_color;
	};

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
		char* output;
		size_t output_size;

		size_t output_capacity;
		ConverterState state;
		CSSProperties* current_css;

		int error_code;
		char error_message[256];

		char* image_output_dir;
		int download_images;
		int image_counter;
	};

	/* Creates a new LaTeXConverter* and allocates memory. */
	LaTeXConverter* html2tex_create(void);

	/* Returns a copy of the LaTeXConverter* object. */
	LaTeXConverter* html2tex_copy(LaTeXConverter*);

	/* Frees a LaTeXConverter* structure. */
	void html2tex_destroy(LaTeXConverter* converter);

	/* Parses input HTML and converts it to LaTeX. */
	char* html2tex_convert(LaTeXConverter* converter, const char* html);

	/* Returns the error code from the HTML-to-LaTeX conversion. */
	int html2tex_get_error(const LaTeXConverter* converter);

	/* Returns the error message from the HTML-to-LaTeX conversion. */
	const char* html2tex_get_error_message(const LaTeXConverter* converter);

	/* Append a string to the LaTeX output buffer with optimized copying. */
	void append_string(LaTeXConverter* converter, const char* str);

	/* Recursively converts a DOM child node to LaTeX. */
	void convert_children(LaTeXConverter* converter, HTMLNode* node, CSSProperties* inherited_props);

	/* Sets the download output directory. */
	void html2tex_set_image_directory(LaTeXConverter* converter, const char* dir);

	/* Toggles image downloading according to the enable flag. */
	void html2tex_set_download_images(LaTeXConverter* converter, int enable);

	/* Downloads an image from the specified URL. */
	char* download_image_src(const char* src, const char* output_dir, int image_counter);

	/* Returns whether src contains a base64-encoded image. */
	int is_base64_image(const char* src);

	/* Initializes download processing. */
	int image_utils_init(void);

	/* Frees image-download resources. */
	void image_utils_cleanup(void);

	/* Converts a CSS length to LaTeX points. */
	int css_length_to_pt(const char* length_str);

	/* Converts a CSS color to hexadecimal format. */
	char* css_color_to_hex(const char* color_value);

	/* Returns a null-terminated duplicate of the string referenced by str. */
	char* html2tex_strdup(const char* str);

	/* Convert an integer to a null-terminated string using the given radix and store it in buffer. */
	void portable_itoa(int value, char* buffer, int radix);

	/* Pushes a data element onto the stack. */
	int stack_push(Stack** top, void* data);

	/* Pops and returns the top element from the stack. */
	void* stack_pop(Stack** top);

	/* Recursively frees all stack nodes and resets the stack to empty. */
	void stack_cleanup(Stack** top);

	/* Tests whether the stack is empty. */
	int stack_is_empty(Stack* top);

	/* Returns the top element without removing it from the stack. */
	void* stack_peek(Stack* top);

	/* Adds an HTML node to the rear of the queue for breadth-first traversal. */
	int queue_enqueue(Queue** front, Queue** rear, void* data);

	/* Removes and returns the HTML node from the front of the queue. */
	void* queue_dequeue(Queue** front, Queue** rear);

	/* Recursively frees all queue structures and all HTML DOM nodes contained within the queue. */
	void queue_cleanup(Queue** front, Queue** rear);

	/* Process an image node within table context for LaTeX generation. */
	void process_table_image(LaTeXConverter* converter, HTMLNode* img_node);

	/* Generate LaTeX figure caption for a table containing images. */
	void append_figure_caption(LaTeXConverter* converter, HTMLNode* table_node);

	/* Calculate maximum number of columns in an HTML table. */
	int count_table_columns(HTMLNode* node);

	/* Determines if a CSS property is inheritable per W3C CSS 2.1 specification. */
	int is_css_property_inheritable(const char* property_name);

	/* Allocates and initializes a new CSSProperties container. */
	CSSProperties* css_properties_create(void);

	/* Releases memory used by CSSProperties for managing styles. */
	void css_properties_destroy(CSSProperties* props);

	/* Set or update a CSS property in the properties collection. */
	int css_properties_set(CSSProperties* props, const char* key, const char* value, int important);

	/* Retrieve the value of a CSS property. */
	const char* css_properties_get(const CSSProperties* props, const char* key);

	/* Check if a CSS property exists in the collection. */
	int css_properties_has(const CSSProperties* props, const char* key);

	/* Merge two CSS property collections with inheritance rules. */
	CSSProperties* css_properties_merge(const CSSProperties* parent,
		const CSSProperties* child);

	/* Apply CSS properties to the current LaTeX conversion context.*/
	void css_properties_apply(LaTeXConverter* converter, const CSSProperties* props, const char* tag_name);

	/* Finalize CSS application and reset the converter state. */
	void css_properties_end(LaTeXConverter* converter, const CSSProperties* props, const char* tag_name);

	/* Create a deep copy of the CSSProperties structure. */
	CSSProperties* css_properties_copy(const CSSProperties* src);

	/* Parses inline CSS from style. */
	CSSProperties* parse_css_style(const char* style_str);

	/* Find first DOM node matching criteria with computed CSS. */
	HTMLElement* search_tree(HTMLNode* root, int (*predicate)(HTMLNode*, void*), void* data, CSSProperties* inherited_props);

	/* Safely deallocates an HTMLElement structure. */
	void html_element_destroy(HTMLElement* elem);

#ifndef CSS_MAX_PROPERTY_LENGTH
#define CSS_MAX_PROPERTY_LENGTH 65535
#endif

#ifndef CSS_KEY_PROPERTY_LENGTH
#define CSS_KEY_PROPERTY_LENGTH 128
#endif

#ifndef CSS_INHERITABLE_MASK
#define CSS_INHERITABLE_MASK (CSS_BOLD | CSS_ITALIC | CSS_UNDERLINE | \
                             CSS_COLOR | CSS_FONT_FAMILY | CSS_FONT_SIZE | \
                             CSS_TEXT_ALIGN)
#endif

#ifndef MAX_REASONABLE_MARGIN_LENGTH
#define MAX_REASONABLE_MARGIN_LENGTH 256
#endif

#ifndef MAX_MARGIN_TOKEN_LENGTH
#define MAX_MARGIN_TOKEN_LENGTH 32
#endif

#ifdef _MSC_VER
#define strdup html2tex_strdup
#define html2tex_itoa(value, buffer, radix) _itoa((value), (buffer), (radix))
#else
#define html2tex_itoa(value, buffer, radix) portable_itoa((value), (buffer), (radix))
#endif

#ifdef _WIN32
#define mkdir(path) _mkdir(path)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#define mkdir(path) mkdir(path, 0755)
#include <strings.h>
#endif

#ifdef __cplusplus
}
#endif

#endif