#ifndef HIGHPRIEST_3DLIB_MESH_H
#define HIGHPRIEST_3DLIB_MESH_H

#include "Prim.h"
#include "Renderer.h"
const	int MAX_SHADEGROUP = 32;

class C3dMesh;

class C3dMesh {
public:
	C3dMesh();
	~C3dMesh();
	void SetParent(C3dMesh*);
	void UpdateNormal();
	void Release();
	void SortFace();
private:
	friend class C3dModelRes;
	friend class C3dNode;
	int m_numVert;
	int m_numFace;
	int m_numTVert;
	vector3d* m_vert;
	vector3d* m_faceNormal;
	vector3d* m_vertNormal;
	tvertex3d* m_tvert;
	face3d* m_face;
	C3dMesh* m_parent;
	std::vector<int> m_shadeGroup[32];
	CDynamicVB m_vb;
};

#endif
