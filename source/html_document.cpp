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

HtmlDocument::HtmlDocument(HtmlDocument&& other) noexcept
    : node(other.node), props(other.props), hasProps(other.hasProps) {
    other.node = nullptr;
    other.props = nullptr;
    other.hasProps = false;
}

HtmlDocument& HtmlDocument::operator=(const HtmlDocument& other) {
    if (this != &other) {
        /* clean up existing CSS */
        if (hasProps && props)
            css_properties_destroy(props);

        node = other.node;

        if (other.props) {
            try {
                props = css_properties_copy(other.props);
                hasProps = true;
            }
            catch (const HtmlRuntimeException& exception) {
                std::cerr << exception.toString() << std::endl;
            }
        }
        else {
            props = nullptr;
            hasProps = false;
        }
    }

    return *this;
}

HtmlDocument& HtmlDocument::operator=(HtmlDocument&& other) noexcept {
    if (this != &other) {
        /* clean up existing CSS (take ownership) */
        if (hasProps && props)
            css_properties_destroy(props);

        node = other.node;
        props = other.props;
        hasProps = other.hasProps;

        other.node = nullptr;
        other.props = nullptr;
        other.hasProps = false;
    }

    return *this;
}

bool HtmlDocument::isValid() const noexcept {
    return node != nullptr;
}

std::string HtmlDocument::tagName() const noexcept {
    if (!node || !node->tag) return "";
    return std::string(node->tag);
}

std::string HtmlDocument::textContent() const noexcept {
    if (!node || !node->content) return "";
    return std::string(node->content);
}

std::string HtmlDocument::getAttribute(const std::string& key) const {
    if (!node) return "";
    const char* value = get_attribute(node->attributes, key.c_str());
    return value ? std::string(value) : "";
}

bool HtmlDocument::hasAttribute(const std::string& key) const {
    if (!node) return false;
    return get_attribute(node->attributes, key.c_str()) != nullptr;
}

CSSProperties* HtmlDocument::cssProperties() const noexcept {
    return props;
}

HtmlDocument HtmlDocument::parent() const noexcept {
    if (!node || !node->parent) return HtmlDocument();
    return HtmlDocument(node->parent);
}

bool HtmlDocument::hasParent() const noexcept {
    return node && node->parent;
}

HtmlDocument::~HtmlDocument() {
    if (hasProps && props)
        css_properties_destroy(props);
}