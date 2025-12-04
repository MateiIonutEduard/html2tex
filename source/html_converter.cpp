#include "html2tex.h"
#include "htmltex.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

HtmlTeXConverter::HtmlTeXConverter() : converter(nullptr, &html2tex_destroy), valid(false) {
    LaTeXConverter* raw_converter = html2tex_create();

    if (raw_converter) {
        converter.reset(raw_converter);
        valid = true;
    }
}

HtmlTeXConverter::HtmlTeXConverter(const HtmlTeXConverter& other) 
: converter(nullptr, &html2tex_destroy), valid(false) {
    if (other.converter && other.valid) {
        LaTeXConverter* clone = html2tex_copy(other.converter.get());

        if (clone) {
            converter.reset(clone);
            valid = true;
        }
    }
}

HtmlTeXConverter::HtmlTeXConverter(HtmlTeXConverter&& other) noexcept
    : converter(std::move(other.converter)), valid(other.valid) { 
    other.converter.reset(nullptr);
    other.valid = false;
}

bool HtmlTeXConverter::setDirectory(const std::string& fullPath) {
    if (!converter || !valid) return false;
    html2tex_set_image_directory(converter.get(), fullPath.c_str());

    html2tex_set_download_images(converter.get(), 1);
    return true;
}

std::string HtmlTeXConverter::convert(const std::string& html) {
    if (!converter || !valid)
        throw std::runtime_error("Converter not initialized.");

    if (html.empty()) return "";
    char* result = html2tex_convert(converter.get(), html.c_str());

    if (!result) {
        /* check if this is an actual error or just empty conversion */
        if (hasError()) throw std::runtime_error(getErrorMessage());

        /* legitimate empty result */
        return "";
    }

    std::string latex(result);
    free(result);
    return latex;
}

bool HtmlTeXConverter::convertToFile(const std::string& html, const std::string& filePath) {
    /* validate converter and HTML code */
    if (!isValid()) 
        throw std::runtime_error("Converter not initialized.");

    if (html.empty()) 
        return false;

    /* convert the HTML code first */
    std::unique_ptr<char[], void(*)(char*)> result(
        html2tex_convert(converter.get(), html.c_str()),
        [](char* p) noexcept { 
            std::free(p); 
        });

    if (!result) {
        if (hasError()) throw std::runtime_error(getErrorMessage());
        
        /* empty but valid conversion */
        return false;
    }

    /* write the file */
    std::ofstream fout(filePath);

    if (!fout)
        throw std::runtime_error("Cannot open output file.");

    fout << result.get();
    fout.flush();

    if (!fout)
        throw std::runtime_error("Failed to write LaTeX output.");

    return true;
}

std::string HtmlTeXConverter::convert(const HtmlParser& parser) {
    /* precondition validation */
    if (!isValid())
        throw std::runtime_error("HtmlTeXConverter in invalid state.");

    /* get HTML content - one serialization only */
    std::string html = parser.toString();

    /* return empty string for empty input */
    if (html.empty()) return "";

    /* perform conversion */
    char* raw_result = html2tex_convert(converter.get(), html.c_str());

    /* RAII management with custom deleter */
    const auto deleter = [](char* p) noexcept { std::free(p); };
    std::unique_ptr<char[], decltype(deleter)> result_guard(raw_result, deleter);

    /* error analysis */
    if (!raw_result) {
        if (hasError()) {
            throw std::runtime_error(
                "HTML to LaTeX conversion failed: " + getErrorMessage());
        }

        /* valid empty result */
        return "";
    }

    /* return result (compiler will optimize it) */
    return std::string(raw_result);
}

bool HtmlTeXConverter::hasError() const {
    return converter && html2tex_get_error(converter.get()) != 0;
}

int HtmlTeXConverter::getErrorCode() const {
    return converter ? html2tex_get_error(converter.get()) : -1;
}

std::string HtmlTeXConverter::getErrorMessage() const {
    if (!converter) return "Converter not initialized.";
    return html2tex_get_error_message(converter.get());
}

bool HtmlTeXConverter::isValid() const { return valid; }

HtmlTeXConverter& HtmlTeXConverter::operator =(const HtmlTeXConverter& other) {
    if (this != &other) {
        std::unique_ptr<LaTeXConverter, decltype(&html2tex_destroy)> temp(nullptr, &html2tex_destroy);
        bool new_valid = false;

        if (other.converter && other.valid) {
            LaTeXConverter* clone = html2tex_copy(other.converter.get());

            if (clone) {
                temp.reset(clone);
                new_valid = true;
            }
        }

        converter = std::move(temp);
        valid = new_valid;
    }

    return *this;
}

HtmlTeXConverter& HtmlTeXConverter::operator =(HtmlTeXConverter&& other) noexcept {
    if (this != &other) {
        converter = std::move(other.converter);
        valid = other.valid;
        other.valid = false;
    }

    return *this;
}