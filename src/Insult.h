#ifndef HIGHPRIEST_INSULT_H
#define HIGHPRIEST_INSULT_H


class CInsultFilter;
struct Insult;

struct Insult {
public:
	Insult(char* word);
	Insult();
	void Set(char* word);
	unsigned char Check(const char*);
	const char* word;
};

class CInsultFilter {
public:
	CInsultFilter();
	~CInsultFilter();
	bool Load(const char* str);
	bool IsGoodSentence(const char* str);
	bool IsBadSentence(const char* str);
private:
	void Insert(char* str);
private:
	std::vector<Insult> m_insult;
};
extern CInsultFilter g_insultFilter;

#endif
