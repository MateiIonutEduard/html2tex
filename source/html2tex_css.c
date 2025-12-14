#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

CSSProperties* css_properties_create(void) {
    CSSProperties* props = (CSSProperties*)calloc(1, sizeof(CSSProperties));
    if (!props) return NULL;

    props->head = NULL;
    props->tail = NULL;
    props->count = 0;
    props->mask = 0;

    return props;
}

void css_properties_destroy(CSSProperties* props) {
    if (!props) return;
    CSSProperty* current = props->head;

    while (current) {
        CSSProperty* next = current->next;
        free((void*)current->key);
        free(current->value);

        free(current);
        current = next;
    }

    free(props);
}

static CSSPropertyMask property_to_mask(const char* key) {
    if (!key) return 0;

    if (strcmp(key, "font-weight") == 0) return CSS_BOLD;
    if (strcmp(key, "font-style") == 0) return CSS_ITALIC;

    if (strcmp(key, "text-decoration") == 0) return CSS_UNDERLINE;
    if (strcmp(key, "color") == 0) return CSS_COLOR;

    if (strcmp(key, "background-color") == 0) return CSS_BACKGROUND;
    if (strcmp(key, "font-family") == 0) return CSS_FONT_FAMILY;
    if (strcmp(key, "font-size") == 0) return CSS_FONT_SIZE;

    if (strcmp(key, "text-align") == 0) return CSS_TEXT_ALIGN;
    if (strcmp(key, "border") == 0) return CSS_BORDER;

    return 0;
}

int css_properties_set(CSSProperties* props, const char* key, const char* value) {
    if (!props || !key || !value) return 0;

    /* clean value by removing !important token */
    char* cleaned_value = (char*)malloc(strlen(value) + 1);
    if (!cleaned_value) return 0;

    char* dest = cleaned_value;
    const char* src = value;
    int in_important = 0;

    while (*src) {
        if (*src == '!' && strncasecmp(src, "!important", 9) == 0) {
            in_important = 1; src += 9;
            continue;
        }

        if (!isspace(*src) || !in_important)
            *dest++ = *src;
        
        src++;
    }
    *dest = '\0';

    /* trim trailing whitespace */
    while (dest > cleaned_value && isspace(*(dest - 1)))
        *(--dest) = '\0';

    /* check if property already exists */
    CSSProperty* current = props->head;
    CSSProperty* prev = NULL;

    while (current) {
        /* update existing value */
        if (strcasecmp(current->key, key) == 0) {
            free(current->value);
            current->value = cleaned_value;

            /* update mask */
            CSSPropertyMask mask = property_to_mask(key);
            if (mask) props->mask |= mask;
            return 1;
        }

        prev = current;
        current = current->next;
    }

    /* create new property */
    CSSProperty* new_prop = (CSSProperty*)malloc(sizeof(CSSProperty));

    if (!new_prop) {
        free(cleaned_value);
        return 0;
    }

    new_prop->key = strdup(key);

    if (!new_prop->key) {
        free(new_prop);
        free(cleaned_value);
        return 0;
    }

    new_prop->value = cleaned_value;
    new_prop->next = NULL;

    /* add to list */
    if (!props->head) {
        props->head = new_prop;
        props->tail = new_prop;
    }
    else {
        props->tail->next = new_prop;
        props->tail = new_prop;
    }

    props->count++;

    /* update mask */
    CSSPropertyMask mask = property_to_mask(key);
    if (mask) props->mask |= mask;
    return 1;
}

const char* css_properties_get(const CSSProperties* props, const char* key) {
    if (!props || !key) return NULL;
    CSSProperty* current = props->head;

    while (current) {
        if (strcasecmp(current->key, key) == 0)
            return current->value;
        
        current = current->next;
    }

    return NULL;
}

