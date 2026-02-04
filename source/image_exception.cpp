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

ImageRuntimeException ImageRuntimeException::fromImageError() {
    const int error_code = html2tex_get_error();
    const char* error_msg = html2tex_get_error_message();

    return ImageRuntimeException(
        error_msg ? error_msg : 
        "Unknown image error.",
        error_code);
}

ImageRuntimeException ImageRuntimeException::fromCurlError(int curl_error,
    const std::string& url) {

    const char* curl_msg = curl_easy_strerror(static_cast<CURLcode>(curl_error));
    std::string message = "CURL error [" + std::to_string(curl_error) +
        "]: " + (curl_msg ? curl_msg : "Unknown CURL error");

    if (!url.empty()) {
        message += " while downloading: " + url;
    }

    return ImageRuntimeException(message, 
        HTML2TEX_ERR_IMAGE_DOWNLOAD);
}

ImageRuntimeException ImageRuntimeException::fromFileError(const std::string& path,
    const std::string& operation, int error_code) {

    std::string message = "File " + operation + " failed for '" + path + "'";

    if (error_code != 0) {
        const char* error_str = std::strerror(error_code);
        message += ": " + (error_str ? 
            std::string(error_str) : "Unknown image error.");
    }

    return ImageRuntimeException(message, 
        HTML2TEX_ERR_IO);
}

ImageRuntimeException ImageRuntimeException::fromNetworkError(const std::string& url,
    int http_status) {

    std::string message = "Network error";

    if (http_status != 0) 
        message += " [HTTP " + std::to_string(http_status) + "]";

    if (!url.empty()) 
        message += " while accessing: " + url;

    return ImageRuntimeException(message,
        http_status != 0 ? HTML2TEX_ERR_DOWNLOAD 
        : HTML2TEX_ERR_NETWORK);
}

void ImageRuntimeException::format(std::ostream& stream) const {
    stream << "[Image Error " << code() << "] " << message();

    if (file()) {
        stream << " (at " << file();

        if (line() > 0)
            stream << ":" << line();

        stream << ")";
    }
}