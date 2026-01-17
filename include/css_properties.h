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

	/**
	 * @brief Converts CSS length values to LaTeX points (1pt = 1/72 inch).
	 * @param length_str CSS length value with optional unit
	 * @return Success: Equivalent points (rounded to nearest integer)
	 * @return Failure: 0 (invalid input or unsupported unit)
	 */
	int css_length_to_pt(const char* length_str);

	/**
	 * @brief Converts CSS color specification to 6-digit hexadecimal RGB.
	 * @param color_value CSS color string
	 * @return Success: 6-digit hex string (e.g., "FF8800", caller must free)
	 * @return Failure: NULL with error set
	 */
	char* css_color_to_hex(const char* color_value);

	/**
	 * @brief Determines if CSS property inherits according to W3C CSS 2.1.
	 * @param property_name CSS property name (case-insensitive)
	 * @return 1: Property is inheritable
	 * @return 0: Property is not inheritable
	 */
	int is_css_property_inheritable(const char* property_name);

	/**
	 * @brief Creates empty CSS properties container.
	 * @return Success: Empty CSSProperties* container
	 * @return Failure: NULL with error set (HTML2TEX_ERR_NOMEM)
	 */
	CSSProperties* css_properties_create(void);

	/**
	 * @brief Releases CSS properties container and all contained properties.
	 * @param props Container to destroy (NULL-safe)
	 */
	void css_properties_destroy(CSSProperties* props);

	/**
	 * @brief Sets or updates CSS property with validation and bitmask tracking.
	 * @param props Target properties container
	 * @param key CSS property name (case-insensitive)
	 * @param value CSS property value
	 * @param important Non-zero for !important priority
	 * @return Success: 1
	 * @return Failure: 0 with error set
	 */
	int css_properties_set(CSSProperties* props, const char* key, const char* value, int important);

	/**
	 * @brief Retrieves CSS property value.
	 * @param props Properties container to query
	 * @param key CSS property name (case-insensitive)
	 * @return Found: Property value string (do not free)
	 * @return Not found: NULL (not an error)
	 * @return Error: NULL with error set
	 */
	const char* css_properties_get(const CSSProperties* props, const char* key);

	/**
	 * @brief Checks if CSS property exists in container.
	 * @param props Properties container to check
	 * @param key CSS property name (case-insensitive)
	 * @return 1: Property exists
	 * @return 0: Property does not exist
	 */
	int css_properties_has(const CSSProperties* props, const char* key);

	/**
	 * @brief Merges parent and child properties with CSS cascade rules.
	 * @param parent Inherited properties (NULL allowed)
	 * @param child Element-specific properties (NULL treated as empty)
	 * @return Success: Merged properties (caller owns)
	 * @return Failure: NULL with error set
	 */
	CSSProperties* css_properties_merge(const CSSProperties* parent, const CSSProperties* child);

	/**
	 * @brief Applies CSS properties to LaTeX conversion context.
	 * @param converter Active LaTeX conversion context
	 * @param props CSS properties to apply
	 * @param tag_name HTML element name for context-aware application
	 */
	void css_properties_apply(LaTeXConverter* converter, const CSSProperties* props, const char* tag_name);

	/**
	 * @brief Finalizes CSS application and closes opened LaTeX constructs.
	 * @param converter Conversion context with pending CSS
	 * @param props Originally applied properties (for context)
	 * @param tag_name HTML element name
	 */
	void css_properties_end(LaTeXConverter* converter, const CSSProperties* props, const char* tag_name);

	/**
	 * @brief Creates deep copy of CSS properties container.
	 * @param src Source properties to copy
	 * @return Success: Independent copy of properties
	 * @return Failure: NULL with error set
	 */
	CSSProperties* css_properties_copy(const CSSProperties* src);

	/**
	 * @brief Parses inline style attribute into CSS properties container.
	 * @param style_str CSS declaration block (e.g., "color: red; font-weight: bold")
	 * @return Success: Properties container (caller owns)
	 * @return Failure: NULL with error set
	 */
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