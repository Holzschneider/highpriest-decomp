#include "common.h"
#include "../Session.h"
#include "../GameActor.h"
#include "Model.h"
#include "../View.h"
#include "../GameWorld.h"
#include "../GameMode.h"
#include "../RagEffect.h"
#include "../3dlib/Prim.h"
#include "Attr.h"
#include "Wave.h"
#include "../device/Sound.h"
#include <algorithm>
#include "../3dlib/Anim.h"
#include "../3dlib/Mesh.h"
#include "../3dlib/Texture.h"
#include "../base/File.h"
#include "Skybox.h"
#include "Ground.h"






extern DWORD const g_GlobalVertexFormat = (D3DFVF_XYZ|D3DFVF_NORMAL|
									D3DFVF_TEX1|D3DFVF_TEXCOORDSIZE2(0));

using std::string;

inline void C3dMesh::SortFace()
{
	std::sort(m_face, m_face + m_numFace);
}





typedef std::list<C3dNodeRes *>     C3dNodeResList;
typedef std::list<CVolumeBox *>     CVolumeBoxList;
typedef std::list<C3dAnimNodeRes *> C3dAnimNodeResList;
typedef std::list<int>              IntList;


#ifndef Trace
#define Trace (void)
#endif






#ifndef MAX_SHADEGROUP
#endif
#ifndef MAX_GROUP
#define MAX_GROUP MAX_SHADEGROUP
#endif
#ifndef MAX_FACE_PER_MESH
#define MAX_FACE_PER_MESH 1000
#endif
#ifndef MAX_VERTEX_PER_MESH
#define MAX_VERTEX_PER_MESH 1000
#endif
#ifndef MAX_BONE_TYPE
#define MAX_BONE_TYPE 100
#endif
#ifndef MAX_GRANNY_CONTROL
#define MAX_GRANNY_CONTROL 20
#endif
#ifndef MAX_GRANNY_VERTEX
#define MAX_GRANNY_VERTEX 10000
#endif


extern CRenderer* g_renderer;

C3dGrannyBoneRes g_GrannyBoneRes;

extern CSession g_session;





#include <windows.h>
#include <ddraw.h>


#ifndef sprintf_s
#include <stdio.h>
#include <stdarg.h>
#define sprintf_s rag_sprintf_s
inline int rag_sprintf_s(char* buf, size_t, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int n = vsprintf(buf, fmt, ap);
	va_end(ap);
	return n;
}
#endif



typedef _W3D_TLVERTEX w3dtlvertex;

#include <math.h>


C3dNodeRes :: C3dNodeRes()
{
	memset(parentname, 0, sizeof(parentname));
	parent = NULL;
	texture = NULL;
	mesh = NULL;
	numTexture = 0;
}

C3dNodeRes :: ~C3dNodeRes()
{
	Release();
}

void	C3dNodeRes :: Release()
{
	if (texture) {
		for (int i = 0; i < numTexture; i++) {
			texture[i]->Unlock();
			g_texMgr.DestroyTexture(texture[i]);
		}
		delete [] texture;
		texture = NULL;
	}
	if (mesh) {
		delete mesh;
		mesh = NULL;
	}
	child.erase(child.begin(), child.end());
}

static C3dModelRes C3dModelResInstance(Exemplar(), "rsm", "model\\");


C3dModelRes :: C3dModelRes()
	: CRes()
{
	m_numMaterials = 0;
	m_material = NULL;
}

void	C3dModelRes :: Reset()
{
	if (m_material) {
		delete [] m_material;
		m_material = NULL;
		m_numMaterials = 0;
	}
	C3dNodeResList::iterator i = m_objectList.begin();
	while (i != m_objectList.end()) {
		delete *i;
		i++;
	}
	m_objectList.erase(m_objectList.begin(), m_objectList.end());
	m_rootObjList.erase(m_rootObjList.begin(), m_rootObjList.end());
	m_meshList.erase(m_meshList.begin(), m_meshList.end());

	CVolumeBoxList::iterator j = m_volumeBoxList.begin();
	while (j != m_volumeBoxList.end()) {
		delete *j;
		j++;
	}
	m_volumeBoxList.erase(m_volumeBoxList.begin(), m_volumeBoxList.end());
}


