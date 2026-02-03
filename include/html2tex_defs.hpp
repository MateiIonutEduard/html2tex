#ifndef HTML2TEX_DEFS_HPP
#define HTML2TEX_DEFS_HPP

#include <iostream>

/**
 * @enum HtmlEncodingType
 * @brief Specifies HTML document encoding and processing modes.
 *
 * Used to configure document parsing and serialization behavior.
 *
 * Currently reserved for future expansion of encoding support.
 *
 * @var HtmlEncodingType::NONE
 *      No specific encoding (default).
 *
 * @var HtmlEncodingType::HTML_MINIFIED
 *      Minified HTML encoding.
 *
 * @var HtmlEncodingType::HTML_STANDARD
 *      Standard HTML encoding with proper formatting.
 *
 * @note Currently unused in HtmlDocument implementation. Reserved for
 *       future encoding-aware parsing and serialization features.
 *
 * @see HtmlParser for potential usage
 */
enum class HtmlEncodingType : uint8_t {
	NONE = 0,
	HTML_MINIFIED = 1,
	HTML_STANDARD = 2
};

#endif