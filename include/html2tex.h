#ifndef HTML2TEX_H
#define HTML2TEX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct HTMLNode HTMLNode;
	typedef struct HTMLAttribute HTMLAttribute;

	typedef struct ConverterState ConverterState;
	typedef struct LaTeXConverter LaTeXConverter;

	typedef struct CSSProperty CSSProperty;
	typedef enum CSSPropertyMask CSSPropertyMask;
	typedef struct CSSPropertyDef CSSPropertyDef;

	typedef struct CSSProperties CSSProperties;
	typedef struct Stack Stack;
	typedef struct Queue Queue;

	/* HTML node structure */
	struct HTMLNode {
		char* tag;
		char* content;
		HTMLAttribute* attributes;
		HTMLNode* children;
		HTMLNode* next;
		HTMLNode* parent;
	};

	struct Queue {
		void* data;
		struct Queue* next;
	};

	struct Stack {
		void* data;
		struct Stack* next;
	};

	/* HTML attribute structure */
	struct HTMLAttribute {
		char* key;
		char* value;
		HTMLAttribute* next;
	};

	/* CSS property bitmask for fast presence checking.
	   Each bit represents whether a specific property type is applied.
	 */
	enum CSSPropertyMask {
		CSS_BOLD = 1,
		CSS_ITALIC = 2,
		CSS_UNDERLINE = 4,
		CSS_COLOR = 8,
		CSS_BACKGROUND = 16,
		CSS_FONT_FAMILY = 32,
		CSS_FONT_SIZE = 64,
		CSS_TEXT_ALIGN = 128,
		CSS_BORDER = 256
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

		int pending_margin_bottom : 16;
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

	/* Parse the virtual DOM tree without optimizations. */
	HTMLNode* html2tex_parse(const char* html);

	/* Extracts the HTML document title using Breadth-First Search (BFS). */
	char* html2tex_extract_title(HTMLNode* root);

	/* Parse HTML and return a minified DOM tree. */
	HTMLNode* html2tex_parse_minified(const char* html);

	/* Creates a new instance from the input DOM tree. */
	HTMLNode* dom_tree_copy(HTMLNode* node);

	/* Frees the memory for the HTMLNode* instance. */
	void html2tex_free_node(HTMLNode* node);

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

	/* Return the DOM tree after minification. */
	HTMLNode* html2tex_minify_html(HTMLNode* root);

	/* Writes formatted HTML to the output file. */
	int write_pretty_html(HTMLNode* root, const char* filename);

	/* Returns the HTML as a formatted string. */
	char* get_pretty_html(HTMLNode* root);

	/* Converts a CSS length to LaTeX points. */
	int css_length_to_pt(const char* length_str);

	/* Converts a CSS color to hexadecimal format. */
	char* css_color_to_hex(const char* color_value);

	/* Checks if an element is block-level. */
	int is_block_element(const char* tag_name);

	/* Checks if an element is inline. */
	int is_inline_element(const char* tag_name);

	/* Determines if an HTML element is a self-closing element per HTML5 spec. */
	int is_void_element(const char* tag_name);

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

	/* Determine if an HTML tag should be excluded from LaTeX conversion. */
	int should_exclude_tag(const char* tag_name);

	/* Check if string contains only whitespace. */
	int is_whitespace_only(const char* text);

	/* Check whether the given table element contains nested tables. */
	int should_skip_nested_table(HTMLNode* node);

	/* Detect if we're inside a table cell by checking parent hierarchy. */
	int is_inside_table_cell(LaTeXConverter* converter, HTMLNode* node);

	/* Convert a table containing only img nodes by parsing the DOM tree. */
	void convert_image_table(LaTeXConverter* converter, HTMLNode* node);

	/* Check if an HTML node is inside a table element. */
	int is_inside_table(HTMLNode* node);

	/* Check whether the HTML element is a table containing only images. */
	int table_contains_only_images(HTMLNode* node);

	/* Process an image node within table context for LaTeX generation. */
	void process_table_image(LaTeXConverter* converter, HTMLNode* img_node);

	/* Generate LaTeX figure caption for a table containing images. */
	void append_figure_caption(LaTeXConverter* converter, HTMLNode* table_node);

	/* Retrieve attribute value from linked list with case-insensitive matching. */
	const char* get_attribute(HTMLAttribute* attrs, const char* key);

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