#include "rag_symbol_shift.h"
bool	C3dModelRes :: Load(const char *fName)
{
	RAG_SYMBOL_SHIFT(1)
	CFile	fp;

	if(fp.Open(fName, CFile::modeRead) == false)
		return false;

	char	magicId[] = "GRSM";
	char	magicBuf[5];

	fp.Read(magicBuf, 4);
	magicBuf[4]=0;
	if (strcmp(magicBuf, magicId))
	{
#if defined(BF_PORT)
		bf_port_missing(BFT_RES, "res.unusable", "%s: not a model file", fName);
#else
		ErrorMsg(bf_tr("\xc0\xdf\xb8\xf8\xb5\xc8 \xc6\xf7\xb8\xcb\xc0\xc7 \xb8\xf0\xb5\xa8\xc6\xc4\xc0\xcf\xc0\xd3."));
#endif
		return	false;
	}
	BYTE	verMajor;
	BYTE	verMinor;
	fp.Read(&verMajor, sizeof(BYTE));
	fp.Read(&verMinor, sizeof(BYTE));
	if (verMajor != 1 || verMinor > 5)
	{
#if defined(BF_PORT)
		bf_port_missing(BFT_RES, "res.unusable", "%s: model version %d.%d", fName, verMajor, verMinor);
#else
		ErrorMsg(bf_tr("\xc1\xf6\xbf\xf8\xb5\xc7\xc1\xf6 \xbe\xca\xb4\xc2 \xb9\xf6\xc1\xaf\xc0\xd4\xb4\xcf\xb4\xd9. \xb3\xbb\xbf\xeb\xb9\xb0\xc0\xcc \xb1\xfa\xc1\xae\xc0\xd6\xc0\xbb \xbc\xf6 \xc0\xd6\xbd\xc0\xb4\xcf\xb4\xd9"));
#endif
		return	false;
	}

	fp.Read(&m_animLen, sizeof(int));
	fp.Read(&m_shadeType, sizeof(int));
	if (verMinor >= 4)
		fp.Read(&m_alpha, sizeof(BYTE));
	else
		m_alpha = 0xff;
	char	reserved[16];
	fp.Read(reserved, 16);
	int		numTexture;
	fp.Read(&numTexture, sizeof(int));
	std::string *textureNames = new std::string[numTexture];
	char	tmp[40];
	int i;
	for (i=0; i<numTexture; i++)
	{
		fp.Read(tmp, 40);
		textureNames[i] = std::string(tmp);
	}
	char	mainNode[64];

	fp.Read(tmp, 40);
	strcpy(mainNode, tmp);

	int		numNodes;
	fp.Read(&numNodes, sizeof(int));

	for (i = 0; i < numNodes; i++)
	{
		C3dNodeRes *	temp = new C3dNodeRes;
		temp->scene = this;
		temp->alpha = m_alpha;


		fp.Read(tmp, 40);
		strcpy(temp->name, tmp);


		fp.Read(tmp, 40);
		strcpy(temp->parentname, tmp);


		fp.Read(&temp->numTexture, sizeof(int));


		temp->texture = new CTexture* [temp->numTexture];

		for (int j=0; j<temp->numTexture; j++)
		{
			int	index;
			fp.Read(&index, sizeof(int));
			temp->texture[j] = g_texMgr.GetTexture(textureNames[index].c_str(), 0);
			temp->texture[j]->Lock();
		}


		matrix	offsetTM;
		fp.Read(&offsetTM, sizeof(matrix));


		fp.Read(&temp->pos, sizeof(vector3d));

		fp.Read(&temp->rotangle, sizeof(float));

		fp.Read(temp->rotaxis, sizeof(float)*3);

		fp.Read(&temp->scale, sizeof(float)*3);

		temp->mesh = new C3dMesh;

		fp.Read(&temp->mesh->m_numVert, sizeof(int));

		temp->mesh->m_vert = new vector3d[temp->mesh->m_numVert];
		fp.Read(temp->mesh->m_vert, sizeof(vector3d)*temp->mesh->m_numVert);


		for (int v=0; v<temp->mesh->m_numVert; v++)
		{
			vector3d	vertWithOffset;
			vertWithOffset.MatrixMult(temp->mesh->m_vert[v], offsetTM);
			temp->mesh->m_vert[v] = vertWithOffset;
		}

		fp.Read(&temp->mesh->m_numTVert, sizeof(int));
		temp->mesh->m_tvert = new tvertex3d[temp->mesh->m_numTVert];

		if (verMinor == 1)
		{
			for (int t=0; t<temp->mesh->m_numTVert; t++)
			{
				texCoor		tempvert;
				fp.Read(&tempvert, sizeof(texCoor));
				temp->mesh->m_tvert[t].color = 0xffffffff;
				temp->mesh->m_tvert[t].u = tempvert.u;
				temp->mesh->m_tvert[t].v = tempvert.v;
			}
		}
		else
		{
			fp.Read(temp->mesh->m_tvert, sizeof(tvertex3d)*temp->mesh->m_numTVert);
		}

		fp.Read(&temp->mesh->m_numFace, sizeof(int));
		temp->mesh->m_face = new face3d[temp->mesh->m_numFace];

		if (verMinor == 1)
		{
			for (int t=0; t<temp->mesh->m_numFace; t++) {
				face3d_v11	tempface;
				fp.Read(&tempface, sizeof(face3d_v11));
				temp->mesh->m_face[t] = tempface;
				temp->mesh->m_face[t].smoothGroup = 0;
			}
		}
		else
		{
			fp.Read(temp->mesh->m_face, sizeof(face3d)*temp->mesh->m_numFace);
		}

		temp->mesh->SortFace();
		temp->mesh->UpdateNormal();

		if (verMinor >= 5)
		{
			temp->posanim.Load(&fp);
		}
		temp->rotanim.Load(&fp);

		m_objectList.push_back(temp);
		m_meshList[temp->name] = temp->mesh;
	}

	delete[] textureNames;

	if (verMinor < 5)
	{
		C3dNodeRes *	main = FindNode(mainNode);
		main->posanim.Load(&fp);
	}


	C3dNodeResList::iterator j = m_objectList.begin();
	while (j != m_objectList.end())
	{
		if ((*j)->parentname[0])
		{
			C3dNodeRes *	t = FindNode((*j)->parentname);
			(*j)->parent = t;
			t->child.push_back(*j);
		}
		j++;
	}
	m_rootObjList.push_back(mainNode);

	int		numVol;
	fp.Read(&numVol, sizeof(int));

	if (verMajor == 1 && verMinor > 2)
	{
		for (i=0; i<numVol; i++)
		{
			CVolumeBox *	n = new CVolumeBox;
			CVolumeBox_v21	t;
			fp.Read(&t, sizeof(CVolumeBox_v21));
			n->flag = t.flag;
			n->m_pos = t.m_pos;
			n->m_rot = t.m_rot;
			n->m_size = t.m_size;
			if (n->flag == 0) {
				n->m_size.x += 3;
				n->m_size.z += 3;
			}
			m_volumeBoxList.push_back(n);
		}
	}
	else if (verMajor == 1 && verMinor == 2)
	{
		for (int i=0; i<numVol; i++) {
			CVolumeBox *	n = new CVolumeBox;
			CVolumeBox_v11	t;
			fp.Read(&t, sizeof(CVolumeBox_v11));
			n->flag = 0;
			n->m_pos = t.m_pos;
			n->m_rot = t.m_rot;
			n->m_size = t.m_size;
			m_volumeBoxList.push_back(n);
		}
	}

	fp.Close();
	strcpy(name, fName);
	return	true;
}


C3dNodeRes *	C3dModelRes :: FindNode(const char *name)
{
	C3dNodeResList::iterator i = m_objectList.begin();
	while (i != m_objectList.end()) {
		if (!(strcmp((*i)->name, name))) {
			return	*i;
		}
		i++;
	}
	return	NULL;
}

C3dFrameState :: C3dFrameState()
{
	m_vertices = NULL;
	m_tvertices = NULL;
}

void	C3dFrameState :: CreateVertex(int numVertex)
{
	m_vertices = new vector3d[numVertex];
}

void	C3dFrameState :: CreateTVertex(int numVertex)
{
	m_tvertices = new tvertex3d[numVertex];
}

C3dFrameState :: ~C3dFrameState()
{
	if (m_vertices)
		delete [] m_vertices;
	if (m_tvertices)
		delete [] m_tvertices;
}

C3dAnimNodeRes :: C3dAnimNodeRes()
{
	parent = NULL;
	model = NULL;
	m_numVertices = 0;
	m_numFaces = 0;
	m_frames = NULL;
	m_faceLight = NULL;
	m_faces = NULL;
	m_textures = NULL;
}

C3dAnimNodeRes :: ~C3dAnimNodeRes()
{
	Release();
}



