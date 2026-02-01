#ifndef HTMLTEX_DOCUMENT_HPP
#define HTMLTEX_DOCUMENT_HPP

#include <memory>
#include <string>
#include "html2tex.h"
#include "htmltex.hpp"
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
 *      Minified HTML encoding (future feature).
 *
 * @var HtmlEncodingType::HTML_STANDARD
 *      Standard HTML encoding with proper formatting (future feature).
 *
 * @note Currently unused in HtmlDocument implementation. Reserved for
 *       future encoding-aware parsing and serialization features.
 *
 * @see HtmlDocument for potential future usage
 */
enum class HtmlEncodingType : uint8_t {
	NONE = 0,
	HTML_MINIFIED = 1,
	HTML_STANDARD = 2
};

/**
 * @class HtmlDocument
 * @brief RAII wrapper for HTML document lifecycle management.
 *
 * HtmlDocument provides a modern C++ interface for 
 * 
 * managing HTML document instances with automatic 
 * 
 * resource management. It encapsulates HtmlParser
 * 
 * with unique_ptr semantics, ensuring proper cleanup 
 * 
 * and exception safety.
 *
 * @note This class is designed for scenarios requiring document persistence
 *       and copy semantics. For lightweight parsing, use HtmlParser directly.
 *
 * @warning Copy operations may throw HtmlRuntimeException on memory allocation
 *          or DOM tree copy failures.
 *
 * @thread_safety Thread-safe for independent instances. Not thread-safe
 *                for concurrent modifications to the same instance.
 *
 * @see HtmlParser for underlying parsing functionality
 * @see HtmlTeXConverter for LaTeX conversion
 */
class HtmlDocument {
private:
	std::unique_ptr<HtmlParser> htmlParser;

public:
	/**
	 * @brief Constructs an empty HtmlDocument.
	 *
	 * Creates a document with no HTML content. Subsequent 
	 * 
	 * operations requiring content will behave as no-ops 
	 * 
	 * or return empty results.
	 *
	 * @post hasContent() == false
	 * @post htmlParser.get() == nullptr
	 *
	 * @exception_specification noexcept
	 *
	 * @complexity O(1)
	 *
	 * @example
	 * @code
	 * HtmlDocument doc;  // Empty document
	 * assert(!doc.hasContent());
	 * @endcode
	 */
	HtmlDocument() noexcept;

	/**
	 * @brief Constructs HtmlDocument from existing HtmlParser.
	 *
	 * Creates a document by copying content from an HtmlParser instance.
	 * 
	 * Only copies if the parser contains valid content.
	 *
	 * @param parser Source parser containing HTML content.
	 *
	 * @post If parser.hasContent() == true:
	 *       - hasContent() == true
	 *       - Content matches parser's content
	 *       - htmlParser is valid unique_ptr
	 * @post If parser.hasContent() == false:
	 *       - hasContent() == false
	 *       - htmlParser == nullptr
	 *
	 * @throws HtmlRuntimeException If content copy fails (memory allocation,
	 * 
	 *         DOM tree corruption, or CSS computation errors).
	 *
	 * @note Exception is caught and logged to std::cerr, leaving document empty.
	 *
	 * @complexity O(N) where N = nodes in DOM tree
	 *
	 * @example
	 * @code
	 * HtmlParser parser("<html><body>Hello</body></html>");
	 * HtmlDocument doc(parser);  // Copy content
	 * assert(doc.hasContent());
	 * @endcode
	 */
	HtmlDocument(const HtmlDocument& other);

	/**
	 * @brief Copy constructor (deep copy).
	 *
	 * Creates a new document with independent copy of another document's content.
	 *
	 * @param other Source document to copy from.
	 *
	 * @post If other.hasContent() == true:
	 *       - hasContent() == true
	 *       - Content equals other's content
	 *       - Independent memory allocation
	 * @post If other.hasContent() == false:
	 *       - hasContent() == false
	 *       - htmlParser == nullptr
	 *
	 * @throws HtmlRuntimeException If deep copy fails (memory allocation,
	 * 
	 *         DOM tree copy errors, or CSS property duplication failures).
	 *
	 * @note Exception is caught and logged to std::cerr, leaving document empty.
	 * @warning Expensive operation for large documents. Consider move semantics.
	 *
	 * @complexity O(N) where N = nodes in DOM tree
	 *
	 * @example
	 * @code
	 * HtmlDocument doc1(parser);
	 * HtmlDocument doc2(doc1);  // Deep copy
	 * assert(doc2.hasContent() == doc1.hasContent());
	 * @endcode
	 */
	HtmlDocument(const HtmlParser& parser);

	/**
	 * @brief Destructor with RAII cleanup.
	 *
	 * Automatically releases all document resources, including DOM tree
	 * 
	 * and CSS properties. No explicit cleanup required.
	 *
	 * @post All memory returned to system
	 * @post htmlParser == nullptr (after destruction)
	 *
	 * @exception_specification noexcept
	 *
	 * @complexity O(N) where N = nodes in DOM tree
	 *
	 * @note Part of RAII pattern - ensures no memory leaks
	 */
	~HtmlDocument() = default;
};

#endif