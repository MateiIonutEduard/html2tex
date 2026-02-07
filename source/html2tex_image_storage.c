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