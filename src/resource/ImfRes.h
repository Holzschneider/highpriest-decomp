#ifndef HIGHPRIEST_RESOURCE_IMFRES_H
#define HIGHPRIEST_RESOURCE_IMFRES_H

#include "../base/Res.h"

class CImfRes;
struct ImfData;

struct ImfData {
public:
	std::vector<int> m_Priority;
	std::vector<int> m_cx;
	std::vector<int> m_cy;
};

class CImfRes : public CRes {
public:
	CImfRes();
	DECLARE_RESOURCE(CImfRes)
	virtual bool Load(const char* fName);
	tagPOINT GetPoint(int __$ReturnUdt, int Layer, int Act);
	int GetPriority(int Priority, int Act, int Mot);
	int GetLayer(int Priority, int Act, int Mot);
	int GetLongestMotion(int MaxLayer);
	int GetNumMotion(int NumLayer, int NumAction);
	int GetNumAction(int MaxLayer);
	int GetNumLayer();
	void Create();
	int CheckSum(int MaxLayer);
protected:
	std::vector<std::vector<ImfData> > m_ImfData;
	int m_maxAction[15];
	int m_maxMotion[15][104];
};

#endif