int css_properties_has(const CSSProperties* props, const char* key) {
    if (!props || !key) return 0;
    CSSProperty* current = props->head;

    while (current) {
        if (strcasecmp(current->key, key) == 0) return 1;
        current = current->next;
    }

    return 0;
}

CSSProperties* css_properties_merge(const CSSProperties* parent,
    const CSSProperties* child) {
    if (!child) return NULL;

    /* create merged properties */
    CSSProperties* result = css_properties_create();
    if (!result) return NULL;

    /* first, copy all inheritable properties from parent */
    if (parent) {
        CSSProperty* current = parent->head;

        while (current) {
            if (is_css_property_inheritable(current->key)) {
                css_properties_set(result, current->key, current->value);
            }

            current = current->next;
        }
    }

    /* then, override with child properties */
    CSSProperty* current = child->head;

    while (current) {
        css_properties_set(result, current->key, current->value);
        current = current->next;
    }

    return result;
}

CSSProperties* parse_css_style(const char* style_str) {
    if (!style_str) return NULL;
    CSSProperties* props = css_properties_create();

    if (!props) return NULL;
    char* copy = strdup(style_str);

    if (!copy) {
        css_properties_destroy(props);
        return NULL;
    }

    char* token = strtok(copy, ";");

    while (token) {
        /* find colon separating property and value */
        char* colon = strchr(token, ':');

        if (colon) {
            *colon = '\0';

            /* trim property name */
            char* prop_start = token;

            while (*prop_start && isspace(*prop_start)) prop_start++;
            char* prop_end = prop_start + strlen(prop_start) - 1;

            while (prop_end > prop_start && isspace(*prop_end)) {
                *prop_end = '\0';
                prop_end--;
            }

            /* trim value */
            char* value_start = colon + 1;

            while (*value_start && isspace(*value_start)) value_start++;
            char* value_end = value_start + strlen(value_start) - 1;

            while (value_end > value_start && isspace(*value_end)) {
                *value_end = '\0';
                value_end--;
            }

            if (*prop_start && *value_start)
                css_properties_set(props, prop_start, value_start);
        }

        token = strtok(NULL, ";");
    }

    free(copy);
    return props;
}

static void apply_text_alignment(LaTeXConverter* converter, const char* align) {
    if (!converter || !align) return;

    if (strcmp(align, "center") == 0) {
        append_string(converter, "\\begin{center}\n");
        converter->state.css_environments |= 1;
    }
    else if (strcmp(align, "right") == 0) {
        append_string(converter, "\\begin{flushright}\n");
        converter->state.css_environments |= 2;
    }
    else if (strcmp(align, "left") == 0) {
        append_string(converter, "\\begin{flushleft}\n");
        converter->state.css_environments |= 4;
    }
    else if (strcmp(align, "justify") == 0) {
        append_string(converter, "\\justifying\n");
        converter->state.css_environments |= 8;
    }
}

static void apply_font_weight(LaTeXConverter* converter, const char* weight) {
    if (!converter || !weight) return;

    if (strcmp(weight, "bold") == 0 || strcmp(weight, "bolder") == 0 ||
        atoi(weight) >= 600) {
        if (!(converter->state.applied_props & CSS_BOLD)) {
            append_string(converter, "\\textbf{");
            converter->state.css_braces++;
            converter->state.applied_props |= CSS_BOLD;
        }
    }
    else if (strcmp(weight, "lighter") == 0 || atoi(weight) <= 300) {
        append_string(converter, "\\textmd{");
        converter->state.css_braces++;
    }
}

static void apply_font_style(LaTeXConverter* converter, const char* style) {
    if (!converter || !style) return;

    if (strcmp(style, "italic") == 0) {
        if (!(converter->state.applied_props & CSS_ITALIC)) {
            append_string(converter, "\\textit{");
            converter->state.css_braces++;
            converter->state.applied_props |= CSS_ITALIC;
        }
    }
    else if (strcmp(style, "oblique") == 0) {
        append_string(converter, "\\textsl{");
        converter->state.css_braces++;
    }
    else if (strcmp(style, "normal") == 0) {
        append_string(converter, "\\textup{");
        converter->state.css_braces++;
    }
}

