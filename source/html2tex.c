#include "html2tex.h"
#include <stdlib.h>
#include <string.h>

LaTeXConverter* html2tex_create(void) {
    LaTeXConverter* converter = malloc(sizeof(LaTeXConverter));
    if (!converter) return NULL;

    converter->output = NULL;
    converter->output_size = 0;

    converter->output_capacity = 0;
    converter->state.indent_level = 0;

    converter->state.list_level = 0;
    converter->state.in_paragraph = 0;

    converter->state.in_list = 0;
    converter->state.table_internal_counter = 0;

    converter->state.image_internal_counter = 0;
    converter->state.figure_internal_counter = 0;

    converter->state.in_table = 0;
    converter->state.in_table_row = 0;

    converter->state.in_table_cell = 0;
    converter->state.table_columns = 0;

    converter->state.current_column = 0;
    converter->state.table_caption = NULL;

    /* initialize CSS state tracking */
    converter->state.css_braces = 0;
    converter->state.css_environments = 0;

    converter->state.applied_props = 0;
    converter->state.skip_nested_table = 0;

    converter->state.table_has_caption = 0;
    converter->state.pending_css_reset = 0;
    converter->error_code = 0;

    /* initialize image configuration */
    converter->image_output_dir = NULL;
    converter->download_images = 0;
    converter->image_counter = 0;

    converter->error_message[0] = '\0';
    return converter;
}

LaTeXConverter* html2tex_copy(LaTeXConverter* converter) {
    if (!converter) return NULL;
    LaTeXConverter* clone = malloc(sizeof(LaTeXConverter));

    if (!clone) return NULL;
    clone->output = converter->output ? strdup(converter->output) : NULL;
    clone->output_size = converter->output_size;

    clone->output_capacity = converter->output_capacity;
    clone->state.indent_level = converter->state.indent_level;

    clone->state.list_level = converter->state.list_level;
    clone->state.in_paragraph = converter->state.in_paragraph;

    clone->state.in_list = converter->state.in_list;
    clone->state.table_internal_counter = converter->state.table_internal_counter;

    clone->state.figure_internal_counter = converter->state.figure_internal_counter;
    clone->state.image_internal_counter = converter->state.image_internal_counter;

    clone->state.in_table = converter->state.in_table;
    clone->state.in_table_row = converter->state.in_table_row;

    clone->state.in_table_cell = converter->state.in_table_cell;
    clone->state.table_columns = converter->state.table_columns;

    clone->state.current_column = converter->state.current_column;
    clone->state.table_caption = converter->state.table_caption ? 
        strdup(converter->state.table_caption) : NULL;

    /* copy of CSS state tracking */
    clone->state.css_braces = converter->state.css_braces;
    clone->state.css_environments = converter->state.css_environments;

    clone->state.applied_props = converter->state.applied_props;
    clone->state.skip_nested_table = converter->state.skip_nested_table;

    clone->state.table_has_caption = converter->state.table_has_caption;
    clone->state.pending_css_reset = converter->state.pending_css_reset;
    clone->error_code = converter->error_code;

    /* copy image configuration */
    clone->image_output_dir = converter->image_output_dir ? strdup(converter->image_output_dir) : NULL;
    clone->download_images = converter->download_images;
    clone->image_counter = converter->image_counter;

    /* copy error message safely */
    if (converter->error_message[0] != '\0') {
        strncpy(clone->error_message, converter->error_message, sizeof(clone->error_message) - 1);
        clone->error_message[sizeof(clone->error_message) - 1] = '\0';
    }
    else clone->error_message[0] = '\0';
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
    if (converter->download_images) image_utils_init();

    if (converter->output) {
        free(converter->output);
        converter->output = NULL;
    }

    converter->output_size = 0;
    converter->output_capacity = 0;

    converter->error_code = 0;
    converter->error_message[0] = '\0';

    if (converter->state.table_caption) {
        free(converter->state.table_caption);
        converter->state.table_caption = NULL;
    }

    /* reset CSS state */
    converter->state.applied_props = 0;
    converter->state.css_braces = 0;
    converter->state.css_environments = 0;

    append_string(converter, "\\documentclass{article}\n");
    append_string(converter, "\\usepackage{hyperref}\n");

    append_string(converter, "\\usepackage{ulem}\n");
    append_string(converter, "\\usepackage[table]{xcolor}\n");

    append_string(converter, "\\usepackage{tabularx}\n");
    append_string(converter, "\\usepackage{graphicx}\n");

    append_string(converter, "\\usepackage{placeins}\n");
    append_string(converter, "\\setcounter{secnumdepth}{4}\n");

    HTMLNode* root = html2tex_parse(html);
    char* title = html2tex_extract_title(root);
    int has_title = 0;

    if (title) {
        append_string(converter, "\\title{");
        append_string(converter, title);
        append_string(converter, "}\n");
        free(title); has_title = 1;
    }

    append_string(converter, "\\begin{document}\n");
    if(has_title) append_string(converter, "\\maketitle\n\n");

    if (root) {
        convert_children(converter, root, NULL);
        html2tex_free_node(root);
    }
    else {
        converter->error_code = 1;
        strcpy(converter->error_message, "Failed to parse HTML.");
        return NULL;
    }

    append_string(converter, "\n\\end{document}\n");
    if (converter->download_images) image_utils_cleanup();
    char* result = (char*)malloc(converter->output_size + 1);

    if (result) {
        if (converter->output && converter->output_size > 0) {
            memcpy(result, converter->output, converter->output_size);
            result[converter->output_size] = '\0';
        }
        else
            result[0] = '\0';
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

    /* free table caption if it exists */
    if (converter->state.table_caption)
        free(converter->state.table_caption);

    /* free image directory */
    if (converter->image_output_dir)
        free(converter->image_output_dir);

    if (converter->output)
        free(converter->output);

    free(converter);
}