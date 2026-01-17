#include "html2tex.h"
#include <stdlib.h>
#include <string.h>

LaTeXConverter* html2tex_create(void) {
    html2tex_err_clear();
    LaTeXConverter* converter = calloc(1, sizeof(LaTeXConverter));
    HTML2TEX__CHECK_NULL(converter, HTML2TEX_ERR_NOMEM, "Converter initialization failed.");

    converter->buffer = string_buffer_create(1024);
    HTML2TEX__CHECK_NULL(converter->buffer, HTML2TEX_ERR_NOMEM, "String buffer memory allocation failed.");

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

    converter->image_output_dir = NULL;
    converter->download_images = 0;
    converter->image_counter = 0;
    converter->current_css = NULL;

    return converter;
}

LaTeXConverter* html2tex_copy(LaTeXConverter* converter) {
    html2tex_err_clear();
    HTML2TEX__CHECK_NULL(converter, HTML2TEX_ERR_NULL, "Converter initialization failed.");

    LaTeXConverter* clone = calloc(1, sizeof(LaTeXConverter));
    HTML2TEX__CHECK_NULL(clone, HTML2TEX_ERR_NOMEM, "Clone converter memory allocation failed.");

    /* copy primitive fields */
    clone->state = converter->state;
    clone->download_images = converter->download_images;
    clone->image_counter = converter->image_counter;

    /* copy string fields with duplication */
    if (converter->state.table_caption) {
        clone->state.table_caption = strdup(converter->state.table_caption);
        HTML2TEX__CHECK_NULL(clone->state.table_caption,
            HTML2TEX_ERR_NOMEM, "Table caption duplication in memory failed.");
    }

    if (converter->image_output_dir) {
        clone->image_output_dir = strdup(converter->image_output_dir);
        HTML2TEX__CHECK_NULL(clone->image_output_dir,
            HTML2TEX_ERR_NOMEM, "Image directory duplication failed.");
    }

    /* copy CSS properties if present */
    if (converter->current_css) {
        clone->current_css = css_properties_copy(converter->current_css);
        HTML2TEX__CHECK_NULL(clone->current_css,
            HTML2TEX_ERR_CSS, "CSS properties copy is NULL.");
    }

    /* copy buffer contents if present */
    if (converter->buffer && converter->buffer->data) {
        clone->buffer = string_buffer_create(converter->buffer->capacity);
        HTML2TEX__CHECK_NULL(clone->buffer, HTML2TEX_ERR_NOMEM, "String buffer is NULL.");

        if (string_buffer_append(clone->buffer,
            converter->buffer->data,
            converter->buffer->length) != 0) {
            html2tex_destroy(clone);
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer copy failed.");
            return NULL;
        }
    }
    else {
        clone->buffer = string_buffer_create(1024);
        HTML2TEX__CHECK_NULL(clone->buffer, HTML2TEX_ERR_NOMEM, "String buffer is NULL.");
    }

    return clone;
}

void html2tex_set_image_directory(LaTeXConverter* converter, const char* dir) {
    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "Converter is not initialized.");
        return;
    }

    if (converter->image_output_dir) {
        free(converter->image_output_dir);
        converter->image_output_dir = NULL;
    }

    if (dir && dir[0] != '\0') {
        converter->image_output_dir = strdup(dir);
        if (!converter->image_output_dir) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM, "Image directory memory allocation failed.");
        }
    }
}

void html2tex_set_download_images(LaTeXConverter* converter, int enable) {
    if (!converter) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL, "Converter is not initialized.");
        return;
    }
    converter->download_images = enable ? 1 : 0;
}

