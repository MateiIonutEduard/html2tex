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