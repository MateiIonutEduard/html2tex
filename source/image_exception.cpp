#include "image_exception.hpp"
#include <curl/curl.h>
#include <cstring>

ImageRuntimeException::ImageRuntimeException(const std::string& message) noexcept
    : RuntimeException(message, 0) {
}

ImageRuntimeException::ImageRuntimeException(const std::string& message, int code) noexcept
    : RuntimeException(message, code) {
}

ImageRuntimeException::ImageRuntimeException(const std::string& message, int code,
    const char* file, int line) noexcept
    : RuntimeException(message, code, file, line) {
}