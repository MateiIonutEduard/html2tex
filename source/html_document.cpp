#include "html_document.hpp"
#include <memory>
#include <string>
#include <iostream>

HtmlDocument::HtmlDocument() noexcept : htmlParser(nullptr) 
{ }

HtmlDocument::HtmlDocument(const HtmlDocument& other) : htmlParser(nullptr) {
	if (other.htmlParser != nullptr) {
		try {
			htmlParser = std::make_unique<HtmlParser>(
				*other.htmlParser);
		}
		catch (const HtmlRuntimeException& exception) {
			std::cerr << exception.toString() << std::endl;
		}
	}
}

HtmlDocument::HtmlDocument(const HtmlParser& parser) : htmlParser(nullptr) {
	/* copy the parser content if is not empty */
	if (parser.hasContent()) {
		try {
			htmlParser = std::make_unique<HtmlParser>(parser);
		}
		catch (const HtmlRuntimeException& exception) {
			std::cerr << exception.toString() << std::endl;
		}
	}
}