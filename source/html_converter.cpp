#include "html2tex.h"
#include "htmltex.h"
#include <iostream>

HtmlTeXConverter::HtmlTeXConverter() : converter(nullptr, &html2tex_destroy), valid(false) {
    LaTeXConverter* raw_converter = html2tex_create();

    if (raw_converter) {
        converter.reset(raw_converter);
        valid = true;
    }
}

HtmlTeXConverter::HtmlTeXConverter(const HtmlTeXConverter& converter) 
: converter(nullptr, &html2tex_destroy), valid(converter.valid) {
    if (converter.converter) {
        LaTeXConverter* clone = html2tex_copy(converter.converter.get());
        if (clone) this->converter.reset(clone);
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
    if (!converter || !valid) return "";
    char* result = html2tex_convert(converter.get(), html.c_str());

    if (result) {
        std::string latex(result);
        free(result);
        return latex;
    }

    return "";
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

HtmlTeXConverter& HtmlTeXConverter::operator =(const HtmlTeXConverter& converter) {
    if (this != &converter) {
        if (converter.converter) {
            LaTeXConverter* clone = html2tex_copy(converter.converter.get());
            if (clone) this->converter.reset(clone);
            else this->converter.reset();
        }
        else this->converter.reset();
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