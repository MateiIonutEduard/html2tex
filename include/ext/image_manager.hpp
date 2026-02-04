/**
 * @brief Asynchronous image download manager for HTML to LaTeX conversion
 *
 * @section Overview
 *
 * ImageManager provides efficient, thread-safe asynchronous downloading of images
 * referenced in HTML documents during conversion to LaTeX. It wraps the existing
 * C API (download_image_src) with modern C++14 concurrency primitives, offering
 * both synchronous and asynchronous download patterns with automatic resource
 * management.
 *
 * @section Design Philosophy
 *
 * - Minimal overhead: Direct mapping to underlying C API without abstraction layers
 * - Thread safety: Lock-free where possible, mutex-protected where required
 * - RAII: Automatic cleanup of threads and CURL resources
 * - Non-intrusive: No modifications to existing converter workflows
 * - Failure transparency: Exceptions propagate through futures when appropriate
 *
 * @section Thread Safety
 *
 * - Public member functions are thread-safe
 * - Multiple threads may call downloadAsync() concurrently
 * - Internal synchronization uses single producer-consumer queue
 * - Worker threads are isolated from each other
 *
 * @section Resource Management
 *
 * - CURL is initialized once globally (thread-safe)
 * - Thread pool created on construction, joined on destruction
 * - Pending promises are rejected with exception on destruction
 * - All heap allocations are exception-safe
 *
 * @note This class is non-copyable but supports move semantics
 * @warning Caller must ensure output directories exist or are creatable
 * @see HtmlTeXConverter for integrated usage
 */

#ifndef IMAGE_MANAGER_HPP
#define IMAGE_MANAGER_HPP

#include <string>
#include <vector>
#include <memory>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include "html2tex.h"
#include "image_exception.hpp"

 /**
  * @class ImageManager
  * @brief Manages concurrent downloading of images from URLs and data URIs.
  *
  * Provides a fixed-size thread pool for downloading images asynchronously while
  * 
  * maintaining the order of completion requests. Each download is independent and
  * 
  * may succeed or fail without affecting others.
  */
class ImageManager {
public:
    /**
     * @struct DownloadRequest
     * @brief Complete specification for a single image download.
     */
    struct DownloadRequest {
        std::string url;           ///< Source URL or data URI
        std::string output_dir;    ///< Destination directory
        int sequence_number;       ///< Unique identifier for this download
    };

    /**
     * @struct DownloadResult
     * @brief Outcome of a download operation.
     */
    struct DownloadResult {
        std::string url;           ///< Original source URL
        std::string local_path;    ///< Path to downloaded file (if successful)
        bool success;              ///< Whether download succeeded
        std::string error;         ///< Error description if failed
        int sequence_number;       ///< Request sequence number
    };

    /**
     * @brief Constructs ImageManager with specified thread pool size.
     *
     * Initializes global CURL context and creates worker threads. CURL is
     * 
     * initialized only once globally, even with multiple ImageManager instances.
     *
     * @param max_workers Number of concurrent download threads
     *        - 0: Uses std::thread::hardware_concurrency()
     *        - >0: Uses specified number (capped at system limits)
     *        - Default: 4 threads (balanced for typical I/O workloads)
     *
     * @throws std::system_error if thread creation fails
     * @throws ImageRuntimeException if CURL initialization fails
     *
     * @note Thread count should balance between I/O parallelism and system load
     * @warning Excessive threads may degrade performance due to context switching
     */
    explicit ImageManager(size_t max_workers = 4);

    /**
     * @brief Destructor - ensures graceful shutdown.
     *
     * Performs ordered shutdown:
     * 1. Signals all workers to stop
     * 2. Rejects pending promises with std::runtime_error
     * 3. Joins all worker threads
     * 4. Cleans up global CURL resources (if last instance)
     *
     * @note Blocks until all in-progress downloads complete
     * @warning Pending async operations receive std::runtime_error exception
     */
    ~ImageManager();

    // Non-copyable - prevent accidental duplication of thread pool
    ImageManager(const ImageManager&) = delete;
    ImageManager& operator=(const ImageManager&) = delete;

    // Move operations defaulted (transfer ownership of thread pool)
    ImageManager(ImageManager&&) = default;
    ImageManager& operator=(ImageManager&&) = default;

