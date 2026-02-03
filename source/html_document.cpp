#include "html_document.hpp"
#include "dom_tree_visitor.h"
#include "html_exception.hpp"
#include <cstring>
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

HtmlDocument HtmlDocument::nextSibling() const noexcept {
    if (!node || !node->next) return HtmlDocument();
    return HtmlDocument(node->next);
}

HtmlDocument HtmlDocument::previousSibling() const noexcept {
    if (!node) return HtmlDocument();

    /* find previous sibling by traversing parent's children */
    if (!node->parent) return HtmlDocument();

    HTMLNode* child = node->parent->children;
    HTMLNode* prev = nullptr;

    while (child && child != node) {
        prev = child;
        child = child->next;
    }

    return prev ? HtmlDocument(prev) 
        : HtmlDocument();
}

bool HtmlDocument::hasNextSibling() const noexcept {
    return node && node->next;
}

bool HtmlDocument::hasPreviousSibling() const noexcept {
    if (!node || !node->parent) return false;

    HTMLNode* child = node->parent->children;
    return child && child != node;
}

HtmlDocument HtmlDocument::firstChild() const noexcept {
    if (!node || !node->children) return HtmlDocument();
    return HtmlDocument(node->children);
}

HtmlDocument HtmlDocument::lastChild() const noexcept {
    if (!node || !node->children) 
        return HtmlDocument();

    HTMLNode* child = node->children;
    while (child->next)
        child = child->next;

    return HtmlDocument(child);
}

bool HtmlDocument::hasChildren() const noexcept {
    return node && node->children;
}

std::vector<HtmlDocument> HtmlDocument::children() const {
    std::vector<HtmlDocument> result;
    if (!node) return result;
    HTMLNode* child = node->children;

    while (child) {
        result.emplace_back(child);
        child = child->next;
    }

    return result;
}

size_t HtmlDocument::childCount() const noexcept {
    if (!node) return 0;
    size_t count = 0;
    HTMLNode* child = node->children;

    while (child) {
        ++count;
        child = child->next;
    }

    return count;
}

bool HtmlDocument::isBlockElement() const {
    if (!node || !node->tag) return false;
    return is_block_element(node->tag) != 0;
}

bool HtmlDocument::isInlineElement() const {
    if (!node || !node->tag) return false;
    return is_inline_element(node->tag) != 0;
}

bool HtmlDocument::isVoidElement() const {
    if (!node || !node->tag) return false;
    return is_void_element(node->tag) != 0;
}

bool HtmlDocument::isWhitespaceOnly() const {
    if (!node) return true;
    return is_whitespace_only(node->content) != 0;
}

bool HtmlDocument::shouldExclude() const {
    if (!node || !node->tag) return false;
    return should_exclude_tag(node->tag) != 0;
}

int HtmlDocument::getElementByIdPredicate(const HTMLNode* root, const void* data) {
    if (!root || !root->tag) return 0;
    
    if (root->attributes) {
        const char* value = get_attribute(root->attributes, "id");
        return (value && std::strcmp(value, (const char*)data) == 0);
    }

    return 0;
}

int HtmlDocument::getElementByClassPredicate(const HTMLNode* root, const void* data) {
    if (!root || !root->tag) return 0;

    if (root->attributes) {
        const char* className = get_attribute(root->attributes, "class");
        return (className && std::strcmp(className, (const char*)data) == 0);
    }

    return 0;
}

HtmlDocument HtmlDocument::getFirstElementById(const std::string& id) const {
    return findFirst(
        &getElementByIdPredicate,
        id);
}

HtmlDocument HtmlDocument::findFirst(DOMTreeVisitor predicate, const std::string& content) const {
    if (!node) return HtmlDocument();

    HTMLElement* found = html2tex_search_tree(
        node, predicate,
        (const void*)content.c_str(),
        props
    );

    if (found)
        return HtmlDocument(found);
    return HtmlDocument();
}

bool HtmlDocument::hasElementWithId(const std::string& id) const {
    return hasAnyElement(
        &getElementByIdPredicate,
        id);
}

bool HtmlDocument::hasElementWithClass(const std::string& className) const {
    return hasAnyElement(
        &getElementByClassPredicate,
        className);
}

bool HtmlDocument::hasAnyElement(DOMTreeVisitor predicate, const std::string content) const {
    return findFirst(
        predicate,
        content
    ).isValid();
}

std::vector<HtmlDocument> HtmlDocument::findAllElementsById(const std::string& id) const {
    return findAll(
        &getElementByIdPredicate,
        id);
}

