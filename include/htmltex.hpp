#ifndef HTMLTEX_HPP
#define HTMLTEX_HPP

#include <memory>
#include <string>
#include "html2tex.h"
#include "htmltex_exception.hpp"
#include <iostream>

/**
 * @class HtmlParser
 * @brief RAII wrapper for HTML DOM tree parsing and manipulation.
 *
 * This class manages the lifecycle of HTML DOM trees parsed 
 * 
 * from strings, files, or streams. It provides methods to 
 * 
 * serialize HTML content and convert it to LaTeX 
 *
 * via HtmlTeXConverter.
 *
 * @note This class is thread-safe for concurrent read operations on
 *       different instances. Write operations are not thread-safe.
 * @warning All methods may throw std::bad_alloc on memory allocation failure.
 * @see HtmlTeXConverter
 */
class HtmlParser {
private:
    std::unique_ptr<HTMLNode, decltype(&html2tex_free_node)> node;
    int minify;
    void setParent(std::unique_ptr<HTMLNode, decltype(&html2tex_free_node)> new_node) noexcept;

public:
    /**
     * @brief Constructs an empty parser instance.
     * @post hasContent() == false
     */
    HtmlParser();

    /**
     * @brief Constructs a parser from HTML string.
     * @param html HTML source code to parse.
     * @throws HtmlRuntimeException if parsing fails.
     * @throws std::bad_alloc if memory allocation fails.
     */
    explicit HtmlParser(const std::string& html);

    /**
     * @brief Constructs a parser with optional minification.
     * @param html HTML source code to parse.
     * @param minify_flag Non-zero to minify HTML during parsing.
     * @throws HtmlRuntimeException if parsing fails.
     * @throws std::bad_alloc if memory allocation fails.
     */
    HtmlParser(const std::string& html, int minify_flag);

    /**
     * @brief Constructs a parser from existing DOM tree (takes ownership).
     * @param raw_node Pointer to DOM tree root (must not be null).
     * @throws HtmlRuntimeException if node is null or invalid.
     * @throws std::bad_alloc if memory allocation fails.
     * @warning Transfers ownership - raw_node becomes invalid after call.
     */
    explicit HtmlParser(const HTMLNode* raw_node);

    /**
     * @brief Constructs a parser from DOM tree with minification option.
     * @param raw_node Pointer to DOM tree root (must not be null).
     * @param minify_flag Non-zero to apply minification to copied tree.
     * @throws HtmlRuntimeException if node is null or invalid.
     * @throws std::bad_alloc if memory allocation fails.
     */
    HtmlParser(const HTMLNode* raw_node, int minify_flag);

    /**
     * @brief Copy constructor (deep copy).
     * @param other Parser to copy.
     * @throws HtmlRuntimeException if deep copy fails.
     * @throws std::bad_alloc if memory allocation fails.
     */
    HtmlParser(const HtmlParser& other);

    /**
     * @brief Move constructor.
     * @param other Parser to move from.
     * @post other.hasContent() == false
     */
    HtmlParser(HtmlParser&& other) noexcept;

    /**
     * @brief Gets raw DOM tree pointer.
     * @return Pointer to internal DOM tree (valid while object exists).
     * @warning Do not free or modify the returned pointer.
     */
    HTMLNode* getHtmlNode() const noexcept;

    /**
     * @brief Checks if parser contains valid HTML content.
     * @return true if parser contains HTML content, false otherwise.
     */
    bool hasContent() const noexcept;

    /**
     * @brief Copy assignment operator (deep copy).
     * @param other Parser to copy from.
     * @return Reference to this parser.
     * @throws HtmlRuntimeException if deep copy fails.
     * @throws std::bad_alloc if memory allocation fails.
     */
    HtmlParser& operator =(const HtmlParser& other);

    /**
     * @brief Move assignment operator.
     * @param other Parser to move from.
     * @return Reference to this parser.
     * @post other.hasContent() == false
     */
    HtmlParser& operator =(HtmlParser&& other) noexcept;

    /**
     * @brief Output stream operator for pretty HTML.
     * @param out Output stream.
     * @param parser Parser to output.
     * @return Reference to output stream.
     */
    friend std::ostream& operator <<(std::ostream& out, const HtmlParser& parser);

    /**
     * @brief Input stream operator for parsing HTML from stream.
     * @param in Input stream.
     * @param parser Parser to populate.
     * @return Reference to input stream.
     * @note Reads entire stream content (up to 128MB limit).
     */
    friend std::istream& operator >>(std::istream& in, HtmlParser& parser);

    /**
     * @brief Creates parser from file stream.
     * @param input Input file stream (must be open for reading).
     * @return HtmlParser instance.
     * @note Stream is read until EOF (up to 128MB limit).
     * @warning Returns empty parser on failure (no exceptions thrown).
     */
    static HtmlParser fromStream(std::ifstream& input) noexcept;

    /**
     * @brief Creates parser from HTML file.
     * @param filePath Path to HTML file.
     * @return HtmlParser instance.
     * @note Reads entire file (up to 128MB limit).
     * @warning Returns empty parser on failure (no exceptions thrown).
     */
    static HtmlParser fromHtml(const std::string& filePath) noexcept;

    /**
     * @brief Writes prettified HTML to file.
     * @param filePath Path to output file.
     * @throws std::logic_error if parser has no content.
     * @throws std::invalid_argument if filePath is empty.
     * @throws std::runtime_error if file I/O fails.
     */
    void writeTo(const std::string& filePath) const;

    /**
     * @brief Serializes parser content to pretty HTML string.
     * @return Pretty HTML string if parser have content.
     * @return Empty string otherwise.
     * @throws HtmlRuntimeException if memory allocation fails.
     */
    std::string toString() const;
    ~HtmlParser() = default;
};

#endif