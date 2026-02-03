#include "html_exception.hpp"
#include "html2tex.h"

HtmlRuntimeException::HtmlRuntimeException(const std::string& message) noexcept
    : RuntimeException(message, 0) {
}

HtmlRuntimeException::HtmlRuntimeException(const std::string& message, int code) noexcept
    : RuntimeException(message, code) {
}

HtmlRuntimeException::HtmlRuntimeException(const std::string& message, int code,
    const char* file, int line) noexcept
    : RuntimeException(message, code, file, line) {
}

HtmlRuntimeException HtmlRuntimeException::fromHtmlError() {
    const int error_code = html2tex_get_error();
    const char* error_msg = html2tex_get_error_message();

    return HtmlRuntimeException(
        error_msg,
        error_code);
}

void HtmlRuntimeException::format(std::ostream& stream) const {
    stream << "[HTML Error " << code() << "] " << message();

    if (file()) {
        stream << " (at " << file();

        if (line() > 0)
            stream << ":" << line();

        stream << ")";
    }
}