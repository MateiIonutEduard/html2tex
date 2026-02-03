#include "latex_exception.hpp"
#include "html2tex.h"

LaTeXRuntimeException::LaTeXRuntimeException(const std::string& message) noexcept
    : RuntimeException(message, 0) {
}

LaTeXRuntimeException::LaTeXRuntimeException(const std::string& message, int code) noexcept
    : RuntimeException(message, code) {
}

LaTeXRuntimeException::LaTeXRuntimeException(const std::string& message, int code,
    const char* file, int line) noexcept
    : RuntimeException(message, code, file, line) {
}

LaTeXRuntimeException LaTeXRuntimeException::fromLaTeXError() {
    const int error_code = html2tex_get_error();
    const char* error_msg = html2tex_get_error_message();

    return LaTeXRuntimeException(
        error_msg,
        error_code);
}

void LaTeXRuntimeException::format(std::ostream& stream) const {
    stream << "[LaTeX Error " << code() << "] " << message();

    if (file()) {
        stream << " (at " << file();

        if (line() > 0)
            stream << ":" << line();

        stream << ")";
    }
}