std::vector<HtmlDocument> HtmlDocument::findAll(DOMTreeVisitor predicate, const std::string& content) const {
    std::vector<HtmlDocument> result;
    if (!node) return result;

    HTMLNodeList* list = html2tex_find_all(
        node, predicate,
        (const void*)content.c_str(),
        props
    );

    if (list) {
        size_t count = list->node_count;
        result.reserve(count);

        /* get array of matching elements */
        HTMLElement** elements = html_nodelist_dismantle(&list);

        if (elements) {
            size_t count = list->node_count;
            result.reserve(count);

            for (size_t i = 0; i < count; ++i)
                result.emplace_back(elements[i]);

            free(elements);
        }

        /* destroy the list, but keep the ownership */
        if (list) html_nodelist_destroy(&list);
    }

    return result;
}

HtmlDocument HtmlDocument::getFirstElementByClassName(const std::string& className) const {
    return findFirst(
        &getElementByClassPredicate,
        className);
}

std::vector<HtmlDocument> HtmlDocument::findAllElementsByClassName(const std::string& className) const {
    return findAll(
        &getElementByClassPredicate,
        className);
}

HtmlDocument::~HtmlDocument() {
    if (hasProps && props)
        css_properties_destroy(props);
}

HtmlDocument::Iterator HtmlDocument::begin() {
    return Iterator(node ? node->children : nullptr, props);
}

HtmlDocument::ConstIterator HtmlDocument::begin() const {
    return ConstIterator(node ? node->children : nullptr, props);
}

HtmlDocument::ConstIterator HtmlDocument::cbegin() const {
    return ConstIterator(node ? node->children : nullptr, props);
}

HtmlDocument::Iterator HtmlDocument::end() {
    return Iterator();
}

HtmlDocument::ConstIterator HtmlDocument::end() const {
    return ConstIterator();
}

HtmlDocument::ConstIterator HtmlDocument::cend() const {
    return ConstIterator();
}

HTMLNode* HtmlDocument::rawNode() const noexcept {
    return node;
}

HtmlDocument HtmlDocument::fromRawElement(HTMLElement* elem) {
    return HtmlDocument(elem);
}

// Iterator class implementation

HtmlDocument::Iterator::Iterator() noexcept
    : current(nullptr), props(nullptr) { }

HtmlDocument::Iterator::Iterator(HTMLNode* node, CSSProperties* css) noexcept
    : current(node), props(css), element(node, css) { }

HtmlDocument::Iterator::reference HtmlDocument::Iterator::operator*() noexcept {
    return element;
}

HtmlDocument::Iterator::pointer HtmlDocument::Iterator::operator->() noexcept {
    return &element;
}

HtmlDocument::Iterator& HtmlDocument::Iterator::operator++() noexcept {
    if (current) {
        current = current->next;
        element = HtmlDocument(current, props);
    }

    return *this;
}

HtmlDocument::Iterator HtmlDocument::Iterator::operator++(int) noexcept {
    Iterator temp = *this;
    ++(*this);
    return temp;
}

bool HtmlDocument::Iterator::operator==(const Iterator& other) const noexcept {
    return current == other.current;
}

bool HtmlDocument::Iterator::operator!=(const Iterator& other) const noexcept {
    return current != other.current;
}

// ConstIterator class implementation

HtmlDocument::ConstIterator::ConstIterator() noexcept
    : current(nullptr), props(nullptr) {
}

HtmlDocument::ConstIterator::ConstIterator(const HTMLNode* node, CSSProperties* css) noexcept
    : current(node), props(css), element(const_cast<HTMLNode*>(node), css) {
}

HtmlDocument::ConstIterator::reference HtmlDocument::ConstIterator::operator*() const noexcept {
    return element;
}

HtmlDocument::ConstIterator::pointer HtmlDocument::ConstIterator::operator->() const noexcept {
    return &element;
}

HtmlDocument::ConstIterator& HtmlDocument::ConstIterator::operator++() noexcept {
    if (current) {
        current = current->next;
        element = HtmlDocument(const_cast<HTMLNode*>(current), props);
    }

    return *this;
}

HtmlDocument::ConstIterator HtmlDocument::ConstIterator::operator++(int) noexcept {
    ConstIterator temp = *this;
    ++(*this);
    return temp;
}

bool HtmlDocument::ConstIterator::operator==(const ConstIterator& other) const noexcept {
    return current == other.current;
}

bool HtmlDocument::ConstIterator::operator!=(const ConstIterator& other) const noexcept {
    return current != other.current;
}