#include "html2tex.h"

int is_block_element(const char* tag_name) {
    if (!tag_name || tag_name[0] == '\0') return 0;

    static const struct {
        const char* tag;
        unsigned char first_char;
        const unsigned char length;
    } block_tags[] = {
        {"div", 'd', 3}, {"p", 'p', 1},
        {"h1", 'h', 2}, {"h2", 'h', 2}, {"h3", 'h', 2},
        {"h4", 'h', 2}, {"h5", 'h', 2}, {"h6", 'h', 2},
        {"ul", 'u', 2}, {"ol", 'o', 2}, {"li", 'l', 2},
        {"table", 't', 5}, {"tr", 't', 2}, {"td", 't', 2}, 
        {"th", 't', 2}, {"blockquote", 'b', 10}, {"section", 's', 7}, 
        {"article", 'a', 7}, {"header", 'h', 6}, {"footer", 'f', 6},
        {"nav", 'n', 3}, {"aside", 'a', 5}, {"main", 'm', 4}, 
        {"figure", 'f', 6}, {"figcaption", 'f', 10}, {NULL, 0, 0}
    };

    /* length-based tags detection */
    size_t len = 0;

    const char* p = tag_name;
    while (*p) {
        len++; p++;

        /* tag name is longer then expected, reject it */
        if (len > 10) return 0;
    }

    /* length-based fast rejection */
    switch (len) {
    case 1:  case 2:  case 3:  case 4:
    case 5: case 6:  case 7:  case 10:
        break;
    default:
        /* length does not match any block tag */
        return 0;
    }

    /* extract first char once */
    const unsigned char first_char = (unsigned char)tag_name[0];

    for (int i = 0; block_tags[i].tag; i++) {
        /* fast reject: first character mismatch */
        if (first_char != (unsigned char)block_tags[i].first_char) continue;

        /* medium reject: length mismatch, cheaper than strcmp */
        if (block_tags[i].length != len) continue;

        /* final verification: exact string match */
        if (strcmp(tag_name, block_tags[i].tag) == 0)
            return 1;
    }

    return 0;
}

int is_inline_element(const char* tag_name) {
    if (!tag_name || tag_name[0] == '\0') return 0;

    static const char* const inline_tags[] = {
        "a", "abbr", "b", "bdi", "bdo", "cite", "code", "data",
        "dfn", "em", "font", "i", "kbd", "mark", "q", "rp", "rt",
        "ruby", "samp", "small", "span", "strong", "sub", "sup",
        "time", "u", "var", "wbr", "br", "img", "map", "object",
        "button", "input", "label", "meter", "output", "progress",
        "select", "textarea", NULL
    };

    /* length-based tags detection */
    size_t len = 0;
    const char* p = tag_name;

    while (*p) {
        len++; p++;

        /* inline element tag name is invalid */
        if (len > 8) return 0;
    }

    switch (len) {
    case 1:  case 2:  case 3: case 4:
    case 5:  case 6: case 8:
        break;
    case 7:
        return 0;
    default:
        /* length doesn't match any known inline tag */
        return 0;
    }

    /* extract first character once */
    const unsigned char first_char = tag_name[0];

    for (int i = 0; inline_tags[i]; i++) {
        /* fast reject using first character mismatch */
        if (first_char != (unsigned char)inline_tags[i][0]) continue;

        /* length mismatch, reject it */
        size_t tag_len = strlen(inline_tags[i]);
        if (tag_len != len) continue;

        /* finally apply expensive strcmp only for few cases */
        if (strcmp(tag_name, inline_tags[i]) == 0)
            return 1;
    }

    return 0;
}

int should_exclude_tag(const char* tag_name) {
    if (!tag_name || tag_name[0] == '\0') return 0;
    static const struct {
        const char* const tag;
        const size_t len;
        const unsigned char first;
    } excluded_tags[] = {
        {"script", 6, 's'}, {"style", 5, 's'}, {"link", 4, 'l'},
        {"meta", 4, 'm'}, {"head", 4, 'h'}, {"noscript", 8, 'n'},
        {"template", 8, 't'}, {"iframe", 6, 'i'}, {"form", 4, 'f'},
        {"input", 5, 'i'}, {"label", 5, 'l'}, {"canvas", 6, 'c'},
        {"svg", 3, 's'}, {"video", 5, 'v'}, {"source", 6, 's'},
        {"audio", 5, 'a'}, {"object", 6, 'o'}, {"button", 6, 'b'},
        {"map", 3, 'm'}, {"area", 4, 'a'}, {"frame", 5, 'f'},
        {"frameset", 8, 'f'}, {"noframes", 8, 'n'}, {"nav", 3, 'n'},
        {"picture", 7, 'p'}, {"progress", 8, 'p'}, {"select", 6, 's'},
        {"option", 6, 'o'}, {"param", 5, 'p'}, {"search", 6, 's'},
        {"samp", 4, 's'}, {"track", 5, 't'}, {"var", 3, 'v'},
        {"wbr", 3, 'w'}, {"mark", 4, 'm'}, {"meter", 5, 'm'},
        {"optgroup", 8, 'o'}, {"q", 1, 'q'}, {"blockquote", 10, 'b'},
        {"bdo", 3, 'b'}, {NULL, 0, 0}
    };

    /* compute the length with bounds checking */
    size_t len = 0;
    const char* p = tag_name;

    while (*p) {
        if (++len > 10) return 0;
        p++;
    }

    /* optimized length rejection */
    switch (len) {
    case 1: case 3: case 4: case 5:
    case 6: case 7: case 8: case 10:
        break;
    case 2: case 9:
        /* no excluded tags of these lengths */
        return 0;
    default:
        return 0;
    }

    /* extract first char once */
    const unsigned char first_char = (unsigned char)tag_name[0];

    /* perform an optimized search */
    for (int i = 0; excluded_tags[i].tag; i++) {
        /* first char mismatch */
        if (excluded_tags[i].first != first_char) continue;

        /* length mismatch */
        if (excluded_tags[i].len != len) continue;

        /* final verification */
        if (strcmp(tag_name, excluded_tags[i].tag) == 0)
            return 1;
    }

    return 0;
}

int is_whitespace_only(const char* text) {
    if (!text) return 1;
    const unsigned char* p = (const unsigned char*)text;

    while (*p) {
        unsigned char c = *p++;
        switch (c) {
        case ' ': case '\t': case '\n': case '\v': case '\f': case '\r':
            continue;
        default:
            return 0;
        }
    }

    return 1;
}