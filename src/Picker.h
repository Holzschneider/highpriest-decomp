#ifndef HIGHPRIEST_PICKER_H
#define HIGHPRIEST_PICKER_H

#include "3dlib/Prim.h"

class CActorPickInfo;
class CActorPickNode;


enum CLASSTYPE { CT_CGAMEACTOR, CT_CITEM, CT_CSKILL, CT_PET, CT_MER};

class CActorPickInfo {
public:
	vector3d m_vectors[2];
	unsigned int m_gid;
	int m_job;
	int m_classType;
	int m_isPkState;

	CActorPickInfo() {}
	~CActorPickInfo() {}

	unsigned int GetGID() { return m_gid; }
	void SetJob(int j) { m_job = j; }
	int GetJob() { return m_job; }
	bool HitTest(vector2d p);
	vector2d GetCenterPos()
	{
		return vector2d(
			m_vectors[1].x+(m_vectors[0].x-m_vectors[1].x)/2
			, m_vectors[1].y+(m_vectors[0].y-m_vectors[1].y)/2);
	}
};

class CActorPickNode {
public:
	CActorPickNode();
	virtual ~CActorPickNode();
	void MakeQuardTree(int level, tagRECT region);
	void AddPickInfo(CActorPickInfo* pickInfo);
	void DeleteSubNode();
	CActorPickInfo* FindPickInfo(vector2d pos);
	tagRECT MakeQuarter(tagRECT __$ReturnUdt, int region);
	int GetQuarterNum(int x, int y);
private:
	tagRECT m_region;
	CActorPickNode* m_child[4];
	std::list<CActorPickInfo *> m_pickInfoList;
};
extern CActorPickNode g_actorPickNode;

#endif
