#ifndef IMAGE_EXCEPTION_HPP
#define IMAGE_EXCEPTION_HPP

#include "base_exception.hpp"
#include "html2tex.h"
#include <string>

/**
 * @class ImageRuntimeException
 * @brief Image-specific runtime exception for image download and processing errors.
 */
class ImageRuntimeException : public RuntimeException {
public:
    /**
     * @brief Creates an image runtime exception with a message.
     * @param message Error description
     */
    explicit ImageRuntimeException(const std::string& message) noexcept;

    /**
     * @brief Creates an image runtime exception with message and error code.
     * @param message Error description
     * @param code Numeric error code
     */
    ImageRuntimeException(const std::string& message, int code) noexcept;

    /**
     * @brief Creates an image runtime exception with full context.
     * @param message Error description
     * @param code Numeric error code
     * @param file Source file where error occurred
     * @param line Line number where error occurred
     */
    ImageRuntimeException(const std::string& message, int code,
        const char* file, int line) noexcept;

    /**
     * @brief Creates exception from current image error state.
     * @return ImageRuntimeException with current error state
     */
    static ImageRuntimeException fromImageError();

    /**
     * @brief Creates exception from libcurl error.
     * @param curl_error libcurl error code
     * @param url URL that failed to download
     * @return ImageRuntimeException with libcurl error context
     */
    static ImageRuntimeException fromCurlError(int curl_error, const std::string& url);

    /**
     * @brief Destructor.
     */
    virtual ~ImageRuntimeException() noexcept override = default;
};

#endif