static void apply_text_decoration(LaTeXConverter* converter, const char* decoration) {
    if (!converter || !decoration) return;

    if (strstr(decoration, "underline")) {
        if (!(converter->state.applied_props & CSS_UNDERLINE)) {
            append_string(converter, "\\underline{");
            converter->state.css_braces++;
            converter->state.applied_props |= CSS_UNDERLINE;
        }
    }

    if (strstr(decoration, "line-through")) {
        append_string(converter, "\\sout{");
        converter->state.css_braces++;
    }

    if (strstr(decoration, "overline")) {
        append_string(converter, "\\overline{");
        converter->state.css_braces++;
    }
}

static void apply_font_family(LaTeXConverter* converter, const char* family) {
    if (!converter || !family) return;

    if (strstr(family, "monospace") || strstr(family, "Courier")) {
        if (!(converter->state.applied_props & CSS_FONT_FAMILY)) {
            append_string(converter, "\\texttt{");
            converter->state.css_braces++;
            converter->state.applied_props |= CSS_FONT_FAMILY;
        }
    }
    else if (strstr(family, "sans") || strstr(family, "Arial") ||
        strstr(family, "Helvetica")) {
        if (!(converter->state.applied_props & CSS_FONT_FAMILY)) {
            append_string(converter, "\\textsf{");
            converter->state.css_braces++;
            converter->state.applied_props |= CSS_FONT_FAMILY;
        }
    }
    else if (strstr(family, "serif") || strstr(family, "Times")) {
        if (!(converter->state.applied_props & CSS_FONT_FAMILY)) {
            append_string(converter, "\\textrm{");
            converter->state.css_braces++;
            converter->state.applied_props |= CSS_FONT_FAMILY;
        }
    }
}

static void apply_font_size(LaTeXConverter* converter, const char* size) {
    if (!converter || !size) return;
    int pt = css_length_to_pt(size);

    if (pt > 0) {
        append_string(converter, "{");

        if (pt <= 8) append_string(converter, "\\tiny ");
        else if (pt <= 10) append_string(converter, "\\small ");
        else if (pt <= 12) append_string(converter, "\\normalsize ");
        else if (pt <= 14) append_string(converter, "\\large ");
        else if (pt <= 18) append_string(converter, "\\Large ");
        else if (pt <= 24) append_string(converter, "\\LARGE ");
        else append_string(converter, "\\huge ");

        converter->state.css_braces++;
        converter->state.applied_props |= CSS_FONT_SIZE;
    }
}

