#include "ext/image_manager.hpp"
#include <cstring>
#include <curl/curl.h>
#include <system_error>
#include <iostream>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define mkdir(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define mkdir(path) mkdir(path, 0755)
#endif

namespace {
    /**
     * @brief Attempts to create directory, but doesn't fail if it exists or can't be created.
     * @param path Directory path
     * @return true if directory exists or was created, false otherwise
     */
    bool tryCreateDirectory(const std::string& path) noexcept {
        if (path.empty())
            return false;

        /* try to create directory */
        if (mkdir(path.c_str()) == 0)
            return true;

        /* if directory already exists, that's fine */
        if (errno == EEXIST)
            return true;

        /* on Windows, check for ERROR_ALREADY_EXISTS */
#ifdef _WIN32
        if (GetLastError() == ERROR_ALREADY_EXISTS)
            return true;
#endif
        std::cerr << "Warning: Could not create directory '" << path
            << "': " << strerror(errno) << std::endl;
        return false;
    }

    /**
     * @brief Downloads single image using existing C API.
     * @param req Download request
     * @return Download result
     */
    ImageManager::DownloadResult downloadSingle(const ImageManager::DownloadRequest& req) {
        ImageManager::DownloadResult result;
        result.url = req.url;
        result.sequence_number = req.sequence_number;

        if (!req.output_dir.empty())
            tryCreateDirectory(req.output_dir);

        char* local_path = download_image_src(
            nullptr,
            req.url.c_str(),
            req.output_dir.c_str(),
            req.sequence_number);

        if (local_path) {
            result.local_path = local_path;
            result.success = true;
            free(local_path);
        }
        else {
            /* failure, check for error */
            result.success = false;

            if (html2tex_has_error()) {
                int error_code = html2tex_get_error();
                const char* error_msg = html2tex_get_error_message();

                if (error_msg && error_msg[0])
                    result.error = error_msg;
                else {
                    result.error = "Download failed"
                        " with error code: " +
                        std::to_string(error_code);
                }
            }
            else
                result.error = "Download failed"
                " (unknown reason).";
        }

        return result;
    }
}

ImageManager::ImageManager(size_t max_workers) {
    static std::once_flag curl_init_flag;
    std::call_once(curl_init_flag, []() {
        CURLcode res = curl_global_init(
            CURL_GLOBAL_DEFAULT);

        if (res != CURLE_OK) {
            throw ImageRuntimeException(
                std::string("CURL initialization failed: ") 
                + curl_easy_strerror(res),
                HTML2TEX_ERR_INTERNAL);
        }
        });

    // Determine worker count
    size_t worker_count = max_workers;
    if (worker_count == 0) {
        worker_count = std::thread::hardware_concurrency();
        if (worker_count == 0) {
            worker_count = 4; // Reasonable default
        }
    }

    // Create worker threads
    workers.reserve(worker_count);
    for (size_t i = 0; i < worker_count; ++i) {
        try {
            workers.emplace_back(
                &ImageManager::workerThread, 
                this);
        }
        catch (const std::system_error& e) {
            // Clean up any successfully created threads
            for (auto& thread : workers) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            throw ImageRuntimeException(
                std::string("Failed to create worker thread: ") + e.what(),
                HTML2TEX_ERR_INTERNAL);
        }
    }
}

ImageManager::~ImageManager() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    stop_flag = true;

    /* reject all pending promises */
    while (!task_queue.empty()) {
        auto& promise = task_queue.front().second;
        try {
            promise.set_exception(std::make_exception_ptr(
                std::runtime_error("ImageManager destroyed"
                    " while download pending")));
        }
        catch (...) {}
        task_queue.pop();
    }

    /* wake up all waiting threads */
    queue_cv.notify_all();

    /* wait for threads to finish */
    for (auto& worker : workers) {
        if (worker.joinable())
            worker.join();
    }
}

void ImageManager::workerThread() {
    while (true) {
        std::pair<DownloadRequest, std::promise<DownloadResult>> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            /* wait for task or shutdown signal */
            queue_cv.wait(lock, [this]() {
                return stop_flag || !task_queue.empty();
                });

            // Check for shutdown
            if (stop_flag && task_queue.empty()) {
                return;
            }

            /* get next task */
            task = std::move(task_queue.front());
            task_queue.pop();
            ++active_workers;
        }

        try {
            /* execute download */
            DownloadResult result = downloadSingle(task.first);

            /* set result in promise */
            task.second.set_value(std::move(result));

        }
        catch (...) {
            /* set exception in promise */
            task.second.set_exception(std::current_exception());
        }

        --active_workers;
    }
}

std::future<ImageManager::DownloadResult>
ImageManager::downloadAsync(const DownloadRequest& request) {
    if (request.url.empty())
        throw std::invalid_argument(
            "Provided URL cannot be empty.");

    if (request.output_dir.empty())
        throw std::invalid_argument(
            "Output directory cannot"
            " be empty.");

    if (request.sequence_number < 0)
        throw std::invalid_argument(
            "Sequence number must be"
            " non-negative.");

    std::promise<DownloadResult> promise;
    auto future = promise.get_future();

    std::lock_guard<std::mutex> lock(queue_mutex);

    /* check if we're shutting down */
    if (stop_flag)
        throw std::runtime_error(
            "ImageManager is shutting down.");

    /* add task to queue */
    task_queue.emplace(request,
        std::move(promise));

    /* wake up one worker thread */
    queue_cv.notify_one();

    return future;
}

ImageManager::DownloadResult
ImageManager::downloadSync(const DownloadRequest& request) {
    auto future = downloadAsync(request);
    return future.get();
}

std::vector<ImageManager::DownloadResult>
ImageManager::downloadBatch(const std::vector<DownloadRequest>& requests) {
    if (requests.empty())
        return {};

    /* start all downloads asynchronously */
    std::vector<std::future<DownloadResult>> futures;
    futures.reserve(requests.size());

    for (const auto& request : requests)
        futures.push_back(downloadAsync(request));

    /* collect results (maintains input order) */
    std::vector<DownloadResult> results;
    results.reserve(requests.size());

    for (auto& future : futures)
        results.push_back(future.get());

    return results;
}

void ImageManager::cancelAll() {
    std::lock_guard<std::mutex> lock(queue_mutex);

    while (!task_queue.empty()) {
        auto& promise = task_queue
            .front().second;

        try {
            promise.set_exception(std::make_exception_ptr(
                std::runtime_error("Download cancelled by user")));
        }
        catch (...) { }
        task_queue.pop();
    }
}

bool ImageManager::isActive() const { 
    return active_workers > 0 
        || !task_queue.empty(); 
}

void ImageManager::waitForCompletion() {
    while (isActive())
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
}