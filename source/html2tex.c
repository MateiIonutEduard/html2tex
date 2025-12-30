#include "html2tex.h"
#include <stdlib.h>
#include <string.h>

LaTeXConverter* html2tex_create(void) {
    LaTeXConverter* converter = calloc(1, sizeof(LaTeXConverter));
    if (!converter) return NULL;

    /* initialize all fields */
    converter->buffer = string_buffer_create(1024);

    if (!converter->buffer) {
        free(converter);
        return NULL;
    }

    /* initialize state */
    converter->state.indent_level = 0;
    converter->state.list_level = 0;
    converter->state.in_paragraph = 0;
    converter->state.in_list = 0;
    converter->state.table_internal_counter = 0;
    converter->state.figure_internal_counter = 0;
    converter->state.image_internal_counter = 0;
    converter->state.in_table = 0;
    converter->state.in_table_row = 0;
    converter->state.in_table_cell = 0;
    converter->state.table_columns = 0;
    converter->state.current_column = 0;
    converter->state.table_caption = NULL;
    converter->state.css_braces = 0;
    converter->state.css_environments = 0;
    converter->state.applied_props = 0;
    converter->state.skip_nested_table = 0;
    converter->state.table_has_caption = 0;
    converter->state.pending_css_reset = 0;
    converter->error_code = 0;
    converter->image_output_dir = NULL;
    converter->download_images = 0;
    converter->image_counter = 0;
    converter->error_message[0] = '\0';
    converter->current_css = NULL;

    return converter;
}

LaTeXConverter* html2tex_copy(LaTeXConverter* converter) {
    if (!converter) return NULL;
    LaTeXConverter* clone = calloc(1, sizeof(LaTeXConverter));
    if (!clone) return NULL;

    /* copy primitive fields */
    clone->state = converter->state;
    clone->error_code = converter->error_code;
    clone->download_images = converter->download_images;
    clone->image_counter = converter->image_counter;

    /* copy string fields with duplication */
    if (converter->state.table_caption) {
        clone->state.table_caption = strdup(converter->state.table_caption);
    }

    if (converter->image_output_dir) {
        clone->image_output_dir = strdup(converter->image_output_dir);
    }

    /* copy CSS properties if present */
    if (converter->current_css)
        clone->current_css = css_properties_copy(converter->current_css);

    /* copy buffer contents if present */
    if (converter->buffer && converter->buffer->data) {
        clone->buffer = string_buffer_create(converter->buffer->capacity);

        if (clone->buffer) {
            if (string_buffer_append(clone->buffer,
                converter->buffer->data,
                converter->buffer->length) != 0) {
                html2tex_destroy(clone);
                return NULL;
            }
        }
    }
    else {
        /* create an empty buffer */
        clone->buffer = string_buffer_create(1024);

        if (!clone->buffer) {
            html2tex_destroy(clone);
            return NULL;
        }
    }

    /* copy the error message */
    strncpy(clone->error_message, converter->error_message,
        sizeof(clone->error_message) - 1);

    clone->error_message[sizeof(clone->error_message) - 1] = '\0';
    return clone;
}

void html2tex_set_image_directory(LaTeXConverter* converter, const char* dir) {
    if (!converter) return;

    /* free existing directory if set */
    if (converter->image_output_dir) {
        free(converter->image_output_dir);
        converter->image_output_dir = NULL;
    }

    if (dir && dir[0] != '\0')
        converter->image_output_dir = strdup(dir);
}

void html2tex_set_download_images(LaTeXConverter* converter, int enable) {
    if (converter)
        converter->download_images = enable ? 1 : 0;
}

