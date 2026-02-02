#include "html_document.hpp"
#include "dom_tree_visitor.h"
#include "htmltex_exception.hpp"
#include <iostream>
#include <queue>
#include <stack>

HtmlDocument::HtmlDocument() noexcept
    : node(nullptr), props(nullptr), hasProps(false) {
}

HtmlDocument::HtmlDocument(HTMLNode* node) noexcept
    : node(node), props(nullptr), hasProps(false) {
}

HtmlDocument::HtmlDocument(HTMLNode* node, CSSProperties* css) noexcept
    : node(node), props(css), hasProps(false) {
}

HtmlDocument::HtmlDocument(HTMLElement* elem) noexcept
    : node(nullptr), props(nullptr), hasProps(false) {
    if (elem) {
        node = elem->node;
        props = elem->css_props;

        /* destroy container only */
        if (props != nullptr) {
            std::free(elem);
            hasProps = true;
        }
    }
}

HtmlDocument::HtmlDocument(const HtmlDocument& other)
    : node(other.node), props(nullptr), hasProps(false) {
    if (other.props) {
        try {
            props = css_properties_copy(other.props);
            hasProps = true;
        }
        catch (const HtmlRuntimeException& exception) {
            std::cerr << exception.toString() << std::endl;
        }
    }
}