void css_properties_apply(LaTeXConverter* converter, const CSSProperties* props, const char* tag_name) {
    if (!converter || !props) return;
    int inside_table_cell = converter->state.in_table_cell;
    int is_block = is_block_element(tag_name);

    /* apply text alignment first, for block elements */
    const char* text_align = css_properties_get(props, "text-align");

    if (text_align && is_block && !inside_table_cell)
        apply_text_alignment(converter, text_align);

    /* apply colors */
    const char* color = css_properties_get(props, "color");

    if (color && !(converter->state.applied_props & CSS_COLOR)) {
        char* hex_color = css_color_to_hex(color);

        if (hex_color && strcmp(hex_color, "000000") != 0) {
            append_string(converter, "\\textcolor[HTML]{");
            append_string(converter, hex_color);
            append_string(converter, "}{");

            converter->state.css_braces++;
            converter->state.applied_props |= CSS_COLOR;
        }

        free(hex_color);
    }

    /* apply background color */
    const char* bg_color = css_properties_get(props, "background-color");

    if (bg_color && !(converter->state.applied_props & CSS_BACKGROUND)) {
        char* hex_color = css_color_to_hex(bg_color);

        if (hex_color && strcmp(hex_color, "FFFFFF") != 0) {
            if (converter->state.in_table_cell)
                append_string(converter, "\\cellcolor[HTML]{");
            else
                append_string(converter, "\\colorbox[HTML]{");

            append_string(converter, hex_color);
            append_string(converter, "}{");

            converter->state.css_braces++;
            converter->state.applied_props |= CSS_BACKGROUND;
        }

        free(hex_color);
    }

    /* apply font properties */
    const char* font_weight = css_properties_get(props, "font-weight");
    if (font_weight) apply_font_weight(converter, font_weight);

    const char* font_style = css_properties_get(props, "font-style");
    if (font_style) apply_font_style(converter, font_style);

    const char* font_family = css_properties_get(props, "font-family");
    if (font_family) apply_font_family(converter, font_family);

    const char* font_size = css_properties_get(props, "font-size");
    if (font_size) apply_font_size(converter, font_size);

    /* apply text decoration */
    const char* text_decoration = css_properties_get(props, "text-decoration");
    if (text_decoration) apply_text_decoration(converter, text_decoration);

    /* apply border */
    const char* border = css_properties_get(props, "border");

    if (border && strstr(border, "solid")) {
        if (!(converter->state.applied_props & CSS_BORDER)) {
            append_string(converter, "\\framebox{");
            converter->state.css_braces++;
            converter->state.applied_props |= CSS_BORDER;
        }
    }
}

void css_properties_end(LaTeXConverter* converter, const CSSProperties* props, const char* tag_name) {
    if (!converter) return;

    /* close all opened braces */
    for (int i = 0; i < (int)converter->state.css_braces; i++)
        append_string(converter, "}");

    converter->state.css_braces = 0;

    /* close alignment environments */
    if (converter->state.css_environments & 1)
        append_string(converter, "\\end{center}\n");
    else if (converter->state.css_environments & 2)
        append_string(converter, "\\end{flushright}\n");
    else if (converter->state.css_environments & 4)
        append_string(converter, "\\end{flushleft}\n");

    converter->state.css_environments = 0;

    /* reset applied properties mask */
    converter->state.applied_props = 0;
}

int css_length_to_pt(const char* length_str) {
    if (!length_str) return 0;

    char* cleaned = (char*)malloc(strlen(length_str) + 1);
    if (!cleaned) return 0;

    char* dest = cleaned;
    const char* src = length_str;

    while (*src) {
        if (*src == '!' && strncasecmp(src, "!important", 9) == 0) {
            src += 9;
            continue;
        }
        *dest++ = *src++;
    }
    *dest = '\0';

    double value;
    char unit[10] = "";

    if (sscanf(cleaned, "%lf%s", &value, unit) < 1) {
        free(cleaned);
        return 0;
    }

    int result = 0;

    if (strcmp(unit, "px") == 0)
        result = (int)(value * 72.0 / 96.0);
    else if (strcmp(unit, "pt") == 0)
        result = (int)value;
    else if (strcmp(unit, "em") == 0 || strcmp(unit, "rem") == 0)
        result = (int)(value * 10.0);
    else if (strcmp(unit, "%") == 0)
        result = (int)(value * 0.01 * 400);
    else if (strcmp(unit, "cm") == 0)
        result = (int)(value * 28.346);
    else if (strcmp(unit, "mm") == 0)
        result = (int)(value * 2.8346);
    else if (strcmp(unit, "in") == 0)
        result = (int)(value * 72.0);
    else
        result = (int)value;

    free(cleaned);
    return result;
}

