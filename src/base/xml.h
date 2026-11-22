#ifndef HIGHPRIEST_BASE_XML_H
#define HIGHPRIEST_BASE_XML_H


class XMLDocument;
class XMLElement;

class XMLElement {
public:
	XMLElement(const char* name);
	~XMLElement();
	void Clear();
	const char* GetName() { return m_name; }
	std::string& GetContents();
	void AddChild(XMLElement* element);
	XMLElement* GetChild() { return m_child; }
	XMLElement* FindChild(const char* name);
	void AddNext(XMLElement* element);
	XMLElement* GetNext() { return m_next; }
	XMLElement* FindNext(const char* name);
	void SetStart(const char* str) { m_start = str; }
	void SetEnd(const char* str) { m_end = str; }
	std::string ParseSpecialCharater(const char* document, const char* end);
	const char* SkipTag(const char* document, const char*) {
		while (*document != '>' && *document != 0) ++document;
		return ++document;
	}
protected:
	const char* m_name;
	const char* m_start;
	const char* m_end;
	std::string m_contents;
	XMLElement* m_next;
	XMLElement* m_child;
};

class XMLDocument {
public:
	XMLDocument() : m_root("XMLDocument"), m_buf(NULL) {}
	~XMLDocument() { Clear(); }
	void Clear();
	bool ReadDocument(const char* document, const char* end);
	XMLElement* GetRootElement() { return &m_root; }
	XMLElement* FindElement(const char* name) { return m_root.FindChild(name); }
protected:
	const char* ReadContents(XMLElement* element, const char* document, const char* end);
	const char* ReadElement(XMLElement* element, const char* document, const char* end);
	bool IsEndTag(const char* document)     { return *(document + 1) == '/'; }
	bool IsPITag(const char* document)      { return *(document + 1) == '?'; }
	bool IsCommentTag(const char* document) { return *(document + 1) == '!'; }
	bool IsClosedTag(const char* document, const char*) { return *(document - 2) == '/'; }
	const char* SkipTag(const char* document, const char* end) {
		while (document != end - 1 && *document != '>') ++document;
		return ++document;
	}
	const char* ParseTag(const char* document, const char* end);
protected:
	XMLElement m_root;
	char* m_buf;
};

inline bool IsTag(const char* document) { return *document == '<'; }
inline bool IsSpecialCharater(const char* document) { return *document == '&'; }
inline const char* SkipSpecialCharacter(const char* document, const char* end) {
	while (document != end && *document != ';') ++document;
	return ++document;
}

#endif
