#include "common.h"
#include "mesh.h"




C3dMesh :: C3dMesh()
{
	m_numVert = 0;
	m_numFace = 0;
	m_numTVert = 0;
	m_vert = NULL;
	m_tvert = NULL;
	m_face = NULL;
	m_faceNormal = NULL;
	m_vertNormal = NULL;
}

C3dMesh :: ~C3dMesh()
{
	Release();
}

void	C3dMesh :: Release()
{
	if (m_face) {
		delete  [] m_face;
		m_face = NULL;
	}
	if (m_vert) {
		delete  [] m_vert;
		m_vert = NULL;
	}
	if (m_tvert) {
		delete  [] m_tvert;
		m_tvert = NULL;
	}
	if (m_faceNormal) {
		delete [] m_faceNormal;
		m_faceNormal = NULL;
	}
	if (m_vertNormal) {
		delete [] m_vertNormal;
		m_vertNormal = NULL;
	}
	m_numVert = 0;
	m_numFace = 0;
	m_numTVert = 0;
}

void	C3dMesh :: UpdateNormal()
{
	if (!m_faceNormal)
		m_faceNormal = new vector3d[m_numFace];
	if (!m_vertNormal)
		m_vertNormal = new vector3d[m_numFace*3];
	vector3d *	tempVertNormal = new vector3d[m_numVert];


	int i;
	for (i=0; i<m_numFace; i++)
	{
		m_faceNormal[i].CrossProduct(m_vert[m_face[i].vertindex[2]]-m_vert[m_face[i].vertindex[1]], m_vert[m_face[i].vertindex[2]]-m_vert[m_face[i].vertindex[0]]);
		m_faceNormal[i].Normalize();
	}


	for (i=0; i<MAX_SHADEGROUP; i++) {
		m_shadeGroup[i].erase(m_shadeGroup[i].begin(), m_shadeGroup[i].end());
	}
	for (i=0; i<m_numFace; i++) {
		sAssert(m_face[i].smoothGroup >= 0 && m_face[i].smoothGroup < MAX_SHADEGROUP);
		m_shadeGroup[m_face[i].smoothGroup].push_back(i);
	}


	for (int g=0; g<MAX_SHADEGROUP; g++)
	{
		if (m_shadeGroup[g].size() == 0)
			continue;
		for (int i=0; i<m_numVert; i++)
		{
			tempVertNormal[i].Set(0, 0, 0);
			for (int j=0; j<m_numFace; j++)
			{
				if (m_face[j].smoothGroup != g)
					continue;
				if (m_face[j].vertindex[0] == i || m_face[j].vertindex[1] == i || m_face[j].vertindex[2] == i)
					tempVertNormal[i] += m_faceNormal[j];
			}
			if (tempVertNormal[i].Magnitude() != 0)
				tempVertNormal[i].Normalize();
		}
		for (std::vector <int> :: iterator li=m_shadeGroup[g].begin(); li!=m_shadeGroup[g].end(); li++)
		{
			int		faceId = *li;
			m_vertNormal[faceId*3+0] = tempVertNormal[m_face[faceId].vertindex[0]];
			m_vertNormal[faceId*3+1] = tempVertNormal[m_face[faceId].vertindex[1]];
			m_vertNormal[faceId*3+2] = tempVertNormal[m_face[faceId].vertindex[2]];
		}
	}
	delete [] tempVertNormal;
}
