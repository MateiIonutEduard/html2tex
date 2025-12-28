#ifndef CSS_PROPERTIES_H
#define CSS_PROPERTIES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct CSSProperty CSSProperty;
	typedef enum CSSPropertyMask CSSPropertyMask;
	typedef struct CSSPropertyDef CSSPropertyDef;
	typedef struct CSSProperties CSSProperties;
	typedef struct LaTeXConverter LaTeXConverter;

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

	/* Converts a CSS length to LaTeX points. */
	int css_length_to_pt(const char* length_str);

	/* Converts a CSS color to hexadecimal format. */
	char* css_color_to_hex(const char* color_value);

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
	CSSProperties* css_properties_merge(const CSSProperties* parent, const CSSProperties* child);

	/* Apply CSS properties to the current LaTeX conversion context.*/
	void css_properties_apply(LaTeXConverter* converter, const CSSProperties* props, const char* tag_name);

	/* Finalize CSS application and reset the converter state. */
	void css_properties_end(LaTeXConverter* converter, const CSSProperties* props, const char* tag_name);

	/* Create a deep copy of the CSSProperties structure. */
	CSSProperties* css_properties_copy(const CSSProperties* src);

	/* Parses inline CSS from style. */
	CSSProperties* parse_css_style(const char* style_str);

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

#ifdef __cplusplus
}
#endif

#endif