void	C3dAnimNodeRes :: CalcFaceLight(matrix &ptm, int time, vector3d &light, vector3d &diffuseCol, vector3d &ambientCol, int shading, int alpha)
{
	int		i, j, g;

	static	vector3d	faceNormals[MAX_FACE_PER_MESH];
	static	vector3d	vertNormals[MAX_VERTEX_PER_MESH];
	IntList	GroupList[MAX_GROUP];
	IntList::iterator	li;

	switch (shading) {
		case	0:
			for (i=0; i<m_numFaces; i++) {
				for (int k=0; k<3; k++)
					m_faceLight[i].color[k] = (alpha<<24) + 0x00ffffff;
			}
			break;
		case	1:
			for (i=0; i<m_numFaces; i++) {

				face3d &	face = m_faces[i];
				vector3d &	v1 = m_frames[time].m_vertices[face.vertindex[0]];
				vector3d &	v2 = m_frames[time].m_vertices[face.vertindex[1]];
				vector3d &	v3 = m_frames[time].m_vertices[face.vertindex[2]];

				vector3d	wv1, wv2, wv3;
				wv1.MatrixMult(v1, ptm);
				wv2.MatrixMult(v2, ptm);
				wv3.MatrixMult(v3, ptm);
				vector3d	wnormal;
				wnormal.CrossProduct(wv3-wv2, wv3-wv1);
				wnormal.Normalize();

				float	product = wnormal.x*light.x + wnormal.y*light.y + wnormal.z*light.z;


				if (product < 0)
					product = 0;

				float	r, g, b;
				int		intr, intg, intb;

				r = ambientCol.x + product*diffuseCol.x;
				if (r > 1)
					r = 1;
				g = ambientCol.y + product*diffuseCol.y;
				if (g > 1)
					g = 1;
				b = ambientCol.z + product*diffuseCol.z;
				if (b > 1)
					b = 1;
				for (int k=0; k<3; k++) {
					int		time2 = time;
					if (m_tvertIdentity) {
						time2 = 0;
					}
					tvertex3d	&	tv = m_frames[time2].m_tvertices[face.tvertindex[k]];
					if (tv.color) {
						intr = r * 128;
						intg = g * 128;
						intb = b * 128;
					} else {
						intr = r * 255;
						intg = g * 255;
						intb = b * 255;
					}
					m_faceLight[i].color[k] = (alpha<<24) | (intr << 16) | (intg << 8) | intb;
				}


			}
			break;
		case	2:

			for (i=0; i<m_numFaces; i++) {

				face3d &	face = m_faces[i];
				vector3d &	v1 = m_frames[time].m_vertices[face.vertindex[0]];
				vector3d &	v2 = m_frames[time].m_vertices[face.vertindex[1]];
				vector3d &	v3 = m_frames[time].m_vertices[face.vertindex[2]];

				vector3d	wv1, wv2, wv3;
				wv1.MatrixMult(v1, ptm);
				wv2.MatrixMult(v2, ptm);
				wv3.MatrixMult(v3, ptm);
				faceNormals[i].CrossProduct(wv3-wv2, wv3-wv1);
				faceNormals[i].Normalize();
				GroupList[face.smoothGroup].push_back(i);
			}

			for (g=0; g<MAX_GROUP; g++) {
				if (GroupList[g].size() == 0)
					continue;
				for (int i=0; i<m_numVertices; i++) {
					vertNormals[i].Set(0, 0, 0);
					for (j=0; j<m_numFaces; j++) {
						face3d &	face = m_faces[j];
						if (face.smoothGroup != g)
							continue;
						if (face.vertindex[0] == i || face.vertindex[1] == i || face.vertindex[2] == i)
							vertNormals[i] += faceNormals[j];
					}
					if (vertNormals[i].Magnitude() != 0)
						vertNormals[i].Normalize();
				}
				for (li=GroupList[g].begin(); li!=GroupList[g].end(); li++) {
					face3d &	face = m_faces[(*li)];
					for (int k=0; k<3; k++) {
						vector3d &	wnormal = vertNormals[face.vertindex[k]];
						float	product = wnormal.x*light.x + wnormal.y*light.y + wnormal.z*light.z;

						if (product < 0)
							product = 0;

						float	r, g, b;
						int		intr, intg, intb;

						r = ambientCol.x + product*diffuseCol.x;
						if (r > 1)
							r = 1;
						g = ambientCol.y + product*diffuseCol.y;
						if (g > 1)
							g = 1;
						b = ambientCol.z + product*diffuseCol.z;
						if (b > 1)
							b = 1;
						int		time2 = time;
						if (m_tvertIdentity) {
							time2 = 0;
						}
						tvertex3d	&	tv = m_frames[time2].m_tvertices[face.tvertindex[k]];
						if (tv.color) {
							intr = r * 128;
							intg = g * 128;
							intb = b * 128;
						} else {
							intr = r * 255;
							intg = g * 255;
							intb = b * 255;
						}
						m_faceLight[*li].color[k] = (alpha<<24) | (intr << 16) | (intg << 8) | intb;


					}
				}
			}
			break;
		default:
			{
				int i;
				for (i=0; i<m_numFaces; i++) {
					for (int k=0; k<3; k++)
						m_faceLight[i].color[k] = (alpha<<24) + 0x00ffffff;
				}
			}
			ErrorMsg("Invalid Shade Type");
			break;
	}
	C3dAnimNodeResList::iterator it = child.begin();
	while (it != child.end()) {
		(*it)->CalcFaceLight(ptm, time, light, diffuseCol, ambientCol, shading, alpha);
		it++;
	}
}


void	C3dAnimNodeRes :: Release()
{
	if (m_frames) {
		delete [] m_frames;
		m_frames = NULL;
	}
	if (m_faces) {
		delete [] m_faces;
		m_faces = NULL;
	}
	if (m_faceLight) {
		delete [] m_faceLight;
		m_faceLight = NULL;
	}
	if (m_textures) {
		for (int i=0; i<m_numTextures; i++) {
			m_textures[i]->Unlock();
			g_texMgr.DestroyTexture(m_textures[i]);
		}
		delete [] m_textures;
		m_textures = NULL;
	}
}


static C3dAnimModelRes C3dAnimModelResInstance(Exemplar(), "rsx", "model\\");


C3dAnimModelRes :: C3dAnimModelRes()
{
	m_numMaterials = 0;
	m_material = NULL;
	m_numFrame = 0;
	m_tickPerFrame = 0;
	m_zoom = 1.0f;
	m_shade = 2;
	m_pivot.Set(0, 0, 0);
	m_isShaded = FALSE;
	m_alpha = 0xff;
}

C3dAnimModelRes :: ~C3dAnimModelRes()
{
	Release();
}

void	C3dAnimModelRes :: Release()
{
	if (m_material) {
		delete [] m_material;
		m_material = NULL;
		m_numMaterials = 0;
	}
	C3dAnimNodeResList::iterator i = m_objectList.begin();
	while (i != m_objectList.end()) {
		delete *i;
		i++;
	}
	m_objectList.erase(m_objectList.begin(), m_objectList.end());
}

C3dAnimNodeRes *	C3dAnimModelRes :: FindNode(const char *name)
{
	C3dAnimNodeResList::iterator i = m_objectList.begin();
	while (i != m_objectList.end()) {
		if ((*i)->name == name) {
			return	*i;
		}
		i++;
	}
	return	NULL;
}

