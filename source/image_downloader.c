#include "html2tex.h"
#include <stdlib.h>
#include <string.h>

/* simplified internal structures */
typedef struct {
    char* url;
    char* output_dir;
    int sequence_number;
} QueuedDownload;

struct DownloadQueue {
    QueuedDownload* items;
    size_t capacity;
    size_t count;
    size_t head;
    mutex_t mutex;
    cond_t not_empty;
    cond_t not_full;
};

struct Worker {
    struct ImageDownloader* downloader;
    thread_t thread;
};

struct ImageDownloader {
    struct Worker* workers;
    size_t worker_count;
    struct DownloadQueue queue;

    DownloadResult** results;
    size_t results_capacity;
    size_t results_count;
    mutex_t results_mutex;

    DownloadCallback download_callback;
    BatchCompleteCallback batch_callback;
    void* user_data;

    atomic_bool stop_flag;
    atomic_size_t active_workers;
    atomic_size_t total_enqueued;
    atomic_size_t completed;
    atomic_size_t successful;

    mutex_t state_mutex;
    cond_t all_complete;
};

static bool queue_init(struct DownloadQueue* queue, size_t capacity) {
    if (capacity == 0) capacity = 64;

    queue->items = malloc(sizeof(QueuedDownload) * capacity);
    if (!queue->items) return false;

    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;

    if (mutex_init(&queue->mutex) != 0 ||
        cond_init(&queue->not_empty) != 0 ||
        cond_init(&queue->not_full) != 0) {
        free(queue->items);
        return false;
    }

    return true;
}

static void mutex_queue_destroy(struct DownloadQueue* queue) {
    if (!queue) return;
    mutex_lock(&queue->mutex);

    for (size_t i = 0; i < queue->count; i++) {
        size_t idx = (queue->head + i) % queue->capacity;
        free(queue->items[idx].url);
        free(queue->items[idx].output_dir);
    }

    mutex_unlock(&queue->mutex);
    cond_destroy(&queue->not_empty);
    cond_destroy(&queue->not_full);

    mutex_destroy(&queue->mutex);
    free(queue->items);
}

static bool queue_push(struct DownloadQueue* queue, QueuedDownload item) {
    mutex_lock(&queue->mutex);

    while (queue->count == queue->capacity)
        cond_wait(&queue->not_full, &queue->mutex);

    size_t tail = (queue->head + queue->count) % queue->capacity;
    queue->items[tail] = item;
    queue->count++;

    cond_signal(&queue->not_empty);
    mutex_unlock(&queue->mutex);
    return true;
}

static bool queue_pop(struct DownloadQueue* queue, QueuedDownload* item) {
    mutex_lock(&queue->mutex);

    while (queue->count == 0)
        cond_wait(&queue->not_empty, &queue->mutex);

    *item = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;

    cond_signal(&queue->not_full);
    mutex_unlock(&queue->mutex);
    return true;
}

static bool mutex_queue_is_empty(struct DownloadQueue* queue) {
    mutex_lock(&queue->mutex);
    bool empty = (queue->count == 0);
    mutex_unlock(&queue->mutex);
    return empty;
}

static THREAD_RETURN_TYPE worker_func(void* arg) {
    struct Worker* worker = (struct Worker*)arg;
    struct ImageDownloader* downloader = worker->downloader;
    atomic_fetch_add(&downloader->active_workers, 1);

    while (!atomic_load(&downloader->stop_flag)) {
        QueuedDownload item;

        if (!queue_pop(&downloader->queue, &item))
            break;

        if (atomic_load(&downloader->stop_flag)) {
            free(item.url);
            free(item.output_dir);
            break;
        }

        DownloadResult* result = malloc(sizeof(DownloadResult));

        if (result) {
            result->url = strdup(item.url);
            result->sequence_number = item.sequence_number;
            result->local_path = NULL;
            result->error = NULL;

            char* local_path = download_image_src(NULL, item.url,
                item.output_dir, item.sequence_number);

            if (local_path) {
                result->local_path = local_path;
                result->success = true;
                atomic_fetch_add(&downloader->successful, 1);
            }
            else {
                result->success = false;
                const char* err = html2tex_get_error_message();
                result->error = err ? strdup(err) : strdup("Image download failed.");
            }

            mutex_lock(&downloader->results_mutex);

            if (downloader->results_count >= downloader->results_capacity) {
                size_t new_cap = downloader->results_capacity * 2;
                if (new_cap == 0) new_cap = 16;

                DownloadResult** new_results = realloc(downloader->results,
                    sizeof(DownloadResult*) * new_cap);

                if (new_results) {
                    downloader->results = new_results;
                    downloader->results_capacity = new_cap;
                }
            }

            if (downloader->results_count < downloader->results_capacity) {
                downloader->results[downloader->results_count++] = result;

                /* call individual callback */
                if (downloader->download_callback)
                    downloader->download_callback(result, 
                        downloader->user_data);
            }
            else {
                free(result->url);
                free(result->error);
                free(result);
            }

            mutex_unlock(&downloader->results_mutex);
        }

        free(item.url);
        free(item.output_dir);

        size_t completed = atomic_fetch_add(&downloader->completed, 1) + 1;
        size_t total = atomic_load(&downloader->total_enqueued);

        if (completed >= total && downloader->batch_callback) {
            size_t success = atomic_load(&downloader->successful);
            downloader->batch_callback(total, success, downloader->user_data);
        }
    }

    atomic_fetch_sub(&downloader->active_workers, 1);

    if (atomic_load(&downloader->active_workers) == 0) {
        mutex_lock(&downloader->state_mutex);
        cond_broadcast(&downloader->all_complete);
        mutex_unlock(&downloader->state_mutex);
    }

    thread_exit(0);
}