char* css_color_to_hex(const char* color_value) {
    if (!color_value) return NULL;
    char* cleaned = (char*)malloc(strlen(color_value) + 1);
    if (!cleaned) return NULL;

    char* dest = cleaned;
    const char* src = color_value;

    while (*src) {
        if (*src == '!' && strncasecmp(src, "!important", 9) == 0) {
            src += 9;
            continue;
        }

        *dest++ = *src++;
    }

    *dest = '\0';
    char* result = NULL;

    if (cleaned[0] == '#') {
        if (strlen(cleaned) == 4) {
            result = (char*)malloc(7);

            if (result) {
                snprintf(result, 7, "%c%c%c%c%c%c",
                    cleaned[1], cleaned[1],
                    cleaned[2], cleaned[2],
                    cleaned[3], cleaned[3]);
            }
        }
        else
            result = strdup(cleaned + 1);
    }
    else if (strncmp(cleaned, "rgb(", 4) == 0) {
        int r, g, b;

        if (sscanf(cleaned, "rgb(%d, %d, %d)", &r, &g, &b) == 3) {
            result = (char*)malloc(7);
            if (result) snprintf(result, 7, "%02X%02X%02X", r, g, b);
        }
    }
    else if (strncmp(cleaned, "rgba(", 5) == 0) {
        int r, g, b;
        float a;

        if (sscanf(cleaned, "rgba(%d, %d, %d, %f)", &r, &g, &b, &a) == 4) {
            result = (char*)malloc(7);
            if (result) snprintf(result, 7, "%02X%02X%02X", r, g, b);
        }
    }
    else {
        static const struct {
            const char* name;
            const char* hex;
        } color_map[] = {
            {"black", "000000"}, {"white", "FFFFFF"},
            {"red", "FF0000"}, {"green", "008000"},
            {"blue", "0000FF"}, {"yellow", "FFFF00"},
            {"cyan", "00FFFF"}, {"magenta", "FF00FF"},
            {"gray", "808080"}, {"grey", "808080"},
            {"silver", "C0C0C0"}, {"maroon", "800000"},
            {"olive", "808000"}, {"lime", "00FF00"},
            {"aqua", "00FFFF"}, {"teal", "008080"},
            {"navy", "000080"}, {"fuchsia", "FF00FF"},
            {"purple", "800080"}, {"orange", "FFA500"},
            {"transparent", "FFFFFF"},
            {NULL, NULL}
        };

        for (int i = 0; color_map[i].name; i++) {
            if (strcasecmp(cleaned, color_map[i].name) == 0) {
                result = strdup(color_map[i].hex);
                break;
            }
        }

        if (!result)
            result = strdup("000000");
    }

    free(cleaned);

    if (result) {
        for (char* p = result; *p; p++)
            *p = toupper(*p);
    }

    return result;
}

int is_css_property_inheritable(const char* property_name) {
    static const struct {
        const char* property_name;
        unsigned char first_char;
        const unsigned char length;
    } inheritable_properties[] = {
        {"font-weight", 'f', 11}, {"font-style", 'f', 10}, {"font-family", 'f', 11},
        {"font-size", 'f', 9}, {"color", 'c', 5}, {"text-align", 't', 10},
        {"text-decoration", 't', 15}, {NULL, 0, 0}
    };

    size_t len = 0;
    const char* p = property_name;

    while (*p) {
        len++; p++;

        /* CSS property name is longer then expected */
        if (len > 15) return 0;
    }

    /* length-based fast rejection */
    switch (len) {
    case 5:  case 9:  case 10:
    case 11: case 15:
        break;
    default:
        /* length does not match any known CSS property */
        return 0;
    }

    /* extract first char once */
    const unsigned char first_char = (unsigned char)property_name[0];

    for (int i = 0; inheritable_properties[i].property_name; i++) {
        /* fast reject: first character mismatch */
        if (first_char != inheritable_properties[i].first_char) continue;

        /* medium reject: length mismatch, cheaper than strcmp */
        if (inheritable_properties[i].length != len) continue;

        /* apply strcmp function for exact string match (only few cases) */
        if (strcmp(property_name, inheritable_properties[i].property_name) == 0)
            return 1;
    }

    return 0;
}