#ifndef HIGHPRIEST_RESOURCE_MODEL_H
#define HIGHPRIEST_RESOURCE_MODEL_H

#include "../3dlib/Prim.h"
#include "../3dlib/Anim.h"
#include "../3dlib/mesh.h"
#include "../3dlib/Volume.h"
#include "../3dlib/Device.h"
#include "../base/File.h"
#include "../base/Res.h"
#include "../granny/granny.h"

struct GrannyMesh;
struct GrannyModel;
struct GrannyTexture;
struct HairInfo;
struct RENDER_INFO_RECT;
struct faceLight;
class C3dAnimModelRes;
struct C3dAnimNodeRes;
struct C3dFrameState;
class C3dGrannyBoneRes;
class C3dGrannyModelRes;
class C3dModelRes;
struct C3dNodeRes;
struct MaterialInfo;

struct MaterialInfo {
public:
	MaterialInfo();
	~MaterialInfo() { delete[] subMaterials; }
	char name[128];
	int numSubMtls;
	std::string* subMaterials;
};

class C3dModelRes : public CRes {
public:
	DECLARE_RESOURCE(C3dModelRes)
	C3dModelRes();
	virtual bool Load(const char* fName);
	virtual void Reset();
	C3dNodeRes* FindNode(const char* name);
	C3dNodeRes* GetFirstNode() { return *(m_objectList.begin()); }
	const char* GetMaterialName(int id, int subId) { return m_material[id].subMaterials[subId].c_str(); }
	const char* GetMaterialName(int id) { return m_material[id].name; }
	int GetSubMtlCount(int id) { return m_material[id].numSubMtls; }
	C3dMesh* GetMesh(const std::string& meshName) { return m_meshList[meshName]; }
private:
	int m_numMaterials;
	MaterialInfo* m_material;
	std::list<C3dNodeRes *> m_objectList;
	std::list<std::string> m_rootObjList;
	std::map<std::string, C3dMesh *> m_meshList;
public:
	char name[80];
	int m_shadeType;
	int m_animLen;
	std::list<CVolumeBox *> m_volumeBoxList;
	unsigned char m_alpha;
};

struct C3dNodeRes {
public:
	C3dNodeRes();
	~C3dNodeRes();
	void Release();
	char name[128];
	char parentname[128];
	C3dModelRes* scene;
	C3dNodeRes* parent;
	std::list<C3dNodeRes *> child;
	C3dMesh* mesh;
	int numTexture;
	CTexture** texture;
	float pos[3];
	float rotaxis[3];
	float rotangle;
	float scale[3];
	unsigned char alpha;
	C3dPosAnim posanim;
	C3dRotAnim rotanim;
};

class C3dAnimModelRes : public CRes {
public:
	DECLARE_RESOURCE(C3dAnimModelRes)
	C3dAnimModelRes();
	virtual ~C3dAnimModelRes();
	virtual void Reset() { Release(); }
	void Release();
	virtual bool Load(const char* fName);
	C3dAnimNodeRes* FindNode(const char* name);
	C3dAnimNodeRes* GetFirstNode() { return *(m_objectList.begin()); }
	std::string GetMaterialName(int id, int subId) { return m_material[id].subMaterials[subId]; }
	std::string GetMaterialName(int id) { return m_material[id].name; }
	int GetSubMtlCount(int id) { return m_material[id].numSubMtls; }
	int m_numMaterials;
	MaterialInfo* m_material;
	std::list<C3dAnimNodeRes *> m_objectList;
	std::list<std::string> m_rootObjList;
	std::string name;
	int m_shade;
	int m_numFrame;
	int m_tickPerFrame;
	float m_zoom;
	int m_isShaded;
	vector3d m_pivot;
	int m_alpha;
};
extern DWORD const g_GlobalVertexFormat;

struct faceLight {
	union {
		DWORD color[3];
		COLOR argb[3];
	};
};

struct C3dAnimNodeRes {
public:
	C3dAnimNodeRes();
	~C3dAnimNodeRes();
	void Release();
	void CalcFaceLight(matrix& ptm, int time, vector3d& light, vector3d& diffuseCol, vector3d& ambientCol, int shading, int alpha);
	std::string name;
	std::string parentname;
	C3dAnimModelRes* model;
	C3dAnimNodeRes* parent;
	std::list<C3dAnimNodeRes *> child;
	int m_numVertices;
	int m_numTVertices;
	int m_numFaces;
	int m_numTextures;
	faceLight* m_faceLight;
	C3dFrameState* m_frames;
	face3d* m_faces;
	CTexture** m_textures;
	int m_tvertIdentity;
};

struct _W3D_TLVERTEX {
	float sx, sy, sz, rhw;
	DWORD color, specular;
	float tu, tv;
};

struct GrannyTexture
{
	char const* Name;
	CTexture* tex;
};

struct GrannyMesh
{
	granny_mesh* Mesh;
	granny_mesh_binding* Binding;
	granny_mesh_deformer* Deformer;
	WORD* VerIdx;
	BYTE* Vertices;
	int TextureCount;
	GrannyTexture** TextureReferences;
};

struct GrannyModel
{
	granny_model_instance* GrannyInstance;
	granny_world_pose* WorldPose;
	int MeshCount;
	GrannyMesh* Meshes;
};