ImageDownloader* image_downloader_create(size_t max_workers,
    DownloadCallback callback,
    BatchCompleteCallback batch_callback,
    void* user_data) {
    if (max_workers == 0) max_workers = get_cpu_count();
    struct ImageDownloader* downloader = calloc(1, sizeof(struct ImageDownloader));
    if (!downloader) return NULL;

    /* initialize queue */
    if (!queue_init(&downloader->queue, max_workers * 4)) {
        free(downloader);
        return NULL;
    }

    /* initialize synchronization */
    if (mutex_init(&downloader->results_mutex) != 0 ||
        mutex_init(&downloader->state_mutex) != 0 ||
        cond_init(&downloader->all_complete) != 0) {
        mutex_queue_destroy(&downloader->queue);
        free(downloader);
        return NULL;
    }

    downloader->download_callback = callback;
    downloader->batch_callback = batch_callback;
    downloader->user_data = user_data;

    atomic_store(&downloader->stop_flag, false);
    atomic_store(&downloader->active_workers, 0);
    atomic_store(&downloader->total_enqueued, 0);
    atomic_store(&downloader->completed, 0);
    atomic_store(&downloader->successful, 0);

    /* create the thread workers */
    downloader->workers = malloc(sizeof(struct Worker) 
        * max_workers);

    if (!downloader->workers) {
        image_downloader_destroy(downloader);
        return NULL;
    }

    downloader->worker_count = max_workers;

    for (size_t i = 0; i < max_workers; i++) {
        downloader->workers[i].downloader = downloader;

        if (thread_create(&downloader->workers[i].thread, worker_func,
            &downloader->workers[i]) != 0) {
            atomic_store(&downloader->stop_flag, true);
            cond_broadcast(&downloader->queue.not_empty);

            for (size_t j = 0; j < i; j++)
                thread_join(downloader->workers[j].thread);

            image_downloader_destroy(downloader);
            return NULL;
        }
    }

    if (image_utils_init() != 0) {
        image_downloader_destroy(downloader);
        return NULL;
    }

    return downloader;
}

bool image_downloader_enqueue(ImageDownloader* downloader,
    const char* url,
    const char* output_dir,
    int sequence_number) {

    if (!downloader || !url 
        || !output_dir) 
        return false;

    QueuedDownload item = {
        .url = strdup(url),
        .output_dir = strdup(output_dir),
        .sequence_number = sequence_number
    };

    if (!item.url || !item.output_dir) {
        free(item.url);
        free(item.output_dir);
        return false;
    }

    if (!queue_push(&downloader->queue, item)) {
        free(item.url);
        free(item.output_dir);
        return false;
    }

    atomic_fetch_add(&downloader->total_enqueued, 1);
    return true;
}

bool image_downloader_start(ImageDownloader* downloader) {
    if (!downloader) return false;
    cond_broadcast(&downloader->queue.not_empty);
    return true;
}

bool image_downloader_wait(ImageDownloader* downloader, unsigned int timeout_ms) {
    if (!downloader) return false;
    mutex_lock(&downloader->state_mutex);

    while (atomic_load(&downloader->completed) <
        atomic_load(&downloader->total_enqueued) &&
        atomic_load(&downloader->active_workers) > 0) {

        if (timeout_ms == 0)
            cond_wait(&downloader->all_complete, 
                &downloader->state_mutex);
        else {
            if (cond_timedwait(&downloader->all_complete,
                &downloader->state_mutex, timeout_ms) != 0) {
                mutex_unlock(&downloader->state_mutex);
                return false;
            }
        }
    }

    mutex_unlock(&downloader->state_mutex);
    return true;
}

size_t image_downloader_cancel(ImageDownloader* downloader) {
    if (!downloader) return 0;
    atomic_store(&downloader->stop_flag, true);
    cond_broadcast(&downloader->queue.not_empty);
    size_t cancelled = 0;

    while (!mutex_queue_is_empty(&downloader->queue)) {
        QueuedDownload item;

        if (queue_pop(&downloader->queue, &item)) {
            free(item.url);
            free(item.output_dir);
            cancelled++;
        }
    }

    return cancelled;
}

DownloadResult** image_downloader_get_results(ImageDownloader* downloader, size_t* count) {
    if (!downloader || !count) return NULL;
    mutex_lock(&downloader->results_mutex);

    *count = downloader->results_count;
    DownloadResult** results = downloader->results;

    downloader->results = NULL;
    downloader->results_count = 0;
    downloader->results_capacity = 0;

    mutex_unlock(&downloader->results_mutex);
    return results;
}

void image_downloader_destroy(ImageDownloader* downloader) {
    if (!downloader) return;
    image_downloader_cancel(downloader);

    for (size_t i = 0; i < downloader->worker_count; i++)
        thread_join(downloader->workers[i].thread);

    mutex_queue_destroy(&downloader->queue);
    mutex_lock(&downloader->results_mutex);

    for (size_t i = 0; i < downloader->results_count; i++)
        download_result_free(downloader->results[i]);
    
    free(downloader->results);
    mutex_unlock(&downloader->results_mutex);

    mutex_destroy(&downloader->results_mutex);
    mutex_destroy(&downloader->state_mutex);
    cond_destroy(&downloader->all_complete);

    free(downloader->workers);
    free(downloader);
    image_utils_cleanup();
}