#include "rag_budget_fill.h"
bool	C3dAnimModelRes :: Load(const char *fName)
{
	CFile	fp;

	if(fp.Open(fName, CFile::modeRead) == false)
		return false;


	char	magicBuf[5];

	fp.Read(magicBuf, 4);
	magicBuf[4]=0;
	BYTE	verMajor;
	BYTE	verMinor;
	fp.Read(&verMajor, sizeof(BYTE));
	fp.Read(&verMinor, sizeof(BYTE));


	if (verMajor == 1 && verMinor >=  2) {
		fp.Read(&m_zoom, sizeof(float));
		fp.Read(&m_pivot, sizeof(vector3d));
	}
	if (verMajor == 1 && verMinor ==  3)
		fp.Read(&m_alpha, sizeof(int));
	fp.Read(&m_shade, sizeof(int));
	fp.Read(&m_numFrame, sizeof(int));
	fp.Read(&m_tickPerFrame, sizeof(int));
	char	reserved[16];
	fp.Read(reserved, 16);

	char	tmp[40];
	int		numTexture;
	string *textureNames;

	fp.Read(&numTexture, sizeof(int));
	textureNames = new string[numTexture];
	int i ;
	for (i=0; i<numTexture; i++) {
		fp.Read(tmp, 40);
		textureNames[i] = string(tmp);
		g_texMgr.GetTexture(tmp, 0);
	}
	string	mainNode;

	fp.Read(tmp, 40);
	mainNode = string(tmp);

	int		numNodes;
	fp.Read(&numNodes, sizeof(int));

	for (i=0; i<numNodes; i++)
	{
		C3dAnimNodeRes *	temp = new C3dAnimNodeRes;
		temp->model = this;
		temp->m_frames = new C3dFrameState[m_numFrame];

		int		nodeHeader;
		fp.Read(&nodeHeader, sizeof(int));
		if (nodeHeader != 0x12345678) {
			ErrorMsg("data corrupt");
			return	false;
		}

		fp.Read(tmp, 40);
		temp->name = string(tmp);


		fp.Read(tmp, 40);
		temp->parentname = string(tmp);


		fp.Read(&temp->m_numTextures, sizeof(int));


		temp->m_textures = new CTexture* [temp->m_numTextures];
		for (int j=0; j<temp->m_numTextures; j++) {
			int	index;
			fp.Read(&index, sizeof(int));
			string	texName = textureNames[index];
			temp->m_textures[j] = g_texMgr.GetTexture(texName.c_str(), 0);
			temp->m_textures[j]->Lock();
		}

		RAG_BUDGET_FILL(4)
		fp.Read(&temp->m_numVertices, sizeof(int));

		int k;
		for (k=0; k<m_numFrame; k++) {
			temp->m_frames[k].CreateVertex(temp->m_numVertices);
			fp.Read(temp->m_frames[k].m_vertices, sizeof(vector3d)*temp->m_numVertices);
		}

		fp.Read(&temp->m_numTVertices, sizeof(int));

		fp.Read(&temp->m_tvertIdentity, sizeof(BOOL));
		if (temp->m_tvertIdentity) {
			temp->m_frames[0].CreateTVertex(temp->m_numTVertices);
			fp.Read(temp->m_frames[0].m_tvertices, sizeof(tvertex3d)*temp->m_numTVertices);
		} else {
			for (k=0; k<m_numFrame; k++) {
				temp->m_frames[k].CreateTVertex(temp->m_numTVertices);
				fp.Read(temp->m_frames[k].m_tvertices, sizeof(tvertex3d)*temp->m_numTVertices);
			}
		}

		fp.Read(&temp->m_numFaces, sizeof(int));

		temp->m_faceLight = new faceLight[temp->m_numFaces];
		temp->m_faces = new face3d[temp->m_numFaces];
		fp.Read(temp->m_faces, sizeof(face3d)*temp->m_numFaces);

		m_objectList.push_back(temp);
		int		nodeFooter;
		fp.Read(&nodeFooter, sizeof(int));
		if (nodeFooter != 0x12345678) {
			ErrorMsg("data corrupt");
			return	false;
		}
	}
	fp.Close();
	delete [] textureNames;


	C3dAnimNodeResList::iterator j = m_objectList.begin();
	while (j != m_objectList.end()) {
		if (!(*j)->parentname.empty()) {
			C3dAnimNodeRes *	t = FindNode((*j)->parentname.c_str());
			(*j)->parent = t;
			t->child.push_back(*j);
		}
		j++;
	}
	m_rootObjList.push_back(mainNode.c_str());

	return	true;
}

static C3dGrannyModelRes C3dGrannyModelResInstance(Exemplar(), "gr2", "model\\3dmob\\");


C3dGrannyModelRes :: C3dGrannyModelRes()
{
	m_Scene.Model = NULL;
	m_pModel = NULL;
	m_nBoneType = 0;
	m_hair = NULL;
}

C3dGrannyModelRes :: ~C3dGrannyModelRes()
{
	if( m_Scene.Model ) Release();
}

bool C3dGrannyModelRes :: Load( const char* fName )
{
  bool Result = false;
	GrannyModel *Model;

  m_pBoneRes = &g_GrannyBoneRes;
  m_Scene.ModelCount = 0;
  m_Scene.MaxBoneCount = 0;
  m_Scene.TextureCount = 0;
  m_Scene.MaxMutableVertexBufferCount = 0;

	const char* pCh = strchr( fName, '_' );
	if( pCh==NULL ) return false;

	int i=0;
	char	str[10];
	pCh++;
	while( *pCh!='.' && i<4 ) {
		str[i] = *pCh;
		i++, pCh++;
	}
	str[i] = 0;
	m_nBoneType = atoi(str);

	CFile	fp;

	if (!fp.Open(fName, CFile::modeRead)) {
#if !defined(BF_PORT)
		// The port: CResMgr::Get reports the missing file (res.missing).
		ErrorMsg((string("Cannot find File : ") + string(fName)).c_str());
#endif
		return	false;
	}

	m_Scene.LoadedFile = bf_granny_open((void*)fp.GetBuf (), fp.GetLength (), NULL, 0);
	fp.Close();
  if(m_Scene.LoadedFile)
  {
	const bf_granny_info *FileInfo = bf_granny_get_info(m_Scene.LoadedFile);
	  strcpy( m_Scene.BaseFileName, fName);

	if(FileInfo) {
		m_Scene.TextureCount = FileInfo->texture_count;
		m_Scene.Textures = new GrannyTexture[m_Scene.TextureCount];
		{
					for(int TextureIndex = 0;TextureIndex < m_Scene.TextureCount;TextureIndex++)
		  {
						m_Scene.Textures[TextureIndex].tex = NULL;
			CreateTexture(&m_Scene.Textures[TextureIndex], TextureIndex);
					}
				}

		m_Scene.ModelCount = FileInfo->model_count;

		sAssert(m_Scene.Model == NULL);

		m_Scene.Model = new GrannyModel;


		{

		  const bf_granny_model *GrannyModel = bf_granny_get_model(m_Scene.LoadedFile, 0);
		  Model = m_Scene.Model;
		  CreateModel(Model, 0);
					m_pSkeleton = bf_granny_get_skeleton(m_Scene.LoadedFile, 0);
					m_pModel = GrannyModel;
		}

		m_Scene.SharedLocalPose = new bf_granny_transform[m_Scene.MaxBoneCount];

		if(m_Scene.MaxMutableVertexBufferSize)
		{
		  m_Scene.MaxMutableVertexBufferCount = 4;
		}

		Result = true;
	  }
	  else
	  {
		ErrorMsg("Unable to access file data .gr2");
	  }
  }
  else
  {
#if defined(BF_PORT)
	// Missing data never stops the client: the file is refused, the
	// resource not made (the loop below would read a model that is not there).
	bf_port_missing(BFT_RES, "res.unusable", "%s: not a Granny file the reader takes", fName);
	return false;
#else
	ErrorMsg("Unable to load file .gr2 (C3dGrannyModelRes :: Load)");
#endif
  }

  SetProjectZ(1600.0f);

	int nHighVertex = 0, nVertex;
	for( i=0;i<m_Scene.Model->MeshCount;i++ ) {
		nVertex = GetMeshIndexCount(i);
		if( nVertex>nHighVertex ) nHighVertex = nVertex;
		if( nVertex>MAX_GRANNY_VERTEX ) {
			ErrorMsg("too many vertex granny model!");
		}
	}

  m_HitareaPos.x = 0.0f, m_HitareaPos.y = 0.0f, m_HitareaPos.z = 0.0f;
	m_HitRadx = 10.0f, m_HitRady = 10.0f;
  return(Result);
}

