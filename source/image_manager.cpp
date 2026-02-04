#include "ext/image_manager.hpp"
#include "html2tex.h"
#include <cstring>
#include <curl/curl.h>
#include <system_error>

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
     * @brief Creates directory recursively. (cross-platform)
     * @param path Directory path to create
     * @return 0 on success, -1 on error with errno set
     */
    int create_directory_recursive(const std::string& path) {
        if (path.empty()) {
            errno = EINVAL;
            return -1;
        }

        if (mkdir(path.c_str()) == 0)
            return 0;

        if (errno == ENOENT) {
            size_t pos = path.find_last_of("/\\");
            if (pos == std::string::npos)
                return -1;

            std::string parent = path.substr(0, pos);
            if (parent.empty()) return -1;

            if (create_directory_recursive(parent) != 0) {
                return -1;

                return mkdir(path.c_str());
            }

            if (errno == EEXIST) {
#ifdef _WIN32
                DWORD attrib = GetFileAttributesA(path.c_str());
                if (attrib != INVALID_FILE_ATTRIBUTES &&
                    (attrib & FILE_ATTRIBUTE_DIRECTORY)) {
                    return 0;
                }
#else
                struct stat st;
                if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                    return 0;
                }
#endif
            }

            return -1;
        }
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

        try {
            if (req.output_dir.empty()) {
                throw ImageRuntimeException(
                    "Output directory is empty.",
                    HTML2TEX_ERR_INVAL);
            }

            if (req.url.empty()) {
                throw ImageRuntimeException(
                    "Provided URL is empty.",
                    HTML2TEX_ERR_INVAL);
            }

            if (create_directory_recursive(req.output_dir) != 0) {
                throw ImageRuntimeException::fromFileError(
                    req.output_dir,
                    "Create directory failed.",
                    errno);
            }

            char* local_path = download_image_src(
                req.url.c_str(),
                req.output_dir.c_str(),
                req.sequence_number);

            if (!local_path) {
                if (html2tex_has_error())
                    throw ImageRuntimeException::fromImageError();

                throw ImageRuntimeException(
                    "download_image_src() function returned"
                    " NULL without setting error.",
                    HTML2TEX_ERR_IMAGE_DOWNLOAD);
            }

            result.local_path = local_path;
            result.success = true;
            free(local_path);

        }
        catch (const ImageRuntimeException& e) {
            result.success = false;
            result.error = e.what();
        }
        catch (const std::exception& e) {
            result.success = false;
            result.error = std::string("Unexpected error: ") + e.what();
        }
        catch (...) {
            result.success = false;
            result.error = "Unknown exception"
                " during download.";
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