char* html2tex_convert(LaTeXConverter* converter, const char* html) {
    if (!converter || !html) return NULL;

    /* reset the converter state */
    converter->error_code = 0;
    converter->error_message[0] = '\0';
    converter->image_counter = 0;

    if (converter->state.table_caption) {
        free(converter->state.table_caption);
        converter->state.table_caption = NULL;
    }

    /* reset CSS state */
    converter->state.applied_props = 0;
    converter->state.css_braces = 0;
    converter->state.css_environments = 0;

    /* clear the buffer for new conversion */
    if (converter->buffer) {
        if (string_buffer_clear(converter->buffer) != 0) {
            converter->error_code = 1;
            strncpy(converter->error_message,
                "Failed to clear buffer.",
                sizeof(converter->error_message) - 1);
            return NULL;
        }
    }
    else {
        converter->buffer = string_buffer_create(1024);
        if (!converter->buffer) {
            converter->error_code = 2;
            strncpy(converter->error_message,
                "Failed to create buffer.",
                sizeof(converter->error_message) - 1);
            return NULL;
        }
    }

    /* initialize image download if needed */
    if (converter->download_images)
        image_utils_init();

    /* compress the HTML code */
    char* compact_html = html2tex_compress_html(html);

    if (!compact_html) {
        converter->error_code = 3;
        strncpy(converter->error_message,
            "Failed to compress HTML.",
            sizeof(converter->error_message) - 1);
        if (converter->download_images) image_utils_cleanup();
        return NULL;
    }

    /* add LaTeX preamble using StringBuffer utilities */
    if (string_buffer_append(converter->buffer,
        "\\documentclass{article}\n"
        "\\usepackage{hyperref}\n"
        "\\usepackage{ulem}\n"
        "\\usepackage[table]{xcolor}\n"
        "\\usepackage{tabularx}\n"
        "\\usepackage{graphicx}\n"
        "\\usepackage{placeins}\n"
        "\\setcounter{secnumdepth}{4}\n", 0) != 0) {
        converter->error_code = 4;
        strncpy(converter->error_message,
            "Failed to add LaTeX preamble.",
            sizeof(converter->error_message) - 1);
        free(compact_html);
        if (converter->download_images) image_utils_cleanup();
        return NULL;
    }

    /* parse HTML and extract title */
    HTMLNode* root = html2tex_parse(compact_html);
    free(compact_html);

    if (!root) {
        converter->error_code = 5;
        strncpy(converter->error_message,
            "Failed to parse HTML.",
            sizeof(converter->error_message) - 1);
        if (converter->download_images) image_utils_cleanup();
        return NULL;
    }

    char* title = html2tex_extract_title(root);
    int has_title = 0;

    if (title) {
        has_title = 1;
        if (string_buffer_append(converter->buffer, "\\title{", 7) != 0 ||
            string_buffer_append_latex(converter->buffer, title) != 0 ||
            string_buffer_append(converter->buffer, "}\n", 2) != 0) {
            converter->error_code = 6;
            strncpy(converter->error_message,
                "Failed to add title.",
                sizeof(converter->error_message) - 1);
            free(title);
            html2tex_free_node(root);
            if (converter->download_images) image_utils_cleanup();
            return NULL;
        }

        free(title);
        title = NULL;
    }

    /* begin the document */
    if (string_buffer_append(converter->buffer,
        "\\begin{document}\n", 0) != 0) {
        converter->error_code = 7;
        strncpy(converter->error_message,
            "Failed to begin document.",
            sizeof(converter->error_message) - 1);
        html2tex_free_node(root);
        if (converter->download_images) image_utils_cleanup();
        return NULL;
    }

    /* add maketitle only if we had a title */
    if (has_title) {
        if (string_buffer_append(converter->buffer, "\\maketitle\n\n", 0) != 0) {
            converter->error_code = 8;
            strncpy(converter->error_message,
                "Failed to add maketitle.",
                sizeof(converter->error_message) - 1);
            html2tex_free_node(root);
            if (converter->download_images) image_utils_cleanup();
            return NULL;
        }
    }

    /* convert the content */
    convert_children(converter, root, NULL);
    html2tex_free_node(root);

    /* check for conversion errors */
    if (converter->error_code != 0) {
        if (converter->download_images) 
            image_utils_cleanup();
        return NULL;
    }

    /* end the document */
    if (string_buffer_append(converter->buffer, "\n\\end{document}\n", 0) != 0) {
        converter->error_code = 9;
        strncpy(converter->error_message,
            "Failed to end document.",
            sizeof(converter->error_message) - 1);
        if (converter->download_images) image_utils_cleanup();
        return NULL;
    }

    /* cleanup image download resources */
    if (converter->download_images)
        image_utils_cleanup();

    /* return the output */
    char* result = string_buffer_detach(converter->buffer);

    if (!result) {
        converter->error_code = 10;
        strncpy(converter->error_message,
            "Failed to detach buffer.",
            sizeof(converter->error_message) - 1);
    }

    return result;
}

int html2tex_get_error(const LaTeXConverter* converter) {
    return converter ? converter->error_code : -1;
}

const char* html2tex_get_error_message(const LaTeXConverter* converter) {
    return converter ? converter->error_message : "Invalid converter";
}

void html2tex_destroy(LaTeXConverter* converter) {
    if (!converter) return;

    /* free the buffer */
    if (converter->buffer)
        string_buffer_destroy(converter->buffer);

    /* free CSS properties */
    if (converter->current_css)
        css_properties_destroy(converter->current_css);

    /* free other allocated strings */
    if (converter->state.table_caption)
        free(converter->state.table_caption);

    if (converter->image_output_dir)
        free(converter->image_output_dir);

    free(converter);
}