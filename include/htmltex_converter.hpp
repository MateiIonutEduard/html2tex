#ifndef HTMLTEX_CONVERTER_HPP
#define HTMLTEX_CONVERTER_HPP

#include <memory>
#include <string>
#include "html2tex.h"
#include "html_parser.hpp"
#include "ext/image_manager.hpp"
#include "latex_exception.hpp"
#include <iostream>
#include <vector>

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
     * @brief Retrieves queued image download requests from deferred storage.
     * @return std::vector<ImageManager::DownloadRequest> Vector of download requests
     *         - Empty if no images queued or converter invalid
     *         - Each request contains URL, output directory, and sequence number
     *         - Sequence numbers start at 1 and preserve original order
     *
     * @throws RuntimeException if converter is not initialized
     * @throws LaTeXRuntimeException if underlying C API reports error
     * @throws std::bad_alloc if memory allocation fails during conversion
     *
     * @note Clears internal ImageStorage as side effect (transfer semantics)
     * @note Requires image directory to be set via setDirectory() for valid requests
     * @note Thread-safe only if converter is not accessed concurrently
     * @warning After calling, internal storage will be empty
     * @warning Returned vector must be processed before converter destruction
     *
     * @see setDirectory() for output directory configuration
     * @see enableLazyDownloading() for deferred download mode
     * @see ImageManager::downloadBatch() for processing returned requests
     * @see clear_image_storage() (C API) for underlying implementation
     */
    std::vector<ImageManager::DownloadRequest> getImages();

    /**
     * @brief Asynchronously downloads a provided list of images using the internal ImageManager.
     *
     * @param imageList Vector of DownloadRequest objects specifying images to download.
     *         - Each request must contain a valid URL, output directory, and
     *         - sequence number. Empty lists are handled as no-ops.
     *
     * @note The converter must have a valid ImageManager instance, typically initialized
     *       after calling setDirectory(). The ImageManager is created lazily on first access.
     * @warning The function modifies internal ImageManager state but is thread-safe for
     *          concurrent calls with different image lists.
     * @post Downloads execute asynchronously; the function returns immediately after
     *        scheduling all download tasks.
     * @post Failed downloads are logged to std::cerr with descriptive error messages
     *       but do not prevent processing of remaining images.
     *
     * @throws RuntimeException If the converter is not properly initialized or
     *         ImageManager cannot be created.
     * @throws std::invalid_argument If any DownloadRequest contains invalid parameters
     *         (empty URL, empty output directory, or negative sequence number).
     * @throws std::bad_alloc If memory allocation fails during batch processing.
     *
     * @par Performance Characteristics:
     * - Time Complexity: O(n) for n image requests
     * - Parallelism: Utilizes all available ImageManager worker threads (default: 4)
     * - Memory: O(n) allocation for internal task scheduling
     * - I/O: Non-blocking; returns immediately after queueing tasks
     *
     * @see downloadQueuedImagesAsync() For processing converter-accumulated images
     * @see getImages() For retrieving queued images from the converter
     * @see ImageManager::downloadBatch() For the underlying batch download implementation
     * @see setDirectory() For configuring default image output location
     */
    void downloadImageListAsync(std::vector<ImageManager::DownloadRequest> imageList);

    /**
     * @brief Asynchronously downloads all queued images accumulated during lazy download mode conversion.
     * @note Requires the converter to be in lazy download mode (enabled via enableLazyDownloading(true))
     *       and an image directory to be configured via setDirectory().
     * @warning Not thread-safe. External synchronization required if called concurrently.
     * @post Failed downloads are logged to std::cerr with descriptive error messages.
     * @post Image queue is cleared internally; subsequent calls will process new images only.
     *
     * @see getImages() For retrieving queued image requests without processing
     * @see ImageManager::downloadBatch() For the underlying batch download mechanism
     * @see enableLazyDownloading() For controlling deferred download mode
     * @see setDirectory() For configuring image output location
     *
     * @throws LaTeXRuntimeException If the converter is not properly initialized or
     *         underlying LaTeX conversion API reports an error.
     * @throws RuntimeException If image directory is not configured via setDirectory().
     * @throws std::bad_alloc If memory allocation fails during batch processing.
     */
    void downloadQueuedImagesAsync();

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