struct GrannyScene
{
	granny_camera Camera;
	granny_file* LoadedFile;
	int TextureCount;
	GrannyTexture* Textures;
	int ModelCount;
	GrannyModel* Model;
	int MaxBoneCount;
	granny_local_pose* SharedLocalPose;
	int MaxMutableVertexBufferSize;
	int MaxMutableVertexBufferCount;
	int MutableVertexBufferIndex;
	granny_pnt332_vertex* MatVer;
	char BaseFileName[MAX_PATH];
};

struct HairInfo {
public:
	int nPartMeshIdx;
	int nSubMeshIdx;
	matrix poseMat;
	vector3d poseLoc;
};

struct GrannyLightInfo {
public:
	vector3d eyeVec;
	int nContrast;
	int lightR;
	int lightG;
	int lightB;
};

class C3dGrannyModelRes : public CRes {
public:
	DECLARE_RESOURCE(C3dGrannyModelRes)
	C3dGrannyModelRes();
	virtual ~C3dGrannyModelRes();
	void Release();
	virtual void Reset() { Release(); }
	virtual bool Load(const char* fName);
	granny_control* ActAnimation(int idx, granny_model_instance* instance, granny_control* control, int nLoop);
	void DeActAnimation(granny_control* control);
	granny_world_pose* UpdateAni(float GameClock, granny_model_instance* instance);
	void* RenderMesh(int nMeshIdx, INT_PTR* RetTex, INT_PTR* RetFaceArr, int* nIndexNo, matrix* poseMat, granny_world_pose* pWorldpose);
	void* GetRigidMesh(int nMeshIdx);
	bool CalculateHitArea(RENDER_INFO_RECT& info, matrix& view, matrix& dispTrans, vector3d& cam);
	int GetMeshNo() { return m_Scene.Model->MeshCount; }
	granny_mesh_binding* GetMeshBinding(int idx) { return m_Scene.Model->Meshes[idx].Binding; }
	int GetMeshIndexCount(int nMeshNo);
	void SetHairInfo(HairInfo* pInfo) { m_hair = pInfo; }
	bool SetHairAndHeadMesh();
	void SetHitRad(float fx, float fy) { m_HitRadx = fx, m_HitRady = fy; }
	unsigned char IsAnimationFinished(granny_control*);
	granny_model_instance* GetModelInstance() { return GrannyInstantiateModel(m_pModel); }
	void SetMatVer(granny_pnt332_vertex* p) { m_Scene.MatVer = p; }
	void* GetWorldMatrix(matrix*, granny_world_pose*);
	void BindMeshToSkeleton(C3dGrannyModelRes* pDstRes);
	granny_skeleton* GetSkeleton() { return m_pSkeleton; }
protected:
	void GrannyError(granny_log_message_type, granny_log_message_origin, const char*, void*);
	void CreateTexture(GrannyTexture* Texture, granny_texture* GrannyTexture);
	CTexture* CreateRGBATexture(granny_texture* GrannyTexture, int g);
	void CreateModel(GrannyModel* Model, granny_model* GrannyModel);
	void CreateMesh(GrannyMesh* Mesh, granny_mesh* GrannyMesh, granny_model_instance* InModel);
	void* RenderModel(GrannyModel* Model, int nMeshIdx, INT_PTR* RetTex, INT_PTR* RetFaceArr, int* nIndexNo, matrix* poseMat, granny_world_pose* pWorldpose);
	void SetProjectZ(float fClip);
	GrannyTexture* FindTexture(granny_material* GrannyMaterial);
	granny_file* LoadFile(const char* BaseFileName, const char* Suffix);
protected:
	float m_ProjectZ;
	GrannyScene m_Scene;
	granny_model* m_pModel;
	vector3d m_HitareaPos;
	float m_HitRadx;
	float m_HitRady;
	C3dGrannyBoneRes* m_pBoneRes;
	int m_nBoneType;
private:
	HairInfo* m_hair;
	granny_skeleton* m_pSkeleton;
};

struct C3dFrameState {
public:
	C3dFrameState();
	~C3dFrameState();
	void CreateVertex(int numVertex);
	void CreateTVertex(int numVertex);
	vector3d* m_vertices;
	tvertex3d* m_tvertices;
};

class C3dGrannyBoneRes {
public:
	C3dGrannyBoneRes();
	~C3dGrannyBoneRes();
	void Init();
	void FreeModel();
	void Release();
	void Reset();
	bool AddBone(const char* fName, int nType, int nAniIdx);
	granny_animation* GetAnimation(int nType, int nAniIdx);
	void UpdateAni(float fClip);
	int GetMeshNo();
protected:
	void GrannyError(granny_log_message_type, granny_log_message_origin, const char*, void*);
	void CreateModel(GrannyModel* Model, granny_model* GrannyModel);
	void CreateMesh(GrannyMesh* Mesh, granny_mesh* GrannyMesh, granny_model_instance* InModel);
	void SetProjectZ(float fClip);
	granny_animation* LoadAnimation(GrannyModel* Model, const char* BaseFileName, const char* Suffix);
	granny_file* LoadFile(const char* BaseFileName, const char* Suffix);
protected:
	float m_ProjectZ;
	GrannyScene m_Scene;
	granny_animation* m_Animation[100][20];
};
extern C3dGrannyBoneRes g_GrannyBoneRes;

#endif
