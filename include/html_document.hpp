#ifndef HTMLTEX_DOCUMENT_HPP
#define HTMLTEX_DOCUMENT_HPP

#include "dom_tree.h"
#include "dom_tree_visitor.h"
#include "css_properties.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>


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
 * @brief Modern C++ wrapper for HTML DOM elements with RAII management and traversal API.
 *
 * This class provides a safe, modern interface to the underlying C DOM API.
 * 
 * It handles memory management automatically and provides convenient methods
 * 
 * for DOM traversal and manipulation.
 */
class HtmlDocument {
private:
    static int getElementByIdPredicate(const HTMLNode* root, const void* data);
    static int getElementByClassPredicate(const HTMLNode* root, const void* data);

public:
    class Iterator;
    class ConstIterator;

    /**
     * @brief Constructs an empty HtmlDocument.
     */
    HtmlDocument() noexcept;

    /**
     * @brief Constructs from raw HTMLNode pointer (non-owning).
     * @param node Raw HTMLNode pointer (must remain valid for lifetime of this object)
     */
    explicit HtmlDocument(HTMLNode* node) noexcept;

    /**
     * @brief Constructs from raw HTMLNode pointer with CSS properties.
     * @param node Raw HTMLNode pointer (must remain valid for lifetime of this object)
     * @param css CSS properties associated with this element
     */
    HtmlDocument(HTMLNode* node, CSSProperties* css) noexcept;

    /**
     * @brief Constructs from HTMLElement pointer (takes ownership).
     * @param elem HTMLElement pointer (this object takes ownership)
     */
    explicit HtmlDocument(HTMLElement* elem) noexcept;

    /**
     * @brief Copy constructor (deep copy of CSS properties).
     */
    HtmlDocument(const HtmlDocument& other);

    /**
     * @brief Move constructor.
     */
    HtmlDocument(HtmlDocument&& other) noexcept;

    /**
     * @brief Copy assignment operator (deep copy of CSS properties).
     */
    HtmlDocument& operator=(const HtmlDocument& other);

    /**
     * @brief Move assignment operator.
     */
    HtmlDocument& operator=(HtmlDocument&& other) noexcept;

    /**
     * @brief Destructor.
     */
    ~HtmlDocument();

    // Core element properties

    /**
     * @brief Checks if element is valid (has underlying node).
     * @return true if element is valid, false otherwise
     */
    bool isValid() const noexcept;

    /**
     * @brief Gets the element's tag name.
     * @return Tag name string, or empty string if invalid
     */
    std::string tagName() const noexcept;

    /**
     * @brief Gets the element's text content.
     * @return Text content string, or empty string if invalid
     */
    std::string textContent() const noexcept;

    /**
     * @brief Gets an attribute value.
     * @param key Attribute name
     * @return Attribute value, or empty string if not found
     */
    std::string getAttribute(const std::string& key) const;

    /**
     * @brief Checks if element has an attribute.
     * @param key Attribute name
     * @return true if attribute exists, false otherwise
     */
    bool hasAttribute(const std::string& key) const;

    /**
     * @brief Gets the computed CSS properties for this element.
     * @return Pointer to CSS properties (may be null)
     */
    CSSProperties* cssProperties() const noexcept;

    // Parent traversal

    /**
     * @brief Gets the parent element.
     * @return Parent element (may be invalid if no parent)
     */
    HtmlDocument parent() const noexcept;

    /**
     * @brief Checks if element has a parent.
     * @return true if element has a parent, false otherwise
     */
    bool hasParent() const noexcept;

    // Sibling traversal

    /**
     * @brief Gets the next sibling element.
     * @return Next sibling element (may be invalid if no next sibling)
     */
    HtmlDocument nextSibling() const noexcept;

    /**
     * @brief Gets the previous sibling element.
     * @return Previous sibling element (may be invalid if no previous sibling)
     */
    HtmlDocument previousSibling() const noexcept;

    /**
     * @brief Checks if element has a next sibling.
     * @return true if element has a next sibling, false otherwise
     */
    bool hasNextSibling() const noexcept;

    /**
     * @brief Checks if element has a previous sibling.
     * @return true if element has a previous sibling, false otherwise
     */
    bool hasPreviousSibling() const noexcept;

    // Child traversal

    /**
     * @brief Gets the first child element.
     * @return First child element (may be invalid if no children)
     */
    HtmlDocument firstChild() const noexcept;

    /**
     * @brief Gets the last child element.
     * @return Last child element (may be invalid if no children)
     */
    HtmlDocument lastChild() const noexcept;

    /**
     * @brief Checks if element has children.
     * @return true if element has children, false otherwise
     */
    bool hasChildren() const noexcept;

    /**
     * @brief Gets all child elements.
     * @return Vector of child elements (empty if no children)
     */
    std::vector<HtmlDocument> children() const;