char* html2tex_convert(LaTeXConverter* converter, const char* html) {
    html2tex_err_clear();

    HTML2TEX__CHECK_NULL(converter, HTML2TEX_ERR_NULL, "Converter is not initialized.");
    HTML2TEX__CHECK_NULL(html, HTML2TEX_ERR_NULL, "HTML input is NULL.");

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
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Buffer clear failed because of overflow.");
            return NULL;
        }
    }
    else {
        converter->buffer = string_buffer_create(1024);
        HTML2TEX__CHECK_NULL(converter->buffer,
            HTML2TEX_ERR_NOMEM, "String buffer creation failed.");
    }

    /* initialize image download if needed */
    if (converter->download_images) {
        if (image_utils_init() != 0) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE, "Image utils init failed.");
            return NULL;
        }
    }

    /* compress the HTML code */
    char* compact_html = html2tex_compress_html(html);
    HTML2TEX__CHECK_NULL(compact_html, HTML2TEX_ERR_PARSE, "HTML compression failed.");

    /* add LaTeX preamble */
    if (string_buffer_append(converter->buffer,
        "\\documentclass{article}\n"
        "\\usepackage{hyperref}\n"
        "\\usepackage{ulem}\n"
        "\\usepackage[table]{xcolor}\n"
        "\\usepackage{tabularx}\n"
        "\\usepackage{graphicx}\n"
        "\\usepackage{placeins}\n"
        "\\setcounter{secnumdepth}{4}\n", 0) != 0) {
        free(compact_html);
        if (converter->download_images) image_utils_cleanup();
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "LaTeX preamble overflow.");
        return NULL;
    }

    /* parse HTML and extract title */
    HTMLNode* root = html2tex_parse(compact_html);
    free(compact_html);

    HTML2TEX__CHECK_NULL(root, HTML2TEX_ERR_PARSE, "Parsed HTML content failed.");

    char* title = html2tex_extract_title(root);
    int has_title = 0;

    if (title) {
        has_title = 1;
        if (string_buffer_append(converter->buffer, "\\title{", 7) != 0 ||
            string_buffer_append_latex(converter->buffer, title) != 0 ||
            string_buffer_append(converter->buffer, "}\n", 2) != 0) {
            free(title);
            html2tex_free_node(root);
            if (converter->download_images) image_utils_cleanup();
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Title addition failed.");
            return NULL;
        }
        free(title);
    }

    /* begin the document */
    if (string_buffer_append(converter->buffer,
        "\\begin{document}\n", 0) != 0) {
        html2tex_free_node(root);
        if (converter->download_images) image_utils_cleanup();
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Document begin overflow.");
        return NULL;
    }

    /* add maketitle if we have a title */
    if (has_title) {
        if (string_buffer_append(converter->buffer, "\\maketitle\n\n", 0) != 0) {
            html2tex_free_node(root);
            if (converter->download_images) image_utils_cleanup();
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Failed maketitle addition.");
            return NULL;
        }
    }

    /* convert the content */
    convert_document(converter, root);
    html2tex_free_node(root);

    /* check for conversion errors */
    if (html2tex_has_error()) {
        if (converter->download_images) image_utils_cleanup();
        return NULL;
    }

    /* end the document */
    if (string_buffer_append(converter->buffer, "\n\\end{document}\n", 0) != 0) {
        if (converter->download_images) image_utils_cleanup();
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW, "Document end overflow.");
        return NULL;
    }

    /* cleanup image download resources */
    if (converter->download_images)
        image_utils_cleanup();

    /* return the output */
    char* result = string_buffer_detach(converter->buffer);
    HTML2TEX__CHECK_NULL(result, HTML2TEX_ERR_BUF_OVERFLOW, "Buffer detach overflow.");

    return result;
}

int html2tex_get_error(void) {
    return (int)html2tex_err_get();
}

const char* html2tex_get_error_message(void) {
    return html2tex_err_msg();
}

void html2tex_destroy(LaTeXConverter* converter) {
    if (!converter) return;

    if (converter->buffer)
        string_buffer_destroy(converter->buffer);

    if (converter->current_css)
        css_properties_destroy(converter->current_css);

    if (converter->state.table_caption)
        free(converter->state.table_caption);

    if (converter->image_output_dir)
        free(converter->image_output_dir);

    free(converter);
}