bf_granny_control* C3dGrannyModelRes :: ActAnimation( int idx, bf_granny_instance* instance, bf_granny_control* control, int nLoop )
{
	if ( control==NULL ) {
		const bf_granny_file* animation = m_pBoneRes->GetAnimation(m_nBoneType, idx);
		if( animation==NULL ) return NULL;
		control = bf_granny_play(instance, 0.0f, animation, 0);
		if( control==NULL ) return NULL;
		bf_granny_set_loop_count(control, nLoop);

	}
	else {
		bf_granny_set_active(control, true);
	}
	return control;
}

void C3dGrannyModelRes :: DeActAnimation( bf_granny_control* control )
{
	if( control ) {
		bf_granny_set_active(control, false);
		bf_granny_free_control(control);

	}
}

void C3dGrannyModelRes :: Release()
{

	if( m_Scene.Model )
  {
	GrannyModel *Model = m_Scene.Model;

	for(int MeshIndex = 0;MeshIndex < Model->MeshCount;MeshIndex++)
	{
	  GrannyMesh &Mesh = Model->Meshes[MeshIndex];

	  bf_granny_free_mesh_binding(Mesh.Binding);

	  delete [] Mesh.TextureReferences;
			delete Mesh.VerIdx ;
			if( Mesh.Vertices ) delete Mesh.Vertices ;
	}

	bf_granny_free_instance(Model->GrannyInstance);
	bf_granny_free_world_pose(Model->WorldPose);
	delete [] Model->Meshes;
  }

	for(int TextureIndex = 0;TextureIndex < m_Scene.TextureCount;TextureIndex++)
	{
		GrannyTexture &Texture = m_Scene.Textures[TextureIndex];
		delete Texture.tex;
	}
	delete [] m_Scene.Textures;

  delete [] m_Scene.SharedLocalPose;
  bf_granny_close(m_Scene.LoadedFile);

  delete m_Scene.Model;
	m_Scene.Model = NULL;
}

void C3dGrannyModelRes :: CreateTexture(GrannyTexture *Texture, int TextureIndex)
{
  const bf_granny_texture *GrannyTexture = bf_granny_get_texture(m_Scene.LoadedFile, TextureIndex);
  Texture->Name = GrannyTexture->file_name;

  // a colour map (texture type 0) of one image
  if((GrannyTexture->texture_type == 0) && (GrannyTexture->image_count == 1))
  {

	{
	  Texture->tex = CreateRGBATexture(TextureIndex, 0);
	}
  }
  else
  {
	ErrorMsg("This sample only works with single-image : ColorMapTextureType textures");
  }
}

CTexture* C3dGrannyModelRes :: CreateRGBATexture(int TextureIndex, int ImageIndex)
{
  LPDIRECTDRAWSURFACE7 D3DTexture = NULL;
	CTexture* RetD3DTexture = NULL;

  const bf_granny_texture *GrannyTexture = bf_granny_get_texture(m_Scene.LoadedFile, TextureIndex);
  int HasAlpha = 0;
  int Width = GrannyTexture->width;
  int Height = GrannyTexture->height;
  unsigned char *PixelBuffer2 = new BYTE[Width * Height * 4];

	// B, G, R, A: the 2008 client swapped GrannyCopyTextureImage's RGBA8888
	// into this order; the reader decodes into it. A texture it cannot
	// decode is black (and opaque below), as granny2.dll left the copy.
	if( !bf_granny_texture_pixels(m_Scene.LoadedFile, TextureIndex, PixelBuffer2, &HasAlpha) ) {
		bf_count("granny.texture_failed", 1);
		memset(PixelBuffer2, 0, Width * Height * 4);
		HasAlpha = 0;
	}

	int didx=0;

	if( HasAlpha ) {
		RetD3DTexture = g_texMgr.CreateTexture( Width, Height, (DWORD*)PixelBuffer2, PF_A4R4G4B4, 0 );
	}
	else {
		for( int i=0;i<Height;i++ ) {
			for( int j=0;j<Width;j++ ) {
				PixelBuffer2[didx+3] =  255;
				didx+=4;
			}
		}
		RetD3DTexture = g_texMgr.CreateTexture( Width, Height, (DWORD*)PixelBuffer2, PF_A1R5G5B5, 0 );
	}

	delete [] PixelBuffer2;

  return(RetD3DTexture);
}

void C3dGrannyModelRes :: CreateModel(GrannyModel *Model, int ModelIndex)
{
  const bf_granny_model *GrannyModel = bf_granny_get_model(m_Scene.LoadedFile, ModelIndex);
  int BoneCount = bf_granny_get_skeleton(m_Scene.LoadedFile, GrannyModel->skeleton)->bone_count;
  if(m_Scene.MaxBoneCount < BoneCount)
  {
	m_Scene.MaxBoneCount = BoneCount;
  }

  Model->GrannyInstance = bf_granny_new_instance(m_Scene.LoadedFile, ModelIndex);

  Model->WorldPose = bf_granny_new_world_pose(BoneCount);

  Model->MeshCount = GrannyModel->mesh_count;
  Model->Meshes = new GrannyMesh[Model->MeshCount];
  for(int MeshIndex = 0;MeshIndex < Model->MeshCount;MeshIndex++)
  {
	CreateMesh(&Model->Meshes[MeshIndex], GrannyModel->meshes[MeshIndex], Model->GrannyInstance);
  }
}

