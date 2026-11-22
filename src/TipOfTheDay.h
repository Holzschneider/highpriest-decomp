#ifndef HIGHPRIEST_TIPOFTHEDAY_H
#define HIGHPRIEST_TIPOFTHEDAY_H


typedef std::vector<std::string> StringList;

class CTipOfTheDay;

class CTipOfTheDay {
public:
	CTipOfTheDay() : m_bTryLoading(0) {}
	~CTipOfTheDay() {}
	int Load();
	int Size()
	{
		if (!m_bTryLoading)
			Load();
		return m_tipList.size();
	}
	StringList& GetTip(int index);
protected:
	unsigned char m_bTryLoading;
	std::vector<StringList> m_tipList;
};
extern CTipOfTheDay g_tipOfTheDay;

#endif
