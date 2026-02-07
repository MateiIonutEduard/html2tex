#include "html2tex.h"

ImageStorage* create_image_storage() {
    /* clear any previous error state */
    html2tex_err_clear();

    ImageStorage* store = (ImageStorage*)malloc(sizeof(ImageStorage));

    if (!store) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate %zu bytes "
            "for ImageStorage structure.",
            sizeof(ImageStorage));
        return NULL;
    }

    /* initialize with safe defaults */
    store->lazy_downloading = 0;
    store->image_stack = NULL;
    return store;
}

int clear_image_storage(ImageStorage* store) {
    /* clear any existing error state */
    html2tex_err_clear();

    if (store) {
        Stack* image_stack = store->image_stack;
        size_t count = 0;

        char** filenames = (char**)stack_to_array(&image_stack, &count);
        int errorThrown = html2tex_has_error();

        /* propagate the existing error forward */
        if (errorThrown) {
            /* force memory clean up first */
            stack_destroy(&image_stack, &free);
            return -1;
        }

        /* transferring data ownership succeed */
        for (size_t i = 0; i < count; i++)
            free(filenames[i]);

        free(filenames);
        return 1;
    }

    return 0;
}

ImageStorage* copy_image_storage(const ImageStorage* store) {
    /* clear any existing error state */
    html2tex_err_clear();

    /* validate input parameter */
    if (!store) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Source ImageStorage is NULL "
            "for copy operation.");
        return NULL;
    }

    /* create destination storage */
    ImageStorage* new_store = create_image_storage();
    if (!new_store) return NULL;

    /* copy lazy downloading flag */
    new_store->lazy_downloading = store->lazy_downloading;

    /* handle empty stack case efficiently */
    if (!store->image_stack || stack_is_empty(store->image_stack))
        return new_store;

    /* get stack size and allocate filename array */
    size_t count = stack_size(store->image_stack);
    if (count == 0) return new_store;

    /* cap count to prevent potential overflow issues */
    const size_t MAX_REASONABLE_FILES = 1000000;

    if (count > MAX_REASONABLE_FILES) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "Image stack too large for copy: %zu "
            "files.", count);
        destroy_image_storage(new_store);
        return NULL;
    }

    char** filenames = (char**)malloc(count * sizeof(char*));
    if (!filenames) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate %zu bytes for"
            " filename array during ImageStorage copy.",
            count * sizeof(char*));
        destroy_image_storage(new_store);
        return NULL;
    }

    /* initialize array to NULL for safe cleanup */
    memset(filenames, 0, count * sizeof(char*));

    /* traverse stack and duplicate strings with error checking */
    const Stack* current = store->image_stack;
    size_t index = 0;
    int allocation_failed = 0;

    /* explicit bounds check */
    while (current && index < count && !allocation_failed) {
        const char* original = (const char*)current->data;

        if (original) {
            char* copy = strdup(original);

            if (!copy) {
                allocation_failed = 1;
                break;
            }

            filenames[index++] = copy;
        }

        current = current->next;
    }

    /* verify it processed exactly count elements */
    if (current != NULL) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_INTERNAL,
            "Stack size changed during ImageStorage copy.");
        allocation_failed = 1;
    }

    /* handle allocation failure mid-copy */
    if (allocation_failed) {
        for (size_t i = 0; i < index && i < count; i++) {
            if (filenames[i])
                free(filenames[i]);
        }

        free(filenames);
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to duplicate filename "
            "string during ImageStorage copy.");

        destroy_image_storage(new_store);
        return NULL;
    }

    /* push strings onto new stack maintaining original order */
    for (size_t i = 0; i < index; i++) {
        if (!stack_push(&new_store->image_stack, filenames[i])) {
            /* cleanup already pushed items */
            while (!stack_is_empty(new_store->image_stack)) {
                char* item = (char*)stack_pop(
                    &new_store->image_stack);
                if (item) free(item);
            }

            /* cleanup remaining filenames */
            for (size_t j = i; j < index; j++) {
                if (filenames[j]) 
                    free(filenames[j]);
            }

            free(filenames);
            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                "Failed to push filename onto "
                "stack during ImageStorage copy.");

            destroy_image_storage(new_store);
            return NULL;
        }
    }

    /* free temporary array */
    free(filenames);
    return new_store;
}

void destroy_image_storage(ImageStorage* store) {
    /* clear any existing error state */
    html2tex_err_clear();
    if (!store) return;

    store->lazy_downloading = 0;
    clear_image_storage(store);
    free(store);
}

int html2tex_enable_downloads(ImageStorage** storage, int enable) {
    /* clear any existing error state */
    html2tex_err_clear();

    if (!storage) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Storage pointer is NULL for "
            "download control.");
        return 0;
    }

    ImageStorage* store = *storage;

    /* create a new container */
    ImageStorage* new_store = create_image_storage();
    if (!new_store) return 0;
    new_store->lazy_downloading = enable;

    if (enable) {
        /* destroy existing one */
        if (store != NULL)
            destroy_image_storage(store);

        *storage = new_store;
    }
    else {
        /* check if container is NULL */
        if (store != NULL) {
            if (stack_is_empty(store->image_stack)) {
                destroy_image_storage(new_store);
                store->lazy_downloading = enable;
            }
            else {
                destroy_image_storage(store);
                *storage = new_store;
            }
        }
        else
            destroy_image_storage(new_store);
    }

    return 1;
}

int html2tex_add_image(ImageStorage** storage, const char* file_path) {
    /* clear any existing error state */
    html2tex_err_clear();

    /* validate input parameters */
    if (!storage) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "ImageStorage pointer is NULL "
            "for image addition.");
        return -1;
    }

    if (!file_path) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "File path is NULL for image addition.");
        return -1;
    }

    if (file_path[0] == '\0') {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_INVAL,
            "File path is empty string for "
            "image addition.");
        return -1;
    }

    ImageStorage* store = *storage;

    /* check if deferred downloading is enabled */
    if (!store || (store && !store->lazy_downloading))
        return 0;

    /* duplicate file path with bounds checking */
    size_t path_len = strlen(file_path);

    if (path_len > SIZE_MAX) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "File path length %zu exceeds maximum allowed %d.",
            path_len, SIZE_MAX);
        return -1;
    }

    char* full_path = strdup(file_path);
    if (!full_path) return -1;

    /* push onto image stack with error handling */
    if (!stack_push(&store->image_stack, full_path)) {
        /* clean up allocated path */
        free(full_path);

        /* check if error was already set */
        if (!html2tex_has_error()) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                "Failed to push image path onto"
                " storage stack.");
        }

        return -1;
    }

    return 1;
}