    /**
     * @brief Gets the number of child elements.
     * @return Number of children
     */
    size_t childCount() const noexcept;

    // Element type checking

    /**
     * @brief Checks if element is a block-level element.
     * @return true if block-level, false otherwise
     */
    bool isBlockElement() const;

    /**
     * @brief Checks if element is an inline element.
     * @return true if inline, false otherwise
     */
    bool isInlineElement() const;

    /**
     * @brief Checks if element is a void (self-closing) element.
     * @return true if void element, false otherwise
     */
    bool isVoidElement() const;

    /**
     * @brief Checks if element contains only whitespace.
     * @return true if only whitespace, false otherwise
     */
    bool isWhitespaceOnly() const;

    /**
     * @brief Checks if element should be excluded from conversion.
     * @return true if should be excluded, false otherwise
     */
    bool shouldExclude() const;

    /**
     * @brief Finds the first descendant matching a specific ID attribute value.
     * @param id The ID attribute value of HTML tree node
     * @return First matching element (invalid if none found)
     */
    HtmlDocument getFirstElementById(const std::string& id) const;

    /**
     * @brief Finds the first descendant matching a specific class name attribute value.
     * @param className The class name attribute value of HTML tree node
     * @return First matching element (invalid if none found)
     */
    HtmlDocument getFirstElementByClassName(const std::string& className) const;

    /**
     * @brief Finds all descendants matching a specific ID attribute value.
     * @param id The ID attribute value of HTML tree node
     * @return Vector of matching elements
     */
    std::vector<HtmlDocument> findAllElementsById(const std::string& id) const;

    /**
     * @brief Finds all descendants matching a specific class name attribute value.
     * @param className The class name attribute value of HTML tree node
     * @return Vector of matching elements
     */
    std::vector<HtmlDocument> findAllElementsByClassName(const std::string& className) const;

    /**
     * @brief Checks if any descendant element has the specified ID.
     * @param id The ID to search for (case-sensitive)
     * @return true if at least one descendant element matches the ID,
     * @return false otherwise or if the document is invalid
     */
    bool hasElementWithId(const std::string& id) const;

    /**
     * @brief Checks if any descendant element has the specified CSS class.
     * @param className The CSS class name to search for (case-sensitive)
     * @return true if at least one descendant element matches the class,
     * @return false otherwise or if the document is invalid
     */
    bool hasElementWithClass(const std::string& className) const;

    // Range-based for loop support

    /**
     * @brief Gets iterator to first child.
     */
    Iterator begin();
    ConstIterator begin() const;
    ConstIterator cbegin() const;

    /**
     * @brief Gets iterator past the last child.
     */
    Iterator end();
    ConstIterator end() const;
    ConstIterator cend() const;

    // Raw pointer access (for compatibility with C API)

    /**
     * @brief Gets raw HTMLNode pointer.
     * @warning Use with caution - pointer validity not guaranteed
     */
    HTMLNode* rawNode() const noexcept;

    /**
     * @brief Creates HtmlDocument from raw HTMLElement pointer (takes ownership).
     * @param elem Raw HTMLElement pointer
     * @return HtmlDocument wrapper
     */
    static HtmlDocument fromRawElement(HTMLElement* elem);

private:
    HTMLNode* node;
    CSSProperties* props;
    bool hasProps;
};

/**
 * @class HtmlDocument::Iterator
 * @brief Forward iterator for child elements.
 */
class HtmlDocument::Iterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = HtmlDocument;
    using difference_type = std::ptrdiff_t;
    using pointer = HtmlDocument*;
    using reference = HtmlDocument&;

    Iterator() noexcept;
    explicit Iterator(HTMLNode* node, CSSProperties* css = nullptr) noexcept;

    reference operator*() noexcept;
    pointer operator->() noexcept;

    Iterator& operator++() noexcept;
    Iterator operator++(int) noexcept;

    bool operator==(const Iterator& other) const noexcept;
    bool operator!=(const Iterator& other) const noexcept;

private:
    HTMLNode* current;
    CSSProperties* props;
    HtmlDocument element;
};

/**
 * @class HtmlDocument::ConstIterator
 * @brief Const forward iterator for child elements.
 */
class HtmlDocument::ConstIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const HtmlDocument;
    using difference_type = std::ptrdiff_t;
    using pointer = const HtmlDocument*;
    using reference = const HtmlDocument&;

    ConstIterator() noexcept;
    explicit ConstIterator(const HTMLNode* node, CSSProperties* css = nullptr) noexcept;

    reference operator*() const noexcept;
    pointer operator->() const noexcept;

    ConstIterator& operator++() noexcept;
    ConstIterator operator++(int) noexcept;

    bool operator==(const ConstIterator& other) const noexcept;
    bool operator!=(const ConstIterator& other) const noexcept;

private:
    const HTMLNode* current;
    CSSProperties* props;
    HtmlDocument element;
};

#endif