void C3dGrannyModelRes :: CreateMesh(GrannyMesh *Mesh, int MeshIndex, bf_granny_instance *InModel)
{
  const bf_granny_mesh *GrannyMesh = bf_granny_get_mesh(m_Scene.LoadedFile, MeshIndex);
  Mesh->Mesh = GrannyMesh;
  Mesh->Index = MeshIndex;

  const bf_granny_skeleton *Skeleton = bf_granny_instance_skeleton(InModel);
  Mesh->Binding = bf_granny_bind_mesh(GrannyMesh, Skeleton);

  int IndexCount = GrannyMesh->index_count;
  int IndexBufferSize = sizeof(unsigned short) * IndexCount;

  Mesh->VerIdx = new WORD[IndexCount];

  if(IndexCount) {
	  bf_granny_copy_indices16(GrannyMesh, Mesh->VerIdx);
  }

  int VertexCount = GrannyMesh->vertex_count;
  int VertexBufferSize = VertexCount * sizeof(bf_granny_pnt332);

	Mesh->Vertices = NULL;

  if(GrannyMesh->rigid) {
		Mesh->Vertices = new BYTE[sizeof(bf_granny_pnt332)*VertexCount];
		bf_granny_copy_vertices(GrannyMesh, (bf_granny_pnt332*)Mesh->Vertices);

	Mesh->Deformer = false;
  }
  else
  {
	if(m_Scene.MaxMutableVertexBufferSize < VertexBufferSize) {
		m_Scene.MaxMutableVertexBufferSize = VertexBufferSize;
	}

	// bf_granny_deform skins position and normal into PNT332, as the
	// deformer granny2.dll made for it
	Mesh->Deformer = true;
  }

  Mesh->TextureCount = GrannyMesh->material_count;
  Mesh->TextureReferences = new GrannyTexture *[Mesh->TextureCount];
  for(int TextureIndex = 0;TextureIndex < Mesh->TextureCount;TextureIndex++) {
	Mesh->TextureReferences[TextureIndex] = FindTexture(GrannyMesh->materials[TextureIndex]);
  }
}

GrannyTexture* C3dGrannyModelRes :: FindTexture(int Material)
{
	GrannyTexture *Texture = NULL;

  // the material's diffuse colour map
  const bf_granny_material *GrannyMaterial = bf_granny_get_material(m_Scene.LoadedFile, Material);
  const bf_granny_texture *GrannyTexture = GrannyMaterial ? bf_granny_get_texture(m_Scene.LoadedFile, GrannyMaterial->diffuse_texture) : NULL;

  if(GrannyTexture)
  {

		for(int TextureIndex = 0;TextureIndex < m_Scene.TextureCount;TextureIndex++)
	{
	  Texture = &m_Scene.Textures[TextureIndex];
	  if(strcmp(Texture->Name, GrannyTexture->file_name) == 0) {
		return(Texture);
	  }
		}
  }
  return(0);
}

void C3dGrannyModelRes :: SetProjectZ(float fClip)
{
	m_ProjectZ = fClip + fClip / 5.0f;
}

bf_granny_world_pose*  C3dGrannyModelRes :: UpdateAni(float GameClock, bf_granny_instance* instance)
{

  GrannyModel *Model = m_Scene.Model;
	if( Model==NULL ) return NULL;

	const bf_granny_skeleton *Skeleton = bf_granny_instance_skeleton(instance);

  int BoneCount = Skeleton->bone_count;

  bf_granny_set_clock(instance, GameClock);
  bf_granny_sample_instance(instance, m_Scene.SharedLocalPose, m_Scene.MaxBoneCount);
  bf_granny_build_world_pose( Skeleton, m_Scene.SharedLocalPose, m_Scene.MaxBoneCount, Model->WorldPose);


	return Model->WorldPose;
}

VOID* C3dGrannyModelRes :: RenderMesh(int nMeshIdx, INT_PTR* RetTex, INT_PTR* RetFaceArr, int* nIndexNo, matrix* poseMat,
																			bf_granny_world_pose* pWorldpose )
{
  return RenderModel(m_Scene.Model, nMeshIdx, RetTex, RetFaceArr, nIndexNo, poseMat, pWorldpose );
}

VOID* C3dGrannyModelRes :: GetRigidMesh( int nMeshIdx )
{
	GrannyMesh *Mesh;
	Mesh = &m_Scene.Model->Meshes[nMeshIdx];
	if(Mesh->Mesh->rigid) {
		return (VOID*)Mesh->Vertices;
	}
	return NULL;
}

bool C3dGrannyModelRes :: SetHairAndHeadMesh( void )
{
	if( !m_hair ) return false;

	for( int i=0;i<m_Scene.Model->MeshCount;i++ ) {
		GrannyMesh* Mesh = &m_Scene.Model->Meshes[i];

		if( strstr(Mesh->Mesh->name, "hair_pivot") ) {
			m_hair->nSubMeshIdx = i;
			return true;
		}
	}
	return false;
}

VOID* C3dGrannyModelRes :: RenderModel(GrannyModel *Model, int nMeshIdx, INT_PTR* RetTex, INT_PTR* RetFaceArr, int* nIndexNo,
																			 matrix* poseMat, bf_granny_world_pose* pWorldpose )
{
	GrannyMesh *Mesh;
	VOID*	ret = NULL;





	if( pWorldpose==NULL ) {
		pWorldpose = Model->WorldPose;
	}
  float *CompositeBuffer = bf_granny_world_pose_composite(pWorldpose, 0);


  {
	Mesh = &Model->Meshes[nMeshIdx];



	int *ToBoneIndices = Mesh->Binding;
		int VertexCount = Mesh->Mesh->vertex_count;
		const bf_granny_tri_group *Group = Mesh->Mesh->groups;

		// read only within the mesh's materials (a mesh without any has an empty array)
		GrannyTexture *Texture = Mesh->TextureCount>Group->material ? Mesh->TextureReferences[Group->material] : NULL;
		if(Texture && Mesh->TextureCount>Group->material) {
			*RetTex = (INT_PTR)Texture->tex;
		}
		else {
			*RetTex = NULL;
		}

		if(Mesh->Mesh->rigid) {

		float* Transform = bf_granny_world_pose_composite(Model->WorldPose, ToBoneIndices[0]);

				poseMat->SetRow1(Transform[0],Transform[1],Transform[2]);
				poseMat->SetRow2(Transform[4],Transform[5],Transform[6]);
				poseMat->SetRow3(Transform[8],Transform[9],Transform[10]);
				poseMat->SetRow4(Transform[12],Transform[13],Transform[14]);

				ret = (VOID*)Mesh->Vertices;
				*RetFaceArr = (INT_PTR)Mesh->VerIdx;
				*nIndexNo = Mesh->Mesh->index_count/3;

				if( strstr(Mesh->Mesh->name, "hair_pivot") ) {
					bf_granny_pnt332* srcVert = (bf_granny_pnt332*)Mesh->Vertices;
					if( m_hair ) {



						m_hair->poseMat = *poseMat;
					}
				}

	}
		else
	if(Mesh->Deformer) {

	  bf_granny_deform(m_Scene.LoadedFile, Mesh->Index,
						   ToBoneIndices, CompositeBuffer,
						   m_Scene.MatVer);

			ret = (VOID*)m_Scene.MatVer;
			*RetFaceArr = (INT_PTR)Mesh->VerIdx;
			*nIndexNo = Mesh->Mesh->index_count/3;

			poseMat->MakeIdentity();
	}
  }
  return ret;
}