    /**
     * @brief Initiates asynchronous download without blocking.
     *
     * Queues the download request and returns immediately. The download executes
     * 
     * on a worker thread from the pool. Result is accessible via the returned future.
     *
     * @param request Complete download specification
     * @return std::future<DownloadResult> Future containing eventual result
     *
     * @throws std::runtime_error if manager is shutting down
     * @throws std::bad_alloc if memory allocation fails
     *
     * @par Performance characteristics:
     * - O(1) queue insertion (amortized)
     * - Lock contention limited to queue mutex
     * - Memory: sizeof(DownloadRequest) + promise overhead
     *
     * @note Future may throw ImageRuntimeException if download fails
     * @warning Request output_dir must exist or be creatable
     *
     * @see downloadSync() for blocking alternative
     */
    std::future<DownloadResult> downloadAsync(const DownloadRequest& request);

    /**
     * @brief Executes download synchronously. (blocks until complete)
     *
     * Convenience wrapper around downloadAsync() that blocks calling thread
     * until download completes. Suitable for simple scripts or when immediate
     * result is required.
     *
     * @param request Complete download specification
     * @return DownloadResult Direct result (no future wrapper)
     *
     * @throws ImageRuntimeException if download fails
     * @throws std::runtime_error if manager is shutting down
     *
     * @note Equivalent to downloadAsync(request).get()
     * @warning Blocks calling thread - avoid in performance-critical paths
     */
    DownloadResult downloadSync(const DownloadRequest& request);

    /**
     * @brief Downloads multiple images with all-or-nothing semantics.
     *
     * Initiates all downloads concurrently and waits for all to complete.
     * 
     * Returns vector of results in the same order as requests. This method
     * 
     * provides maximum parallelism for batch operations.
     *
     * @param requests Vector of download requests
     * @return std::vector<DownloadResult> Results in request order
     *
     * @throws std::bad_alloc if memory allocation fails
     *
     * @par Concurrency behavior:
     * - All downloads start immediately (subject to thread pool limits)
     * - Method blocks until all downloads complete
     * - Ordering of completion is non-deterministic
     * - Ordering of results matches input requests
     *
     * @note Some downloads may succeed while others fail
     * @warning Large batches may exhaust file descriptors or memory
     */
    std::vector<DownloadResult> downloadBatch(const std::vector<DownloadRequest>& requests);

    /**
     * @brief Blocks until all queued and active downloads complete
     *
     * Useful for synchronization points, such as before program exit or
     * 
     * when downloads must complete before subsequent processing.
     *
     * @par Implementation details:
     * - Polls internal state with 10ms sleeps
     * - Returns when task queue is empty and no active workers
     * - Does not prevent new downloads during wait
     *
     * @note May return while other threads add new tasks
     * @warning Not suitable for precise synchronization across threads
     */
    void waitForCompletion();

    /**
     * @brief Cancels all pending downloads immediately.
     *
     * Removes all queued tasks and rejects their promises with
     * std::runtime_error.
     * 
     * Actively executing downloads continue to completion.
     *
     * @par Use cases:
     * - Application shutdown
     * - User cancellation
     * - Error recovery
     *
     * @note Active downloads are not interrupted
     * @warning Cancelled promises throw std::runtime_error when accessed
     */
    void cancelAll();

    /**
     * @brief Checks if downloads are currently active or queued.
     *
     * Atomic check of internal state. Useful for progress
     * 
     * indicators or conditional logic based on download
     * 
     * activity.
     *
     * @return true if any of:
     *         - Tasks are queued but not yet started
     *         - Workers are actively downloading
     *         false otherwise
     *
     * @note Race condition: State may change immediately after check
     * @note O(1) atomic read, no locking
     */
    bool isActive() const;

private:
    /**
     * @brief Worker thread entry point.
     *
     * Each worker thread executes this function, continuously:
     * 1. Waiting for tasks on queue
     * 2. Executing download via download_image_src()
     * 3. Fulfilling promise with result
     *
     * @note Thread-safe access to task queue via mutex
     * @warning Catches all exceptions to prevent thread termination
     */
    void workerThread();

    std::queue<std::pair<DownloadRequest, std::promise<DownloadResult>>> task_queue;
    mutable std::mutex queue_mutex;          ///< Protects task_queue
    std::condition_variable queue_cv;        ///< Signals task availability
    std::vector<std::thread> workers;        ///< Worker thread handles
    std::atomic<bool> stop_flag{ false };      ///< Shutdown signal
    std::atomic<size_t> active_workers{ 0 };   ///< Currently executing workers
};

#endif