#ifndef HTMLTEX_H
#define HTMLTEX_H

#include <memory>
#include <string>
#include "html2tex.h"
#include <iostream>
using namespace std;

class HtmlTeXConverter {
private:
    LaTeXConverter* converter;

public:
    HtmlTeXConverter();
    ~HtmlTeXConverter();

    /* Convert the HTML code to corresponding LaTeX code. */
    string convert(const string& html);

    /* check for errors */
    bool hasError() const;
    int getErrorCode() const;
    string getErrorMessage() const;

    /* delete copy constructor and assignment operator to prevent copying */
    HtmlTeXConverter(const HtmlTeXConverter&) = delete;
    HtmlTeXConverter& operator=(const HtmlTeXConverter&) = delete;
};

#endif