bool C3dGrannyModelRes :: CalculateHitArea(RENDER_INFO_RECT &info, matrix &view, matrix &dispTrans, vector3d &cam)
{
	w3dtlvertex		cpos;
	float			halfrecsizex, halfrecsizey;
	float			px, py, adj;
	vector3d		sour, chkvec;
	matrix			tr;

	tr.MakeZRotation(3.141592f);
	sour.MatrixMult(m_HitareaPos, tr);
	tr.MultMatrix(dispTrans,view);

	cpos.sx = sour.x*tr.v11 + sour.y*tr.v21 + sour.z*tr.v31 + tr.v41;
	cpos.sy = sour.x*tr.v12 + sour.y*tr.v22 + sour.z*tr.v32 + tr.v42;
	cpos.sz = sour.x*tr.v13 + sour.y*tr.v23 + sour.z*tr.v33 + tr.v43;

	chkvec.x = cam.x - sour.x;
	chkvec.y = cam.y - sour.y;
	chkvec.z = cam.z - sour.z;
	chkvec.Normalize();

	chkvec.y = (float)fabs(chkvec.y);
	if (chkvec.y > 1.0f)
		chkvec.y = 1.0f;

	adj = m_HitRady - ((m_HitRady - m_HitRadx) * chkvec.y);

	halfrecsizex = _sqrt((m_HitRadx*m_HitRadx) * 0.5f);
	halfrecsizey = _sqrt((adj*adj) * 0.5f);
	px = cpos.sx - halfrecsizex;
	py = cpos.sy - halfrecsizey;
	info.oow	= 1.0f / cpos.sz;
	info.left	= g_renderer->m_xoffset + (px*g_renderer->m_hpc) * info.oow;
	info.top	= g_renderer->m_yoffset + (py*g_renderer->m_vpc) * info.oow;

	px = cpos.sx + halfrecsizex;
	py = cpos.sy + halfrecsizey;

	info.right = g_renderer->m_xoffset + (px*g_renderer->m_hpc) * info.oow;
	info.bottom = g_renderer->m_yoffset + (py*g_renderer->m_vpc) * info.oow;

	// through int, as the 2008 build's __ftol: a negative edge wraps to a large DWORD
	if ((DWORD)(int)info.left>(DWORD)g_renderer->m_width && (DWORD)(int)info.right>(DWORD)g_renderer->m_width &&
		(DWORD)(int)info.top>(DWORD)g_renderer->m_height && (DWORD)(int)info.bottom>(DWORD)g_renderer->m_height)
		return	FALSE;

	return	TRUE;
}

bf_granny_file* C3dGrannyModelRes :: LoadFile(char const *BaseFileName, char const *Suffix)
{
	static char FileNameBuffer[512];
	sprintf(FileNameBuffer, "%s%s.gr2", BaseFileName, Suffix);

		CFile	fp;
		if (!fp.Open(FileNameBuffer, CFile::modeRead)) {
			ErrorMsg((string("Cannot find File : ") + string(FileNameBuffer)).c_str());
			return	NULL;
		}
		bf_granny_file *File = bf_granny_open((void*)fp.GetBuf (), fp.GetLength (), NULL, 0);
		fp.Close();

	return(File);
}

int C3dGrannyModelRes :: GetMeshIndexCount( int nMeshNo )
{
	return m_Scene.Model->Meshes[nMeshNo].Mesh->index_count/3;
}

void C3dGrannyModelRes :: BindMeshToSkeleton( C3dGrannyModelRes* pDstRes )
{
	GrannyModel* pSrcModel = m_Scene.Model;

	for( int i=0;i<GetMeshNo();i++ ) {
		GrannyMesh* mesh = &pSrcModel->Meshes[i];
		const bf_granny_mesh* pMesh = mesh->Mesh;
	bf_granny_free_mesh_binding(mesh->Binding);
		mesh->Binding = bf_granny_bind_mesh(mesh->Mesh, pDstRes->GetSkeleton() );
	}
}

C3dGrannyBoneRes :: C3dGrannyBoneRes( void )
{
	m_Scene.Model = NULL;
	for( int i=0;i<MAX_BONE_TYPE;i++ ) {
		for( int j=0;j<MAX_GRANNY_CONTROL;j++ ) {
			m_Animation[i][j] = NULL;
		}
	}
}

C3dGrannyBoneRes :: ~C3dGrannyBoneRes()
{
	if( m_Scene.Model ) Release();
}

void C3dGrannyBoneRes :: Init( void )
{
}

bool C3dGrannyBoneRes :: AddBone( const char* fName, int nType, int nAniIdx )
{
  bool Result = false;
	GrannyModel *Model;

  m_Scene.ModelCount = 0;
  m_Scene.MaxBoneCount = 0;
  m_Scene.TextureCount = 0;
  m_Scene.MaxMutableVertexBufferCount = 0;


	CFile	fp;

	if (fp.Open(fName, CFile::modeRead)) {
		m_Scene.LoadedFile = bf_granny_open((void*)fp.GetBuf (), fp.GetLength (), NULL, 0);
		fp.Close();

		if(m_Scene.LoadedFile) {
			const bf_granny_info *FileInfo = bf_granny_get_info(m_Scene.LoadedFile);
			strcpy( m_Scene.BaseFileName, fName );

			if(FileInfo) {
				m_Scene.TextureCount = FileInfo->texture_count;
				m_Scene.ModelCount = FileInfo->model_count;

				if( m_Scene.Model ) {
					FreeModel();
				}

				m_Scene.Model = new GrannyModel;

				{

					Model = m_Scene.Model;
					CreateModel(Model, 0);

					if(FileInfo->animation_count)
					{

						bf_granny_file *Animation = 0;
						char *c = (char*)strchr(m_Scene.BaseFileName, '.');
						if( c!=NULL ) *c = 0;

						bf_granny_file *File = LoadFile(m_Scene.BaseFileName, "");
						if(File)
						{
							const bf_granny_info *FileInfo = bf_granny_get_info(File);
							if(FileInfo)
							{
								if(Model->GrannyInstance && (FileInfo->animation_count == 1))
								{
									Animation = File;
								}
								else
								{
									ErrorMsg("This sample only supports single-animation files");
								}
							}
						}

						m_Animation[nType][nAniIdx] = Animation;
						if( !Animation ) return false;
					}
				}

				m_Scene.SharedLocalPose = new bf_granny_transform[m_Scene.MaxBoneCount];

				if(m_Scene.MaxMutableVertexBufferSize)
				{
					m_Scene.MaxMutableVertexBufferCount = 4;
				}

				Result = true;
			}
			else
			{
				ErrorMsg("Unable to access file data .gr2");
			}

			SetProjectZ(1600.0f);
			return Result;
		}
	}

	return false;
}

