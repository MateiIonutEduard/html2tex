#ifndef IMAGE_DOWNLOADER_H
#define IMAGE_DOWNLOADER_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct ImageStorage ImageStorage;
	typedef struct DownloadResult DownloadResult;

	typedef struct DownloadRequest DownloadRequest;
	typedef struct ImageDownloader ImageDownloader;

    /**
     * @brief Result structure for individual download operations.
     */
    struct DownloadResult {
        char* url;              /* Original source URL/Data URI */
        char* local_path;       /* Path to downloaded file (NULL on failure) */
        bool success;           /* Whether download succeeded */
        char* error;           /* Error description (NULL on success) */
        int sequence_number;    /* Request sequence number */
    };

    /**
     * @brief Request structure for download operations.
     */
    struct DownloadRequest {
        char* url;              /* Source URL or data URI */
        char* output_dir;       /* Destination directory */
        int sequence_number;    /* Unique identifier */
    };

    /**
     * @brief Opaque handle for asynchronous download manager.
     */
    struct ImageDownloader;

    /**
     * @brief Callback type for individual download completion.
     * @param result Download result (ownership transferred to callback)
     * @param user_data User context passed during initialization
     */
    typedef void (*DownloadCallback)(DownloadResult* result, void* user_data);

    /**
     * @brief Callback type for batch completion.
     * @param total Total number of downloads
     * @param completed Number of successful downloads
     * @param user_data User context passed during initialization
     */
    typedef void (*BatchCompleteCallback)(size_t total, size_t completed, void* user_data);

    /**
     * @brief Creates a new asynchronous download manager.
     * @param max_workers Maximum number of concurrent download threads (0 = auto-detect)
     * @param callback Optional callback for individual download completion (NULL allowed)
     * @param batch_callback Optional callback for batch completion (NULL allowed)
     * @param user_data User context passed to callbacks
     * @return New ImageDownloader instance, NULL on failure
     */
    ImageDownloader* image_downloader_create(size_t max_workers,
        DownloadCallback callback,
        BatchCompleteCallback batch_callback,
        void* user_data);

    /**
     * @brief Enqueues a single image for asynchronous download.
     * @param downloader Download manager instance
     * @param url Source URL or data URI (copied internally)
     * @param output_dir Destination directory (copied internally)
     * @param sequence_number Unique identifier for ordering
     * @return true on success, false on failure
     */
    bool image_downloader_enqueue(ImageDownloader* downloader,
        const char* url,
        const char* output_dir,
        int sequence_number);

    /**
     * @brief Enqueues all images from ImageStorage for deferred processing.
     * @param downloader Download manager instance
     * @param storage ImageStorage containing deferred download requests
     * @param output_dir Base output directory for all downloads
     * @return Number of images enqueued, -1 on error
     */
    int image_downloader_enqueue_storage(ImageDownloader* downloader,
        ImageStorage* storage,
        const char* output_dir);

    /**
     * @brief Starts asynchronous processing of all queued downloads.
     * @param downloader Download manager instance
     * @return true on success, false on failure
     */
    bool image_downloader_start(ImageDownloader* downloader);

    /**
     * @brief Waits for all pending downloads to complete.
     * @param downloader Download manager instance
     * @param timeout_ms Maximum wait time in milliseconds (0 = infinite)
     * @return true if all downloads completed, false on timeout or error
     */
    bool image_downloader_wait(ImageDownloader* downloader, unsigned int timeout_ms);

    /**
     * @brief Cancels all pending downloads immediately.
     * @param downloader Download manager instance
     * @return Number of pending downloads cancelled
     */
    size_t image_downloader_cancel(ImageDownloader* downloader);

    /**
     * @brief Checks if downloads are currently active.
     * @param downloader Download manager instance
     * @return true if downloads are active or queued
     */
    bool image_downloader_is_active(const ImageDownloader* downloader);

    /**
     * @brief Retrieves all completed results. (consumes results)
     * @param downloader Download manager instance
     * @param count Output parameter for number of results
     * @return Array of DownloadResult pointers (caller must free array and results)
     */
    DownloadResult** image_downloader_get_results(ImageDownloader* downloader, size_t* count);

    /**
     * @brief Frees a DownloadResult structure and all contained strings.
     * @param result Result to free (NULL-safe)
     */
    void download_result_free(DownloadResult* result);

    /**
     * @brief Destroys download manager and all associated resources
     * @param downloader Download manager to destroy (NULL-safe)
     */
    void image_downloader_destroy(ImageDownloader* downloader);

#ifdef __cplusplus
}
#endif

#endif