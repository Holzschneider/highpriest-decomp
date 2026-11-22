#include "common.h"
#include "Picker.h"

CActorPickNode g_actorPickNode;


bool	CActorPickInfo :: HitTest(vector2d p)
{
	if (p.x > m_vectors[0].x && p.x < m_vectors[1].x && p.y > m_vectors[0].y && p.y < m_vectors[1].y) {
		return	true;
	} else {
		return	false;
	}
}

CActorPickNode :: CActorPickNode()
{
	for (int i=0; i<4; i++)
		m_child[i] = NULL;
}

void CActorPickNode :: MakeQuardTree(int level, RECT region)
{
	DeleteSubNode();
	m_region = region;
	if (level) {
		for (int i=0; i<4; i++) {
			m_child[i] = new CActorPickNode;
			m_child[i]->MakeQuardTree(level-1, MakeQuarter(m_region,i));
		}
	}
}

void	CActorPickNode :: AddPickInfo(CActorPickInfo *pickInfo)
{
	if (m_child[0]==NULL) {
		m_pickInfoList.push_back(pickInfo);
	} else {
		int quarterNum = GetQuarterNum(pickInfo->m_vectors[0].x, pickInfo->m_vectors[0].y);
		m_child[quarterNum]->AddPickInfo(pickInfo);
	}
}

void	CActorPickNode :: DeleteSubNode()
{
	std::list<CActorPickInfo *>::iterator it = m_pickInfoList.begin();
	while (it != m_pickInfoList.end()) {
		delete (*it);
		it++;
	}
	m_pickInfoList.erase(m_pickInfoList.begin(), m_pickInfoList.end());

	for (int i=0; i<4; i++) {
		if (m_child[i]) {
			delete m_child[i];
			m_child[i] = NULL;
		}
	}
}

CActorPickInfo * CActorPickNode :: FindPickInfo(vector2d pos)
{
	if (m_child[0]==NULL) {
		CActorPickInfo *nearest = NULL;
		float	nearestz = 0;
		std::list<CActorPickInfo *>::iterator it = m_pickInfoList.begin();
		while (it!=m_pickInfoList.end()) {
			CActorPickInfo * pi = *it;
			if (pi->HitTest(pos)) {
				float tmp = pi->m_vectors[0].z + pi->m_vectors[1].z;
				if (tmp > nearestz) {
					nearestz = tmp;
					nearest = pi;
				}
			}
			it++;
		}
		return	nearest;
	} else {
		int quarterNum = GetQuarterNum(pos.x, pos.y);
		CActorPickInfo * pickInfo = m_child[quarterNum]->FindPickInfo(pos);
		if (pickInfo)
			return pickInfo;
		else {
			for (int i=0; i<4; i++) {
				if (quarterNum != i) {
					CActorPickInfo * pickInfo = m_child[i]->FindPickInfo(pos);
					if (pickInfo)
						return pickInfo;
				}
			}
			return NULL;
		}
	}
}

RECT CActorPickNode :: MakeQuarter(RECT region, int quarterNum)
{
	switch (quarterNum) {
		case 0:
			region.right /= 2;
			region.bottom /= 2;
			break;
		case 1:
			region.left = region.right/2+1;
			region.bottom /= 2;
			break;
		case 2:
			region.left = region.right/2+1;
			region.top = region.bottom/2+1;
			break;
		case 3:
			region.right /= 2;
			region.top = region.bottom/2+1;
			break;
	}
	return region;
}

int		CActorPickNode :: GetQuarterNum(int x, int y)
{
	int halfX = m_region.right/2;
	int halfY = m_region.bottom/2;
	if (x <= halfX && y <= halfY) {
		return 0;
	} else if (x > halfX && y <= halfY) {
		return 1;
	} else if (x > halfX && y > halfY) {
		return 2;
	} else {
		return 3;
	}
}

CActorPickNode :: ~CActorPickNode()
{
	DeleteSubNode();
}