const bf_granny_file* C3dGrannyBoneRes :: GetAnimation( int nType, int nAniIdx )
{
	bf_granny_file **pAni = &m_Animation[nType][nAniIdx];
	if ( *pAni == NULL )
	{
		const int MAX_ANIIDX = 5;
		const int MAX_TYPE = 10;
		static char * name[MAX_ANIIDX] = {"stand","move","attack","dead","damage" };
		static JOBTYPE STANDNAME[MAX_TYPE] = { JT_EMPELIUM,JT_GUILD_FLAG,JT_TREASURE_BOX1,
			JT_MON_BEGIN,JT_MON_BEGIN,JT_MON_BEGIN,JT_MON_BEGIN,
			JT_KNIGHT_GUARDIAN,JT_ARCHER_GUARDIAN,JT_SOLDIER_GUARDIAN };
		static char buf[MAX_PATH];
		buf[0] = NULL;

		if ( nType >= 0 && nType < MAX_TYPE && nAniIdx >= 0 && nAniIdx < MAX_ANIIDX )
		{
			if ( nAniIdx == ST_STAND )
			{

				sprintf(buf,"model\\3dmob\\%s",g_session.GetJobName( STANDNAME[nType] ));
			}
			else
			{
				sprintf(buf,"model\\3dmob_bone\\%d_%s.gr2",nType,name[nAniIdx]);
			}
		}
		else
			return NULL;

		if ( buf[0] != NULL && CFile::IsFileExist(buf) )
		{
			if (nType != 1)
				AddBone(buf,nType,nAniIdx);
		}
		else
			return NULL;
	}

	return *pAni;
}

void C3dGrannyBoneRes :: FreeModel( void )
{
	if( m_Scene.Model )
  {
	GrannyModel *Model = m_Scene.Model;

	for(int MeshIndex = 0;MeshIndex < Model->MeshCount;MeshIndex++)
	{
	  GrannyMesh &Mesh = Model->Meshes[MeshIndex];

	  bf_granny_free_mesh_binding(Mesh.Binding);

			delete Mesh.VerIdx ;
	}

	bf_granny_free_instance(Model->GrannyInstance);
	bf_granny_free_world_pose(Model->WorldPose);
	delete [] Model->Meshes;
  }
  delete m_Scene.Model;
	m_Scene.Model = NULL;
}

void C3dGrannyBoneRes :: Release()
{

	FreeModel();

	for( int i=0;i<MAX_BONE_TYPE;i++ ) {
		for( int j=0;j<MAX_GRANNY_CONTROL;j++ ) {
			if( m_Animation[i][j] ) {
				m_Animation[i][j] = NULL;
			}
		}
	}

  delete [] m_Scene.SharedLocalPose;
  bf_granny_close(m_Scene.LoadedFile);
	}

void C3dGrannyBoneRes :: CreateModel(GrannyModel *Model, int ModelIndex)
{
  const bf_granny_model *GrannyModel = bf_granny_get_model(m_Scene.LoadedFile, ModelIndex);
  int BoneCount = bf_granny_get_skeleton(m_Scene.LoadedFile, GrannyModel->skeleton)->bone_count;
  if(m_Scene.MaxBoneCount < BoneCount)
  {
	m_Scene.MaxBoneCount = BoneCount;
  }

  Model->GrannyInstance = bf_granny_new_instance(m_Scene.LoadedFile, ModelIndex);

  Model->WorldPose = bf_granny_new_world_pose(BoneCount);

  Model->MeshCount = GrannyModel->mesh_count;
  Model->Meshes = new GrannyMesh[Model->MeshCount];
  for(int MeshIndex = 0;MeshIndex < Model->MeshCount;MeshIndex++)
  {
	CreateMesh(&Model->Meshes[MeshIndex], GrannyModel->meshes[MeshIndex], Model->GrannyInstance);
  }
}

void C3dGrannyBoneRes :: CreateMesh(GrannyMesh *Mesh, int MeshIndex, bf_granny_instance *InModel)
{
  const bf_granny_mesh *GrannyMesh = bf_granny_get_mesh(m_Scene.LoadedFile, MeshIndex);
  Mesh->Mesh = GrannyMesh;
  Mesh->Index = MeshIndex;

  const bf_granny_skeleton *Skeleton = bf_granny_instance_skeleton(InModel);
  Mesh->Binding = bf_granny_bind_mesh(GrannyMesh, Skeleton);

  int IndexCount = GrannyMesh->index_count;
  int IndexBufferSize = sizeof(unsigned short) * IndexCount;

  Mesh->VerIdx = new WORD[IndexCount];

  if(IndexCount) {
	  bf_granny_copy_indices16(GrannyMesh, Mesh->VerIdx);
  }

  int VertexCount = GrannyMesh->vertex_count;
  int VertexBufferSize = VertexCount * sizeof(bf_granny_pnt332);

  if(GrannyMesh->rigid) {

	  Mesh->Deformer = false;
  }
  else
  {
	if(m_Scene.MaxMutableVertexBufferSize < VertexBufferSize) {
		m_Scene.MaxMutableVertexBufferSize = VertexBufferSize;
	}

	// bf_granny_deform skins position and normal into PNT332, as the
	// deformer granny2.dll made for it
	Mesh->Deformer = true;
  }
}

void C3dGrannyBoneRes :: SetProjectZ(float fClip)
{
	m_ProjectZ = fClip + fClip / 5.0f;
}

bf_granny_file* C3dGrannyBoneRes :: LoadAnimation(GrannyModel *Model, char const *BaseFileName, char const *Suffix)
{
  bf_granny_file *Animation = 0;

  char *c = (char*)strchr(BaseFileName, '.');
	if( c!=NULL ) *c = 0;

  bf_granny_file *File = LoadFile(BaseFileName, Suffix);
  if(File)
  {
	const bf_granny_info *FileInfo = bf_granny_get_info(File);
	if(FileInfo)
	{
	  if(Model->GrannyInstance && (FileInfo->animation_count == 1))
	  {
		Animation = File;
	  }
	  else
	  {
		ErrorMsg("This sample only supports single-animation files");
	  }
	}
  }

  return(Animation);
}

bf_granny_file* C3dGrannyBoneRes :: LoadFile(char const *BaseFileName, char const *Suffix)
{
  static char FileNameBuffer[512];
	CFile	fp;
  sprintf(FileNameBuffer, "%s%s.gr2", BaseFileName, Suffix);

	if (!fp.Open(FileNameBuffer, CFile::modeRead)) {
		ErrorMsg((string("Cannot find File : ") + string(FileNameBuffer)).c_str());
		return	NULL;
	}

	bf_granny_file *File = bf_granny_open((void*)fp.GetBuf (), fp.GetLength (), NULL, 0);
	fp.Close();
  return(File);
}
