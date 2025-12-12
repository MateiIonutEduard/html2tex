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

    static const struct {
        const char* tag;
        unsigned char first_char;
        const unsigned char length;
    } inline_tags[] = {
        {"a", 'a', 1}, {"abbr", 'a', 4}, {"b", 'b', 1},
        {"bdi", 'b', 3}, {"bdo", 'b', 3}, {"cite", 'c', 4},
        {"code", 'c', 4}, {"data", 'd', 4}, {"dfn", 'd', 3},
        {"em", 'e', 2}, {"font", 'f', 4}, {"i", 'i', 1},
        {"kbd", 'k', 3}, {"mark", 'm', 4}, {"q", 'q', 1},
        {"rp", 'r', 2}, {"rt", 'r', 2}, {"ruby", 'r', 4},
        {"samp", 's', 4}, {"small", 's', 5}, {"span", 's', 4},
        {"strong", 's', 6}, {"sub", 's', 3}, {"sup", 's', 3},
        {"time", 't', 4}, {"u", 'u', 1}, {"var", 'v', 3},
        {"wbr", 'w', 3}, {"br", 'b', 2}, {"img", 'i', 3},
        {"map", 'm', 3}, {"object", 'o', 6}, {"button", 'b', 6},
        {"input", 'i', 5}, {"label", 'l', 5}, {"meter", 'm', 5},
        {"output", 'o', 6}, {"progress", 'p', 8}, {"select", 's', 6},
        {"textarea", 't', 8}, {NULL, 0, 0}
    };

    /* compute the length with early bounds check */
    size_t len = 0;
    const char* p = tag_name;

    while (*p) {
        len++;
        p++;

        /* early exit for unreasonably long tags */
        if (len > 8) return 0;
    }

    /* length-based fast rejection */
    switch (len) {
    case 1: case 2: case 3: case 4:
    case 5: case 6: case 8:
        break;
    case 7:
        /* explicitly reject length 7 */
        return 0;
    default:
        /* length doesn't match any known inline tag */
        return 0;
    }

    /* extract first character once */
    const unsigned char first_char = (unsigned char)tag_name[0];

    /* optimized linear search with metadata filtering */
    for (int i = 0; inline_tags[i].tag; i++) {
        /* fast reject for first character mismatch */
        if (first_char != inline_tags[i].first_char) continue;

        /* reject by length mismatch */
        if (len != inline_tags[i].length) continue;

        /* final verification by exact string match */
        if (strcmp(tag_name, inline_tags[i].tag) == 0)
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