#ifndef HTMLTEX_H
#define HTMLTEX_H

#include <memory>
#include <string>
#include "html2tex.h"
#include <iostream>

class HtmlParser {
private:
    std::unique_ptr<HTMLNode, decltype(&html2tex_free_node)> node;
    int minify;
    void setParent(std::unique_ptr<HTMLNode, decltype(&html2tex_free_node)> new_node) noexcept;

public:
    /* Create an empty, valid parser instance. */
    HtmlParser();

    /* Creates a parser from the input HTML. */
    explicit HtmlParser(const std::string& html);

    /* Initializes parser from HTML with optimization flag. */
    HtmlParser(const std::string& html, int minify_flag);

    /* Initializes the parser with the input DOM tree. */
    explicit HtmlParser(const HTMLNode* raw_node);

    /* Instantiates the parser with the DOM tree and the minify option. */
    HtmlParser(const HTMLNode* raw_node, int minify_flag);

    /* Clones an existing HtmlParser to create a new parser. */
    HtmlParser(const HtmlParser& other);

    /* Efficiently moves an existing HtmlParser instance. */
    HtmlParser(HtmlParser&& other) noexcept;

    /* Return a pointer to the DOM tree's root node. */
    HTMLNode* getHtmlNode() const noexcept;

    /* Check whether the parser contains content. */
    bool hasContent() const noexcept;

    HtmlParser& operator =(const HtmlParser& other);
    HtmlParser& operator =(HtmlParser&& other) noexcept;

    friend std::ostream& operator <<(std::ostream& out, const HtmlParser& parser);
    friend std::istream& operator >>(std::istream& in, HtmlParser& parser);

    /* Initializes the parser from the given file stream. */
    static HtmlParser fromStream(std::ifstream& input) noexcept;

    /* Creates a parser from the given HTML file path. */
    static HtmlParser fromHtml(const std::string& filePath) noexcept;

    /* Write the DOM tree in HTML format to the file at the specified path. */
    void writeTo(const std::string& filePath) const;

    /* Returns prettified HTML from this instance. */
    std::string toString() const;
    ~HtmlParser() = default;
};

class HtmlTeXConverter {
private:
    std::unique_ptr<LaTeXConverter, decltype(&html2tex_destroy)> converter;
    bool valid;

public:
    /* Create a new valid HtmlTeXConverter instance. */
    HtmlTeXConverter();
    ~HtmlTeXConverter() = default;

    /* Convert the input HTML code to the corresponding LaTeX output. */
    std::string convert(const std::string& html) const;

    /* Convert the HtmlParser instance to its corresponding LaTeX output. */
    std::string convert(const HtmlParser& parser) const;

    /* Convert the input HTML code to LaTeX and write the output to the file at the specified path. */
    bool convertToFile(const std::string& html, const std::string& filePath) const;

    /* Convert the HtmlParser instance to LaTeX and write the result to the specified file path. */
    bool convertToFile(const HtmlParser& parser, const std::string& filePath) const;

    /* Convert the HtmlParser instance to LaTeX and write the result to a file. */
    bool convertToFile(const HtmlParser& parser, std::ofstream& output) const;

    /*
       Set the directory where images extracted from the DOM tree are saved.
       @return true on success, false otherwise.
    */
    bool setDirectory(const std::string& fullPath) const noexcept;

    /* Check for errors during conversion. */
    bool hasError() const;

    /* Return the conversion error code. */
    int getErrorCode() const;

    /* Return the conversion error message. */
    std::string getErrorMessage() const;

    /* Check whether the converter is initialized and valid. */
    bool isValid() const;

    /* Efficiently moves an existing HtmlTeXConverter instance. */
    HtmlTeXConverter(HtmlTeXConverter&& other) noexcept;

    /* Creates a new HtmlTeXConverter by copying an existing instance. */
    HtmlTeXConverter(const HtmlTeXConverter& other);

    HtmlTeXConverter& operator =(const HtmlTeXConverter& other);
    HtmlTeXConverter& operator =(HtmlTeXConverter&& other) noexcept;
};

#endif