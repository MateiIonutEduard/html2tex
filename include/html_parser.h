#include "html2tex.h"
#include <string>
#include <iostream>
using namespace std;

class HtmlParser {
private:
	HTMLNode* node;

	HTMLNode* dom_tree_copy(const HTMLNode*);
	HTMLNode* dom_tree_copy(const HTMLNode*, HTMLNode*);

public:
	HtmlParser(const string&, bool);
	HtmlParser(const HtmlParser&);

	HtmlParser& operator =(const HtmlParser&);
	friend ostream& operator<<(ostream&, HtmlParser&);


	string toString();
	~HtmlParser();
};