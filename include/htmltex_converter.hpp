#ifndef HTMLTEX_CONVERTER_HPP
#define HTMLTEX_CONVERTER_HPP

#include <memory>
#include <string>
#include "html2tex.h"
#include "html_parser.hpp"
#include "ext/image_manager.hpp"
#include "latex_exception.hpp"
#include <iostream>

/**
 * @class HtmlTeXConverter
 * @brief RAII wrapper for HTML to LaTeX conversion.
 *
 * This class manages the conversion context for transforming HTML content
 *
 * to LaTeX documents. It supports image downloading, custom output directories,
 *
 * and file-based conversion.
 *
 * @note Thread-safe: Multiple converters can operate concurrently.
 * @warning Not thread-safe for concurrent operations on same instance.
 * @see HtmlParser
 */
class HtmlTeXConverter {
private:
    std::unique_ptr<LaTeXConverter, decltype(&html2tex_destroy)> converter;
    std::unique_ptr<ImageManager> image_manager;
    std::string image_directory;
    bool downloads_enabled, valid;

public:
    /**
     * @brief Constructs a new converter instance.
     * @throws LaTeXRuntimeException if converter initialization fails.
     * @throws std::bad_alloc if memory allocation fails.
     */
    HtmlTeXConverter();
    ~HtmlTeXConverter() = default;

    /**
     * @brief Converts HTML string to LaTeX document.
     * @param html HTML source code to convert.
     *
     * @return Complete LaTeX document as string.
     * @return Empty string for empty input.
     * @throws LaTeXRuntimeException if conversion fails.
     * @throws std::runtime_error if converter is not valid.
     */
    std::string convert(const std::string& html) const;

    /**
      * @brief Converts HtmlParser content to LaTeX document.
      * @param parser Parser containing HTML to convert.
      * @return Complete LaTeX document as string.
      * @return Empty string for empty parser.
      * @throws LaTeXRuntimeException if conversion fails.
      * @throws std::runtime_error if converter is not valid.
     */
    std::string convert(const HtmlParser& parser) const;

    /**
     * @brief Converts HTML string to LaTeX and writes to file.
     * @param html HTML source code to convert.
     * @param filePath Path to output LaTeX file.
     * @return true if conversion and writing succeeded.
     * @throws LaTeXRuntimeException if conversion fails.
     * @throws std::runtime_error if converter is not valid or file I/O fails.
     */
    bool convertToFile(const std::string& html, const std::string& filePath) const;

    /**
     * @brief Converts HtmlParser content to LaTeX and writes to file.
     * @param parser Parser containing HTML to convert.
     * @param filePath Path to output LaTeX file.
     * @return true if conversion and writing succeeded.
     * @throws LaTeXRuntimeException if conversion fails.
     * @throws std::runtime_error if converter is not valid or file I/O fails.
     */
    bool convertToFile(const HtmlParser& parser, const std::string& filePath) const;

    /**
     * @brief Converts HtmlParser content to LaTeX and writes to stream.
     * @param parser Parser containing HTML to convert.
     * @param output Output stream for LaTeX content.
     * @return true if conversion and writing succeeded.
     * @throws LaTeXRuntimeException if conversion fails.
     * @throws std::runtime_error if converter is not valid or stream I/O fails.
     */
    bool convertToFile(const HtmlParser& parser, std::ofstream& output) const;

    /**
     * @brief Sets directory for downloaded images.
     * @param fullPath Directory path for image storage.
     * @return true if directory was set successfully.
     * @note Enables automatic image downloading.
     * @warning Directory must exist and be writable.
     */
    bool setDirectory(const std::string& fullPath) noexcept;

    /**
     * @brief Enables or disables lazy downloading mode for deferred image processing.
     * @param enabled Boolean flag controlling lazy download mode.
     *
     * @return true if operation succeeded
     * @return false if operation failed (check error state)
     *
     * @throws LaTeXRuntimeException if converter is not initialized
     * @throws LaTeXRuntimeException if underlying C API returns error
     *
     * @note Requires converter to be initialized via constructor
     * @note Does not affect previously queued images in storage
     * @warning When disabling with pending downloads, queued images may be lost
     *          unless processed via clear_image_storage()
     *
     * @see HtmlTeXConverter::setDirectory() for output directory configuration
     * @see clear_image_storage() (C API) for processing queued images
     * @see html2tex_enable_downloads() (C API) for underlying implementation
     */
    bool enableLazyDownloading(bool enabled);

    /**
     * @brief Checks for errors from last operation.
     * @return true if error occurred, false otherwise.
     */
    bool hasError() const;

    /**
     * @brief Gets error code from last operation.
     * @return Error code (HTML2TEX_OK = 0 means no error).
     */
    int getErrorCode() const;

    /**
     * @brief Provides access to the asynchronous image download manager.
     * @return ImageManager reference to internal image manager instance.
     *
     * @throws RuntimeException if converter is not initialized
     * @throws RuntimeException if image directory is not configured via setDirectory()
     * @throws std::bad_alloc if memory allocation for ImageManager fails
     *
     * @note ImageManager is created lazily on first access
     * @note Subsequent calls return reference to same manager instance
     * @note Thread-safe for concurrent access after initialization
     * @warning Must call setDirectory() before first access to getImageManager()
     *
     * @see setDirectory() for image output configuration
     * @see ImageManager for complete asynchronous download capabilities
     * @see HtmlTeXConverter::convert() for integrated conversion workflow
     */
    ImageManager& getImageManager();

    /**
     * @brief Gets error message from last operation.
     * @return Human-readable error description.
     */
    std::string getErrorMessage() const;

    /**
     * @brief Checks if converter is properly initialized.
     * @return true if converter is valid and ready for use.
     */
    bool isValid() const;

    /**
     * @brief Move constructor.
     * @param other Converter to move from.
     * @post other.isValid() == false
     */
    HtmlTeXConverter(HtmlTeXConverter&& other) noexcept;

    /**
     * @brief Copy constructor (deep copy).
     * @param other Converter to copy.
     * @throws LaTeXRuntimeException if copy fails.
     * @throws std::bad_alloc if memory allocation fails.
     */
    HtmlTeXConverter(const HtmlTeXConverter& other);

    /**
     * @brief Copy assignment operator (deep copy).
     * @param other Converter to copy from.
     * @return Reference to this converter.
     * @throws LaTeXRuntimeException if copy fails.
     * @throws std::bad_alloc if memory allocation fails.
     */
    HtmlTeXConverter& operator =(const HtmlTeXConverter& other);

    /**
     * @brief Move assignment operator.
     * @param other Converter to move from.
     * @return Reference to this converter.
     * @post other.isValid() == false
     */
    HtmlTeXConverter& operator =(HtmlTeXConverter&& other) noexcept;
};

#endif