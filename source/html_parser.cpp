#include "html2tex.h"
#include "htmltex.h"
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
using namespace std;

HtmlParser::HtmlParser() {
    node = NULL;
    minify = 0;
}

HtmlParser::HtmlParser(const string& html) : HtmlParser(html, 0)
{ }

HtmlParser::HtmlParser(const string& html, int minify) {
	node = minify ? html2tex_parse_minified(html.c_str()) 
        : html2tex_parse(html.c_str());
    this->minify = minify;
}

HtmlParser::HtmlParser(HTMLNode* node) {
    node = dom_tree_copy(node);
}

HtmlParser::HtmlParser(const HtmlParser& parser) {
    node = dom_tree_copy(parser.node);
}

HtmlParser& HtmlParser::operator=(const HtmlParser& parser)
{
    HtmlParser newParser = HtmlParser(parser);
    return newParser;
}

ostream& operator <<(ostream& out, HtmlParser& parser) {
    string output = parser.toString();
    out << output << endl;
    return out;
}

void HtmlParser::setParent(HTMLNode* node) {
    if (this->node) html2tex_free_node(this->node);
    this->node = dom_tree_copy(node);
}

istream& operator >>(istream& in, HtmlParser& parser) {
    
    ostringstream stream;
    stream << in.rdbuf();

    string ptr = stream.str();
    HTMLNode* node = parser.minify ? html2tex_parse_minified(ptr.c_str())
        : html2tex_parse(ptr.c_str());

    if (node) {
        parser.setParent(node);
        html2tex_free_node(node);
    }

    return in;
}

string HtmlParser::toString() {
    char* output = get_pretty_html(node);

    if (output) {
        string result(output);
        free(output);
        return result;
    }

    return "";
}

HTMLNode* HtmlParser::dom_tree_copy(const HTMLNode* node) {
    return dom_tree_copy(node, NULL);
}

HTMLNode* HtmlParser::dom_tree_copy(const HTMLNode* node, HTMLNode* parent) {
    if (!node) return NULL;

    /* allocate new node */
    HTMLNode* new_node = (HTMLNode*)malloc(sizeof(HTMLNode));
    if (!new_node) return NULL;

    /* initialize all fields */
    new_node->tag = NULL;
    new_node->content = NULL;

    new_node->attributes = NULL;
    new_node->children = NULL;

    new_node->next = NULL;
    new_node->parent = parent;

    /* deep copy tag string */
    if (node->tag) {
        size_t tag_len = strlen(node->tag);
        new_node->tag = (char*)malloc(tag_len + 1);

        if (!new_node->tag) {
            free(new_node);
            return NULL;
        }

        strcpy(new_node->tag, node->tag);
    }

    /* deep copy content string */
    if (node->content) {
        size_t content_len = strlen(node->content);
        new_node->content = (char*)malloc(content_len + 1);

        if (!new_node->content) {
            if (new_node->tag) free(new_node->tag);
            free(new_node);
            return NULL;
        }

        strcpy(new_node->content, node->content);
    }

    /* deep copy attributes linked list */
    HTMLAttribute* orig_attr = node->attributes;
    HTMLAttribute** current_attr = &new_node->attributes;

    while (orig_attr) {
        HTMLAttribute* new_attr = (HTMLAttribute*)malloc(sizeof(HTMLAttribute));

        if (!new_attr) {
            html2tex_free_node(new_node);
            return NULL;
        }

        new_attr->key = NULL;
        new_attr->value = NULL;
        new_attr->next = NULL;

        /* deep copy key string */
        if (orig_attr->key) {
            size_t key_len = strlen(orig_attr->key);
            new_attr->key = (char*)malloc(key_len + 1);

            if (!new_attr->key) {
                free(new_attr);
                html2tex_free_node(new_node);
                return NULL;
            }

            strcpy(new_attr->key, orig_attr->key);
        }

        /* deep copy value string if exists */
        if (orig_attr->value) {
            size_t value_len = strlen(orig_attr->value);
            new_attr->value = (char*)malloc(value_len + 1);

            if (!new_attr->value) {
                if (new_attr->key) free(new_attr->key);
                free(new_attr);

                html2tex_free_node(new_node);
                return NULL;
            }

            strcpy(new_attr->value, orig_attr->value);
        }

        /* link into attribute list */
        *current_attr = new_attr;
        current_attr = &new_attr->next;
        orig_attr = orig_attr->next;
    }

    /* deep copy children linked list recursively */
    HTMLNode* orig_child = node->children;
    HTMLNode** current_child = &new_node->children;

    while (orig_child) {
        HTMLNode* new_child = dom_tree_copy(orig_child, new_node);
        if (!new_child) {
            html2tex_free_node(new_node);
            return NULL;
        }

        /* link into children list */
        *current_child = new_child;

        current_child = &new_child->next;
        orig_child = orig_child->next;
    }

    return new_node;
}

HtmlParser::~HtmlParser() {
	if(node)
		html2tex_free_node(node);
}