#ifndef HIGHPRIEST_3DACTOR_H
#define HIGHPRIEST_3DACTOR_H

#include "3dlib/mesh.h"
#include "3dlib/Anim.h"
#include "3dlib/Volume.h"
#include "3dlib/Renderer.h"
#include "resource/Model.h"
#include "resource/World.h"

const int MAX_NODENAME = 128;

class C3dActor;
class C3dNode;

class C3dActor {
public:
	C3dActor();
	~C3dActor();
	void OnInit() {}
	void Reset();
	void SetFrame(int framepos);
	void SetOpacity(float opacity);
	void SetRot(const vector3d& rot) { m_rot = rot; m_isMatrixNeedUpdate = 1; }
	void SetPos(const vector3d& p) { m_pos = p; m_isMatrixNeedUpdate = 1; }
	void SetScale(const vector3d& scale) { m_scale = scale; m_isMatrixNeedUpdate = 1; }
	void SetAnimType(int framepos);
	vector3d GetPos() { return m_pos; }
	void AssignModel(C3dModelRes* model);
	void OnProcess();
	void Render(matrix& ctm, bool clip, int flagRF);
	void UpdateVertexColor(const vector3d& lightDir, const vector3d& diffuseCol, const vector3d& ambientCol);
	C3dNode* FindNode(const char*);
	void SetVolumeBoxInfo(std::list<CVolumeBox *>* volBoxList);
	int CheckCollision(const vector3d& wpos);
	void AdjustCollision(vector3d& wpos);
	void CalcShadowHeight(const vector3d& wpos, float& baseHeight);
	void CullByAABB(const CRayPicker& rayPicker);
	void CullByOBB(const lineSegment3d& ray);
	char* GetName();
	float GetAnimSpeed();
	int GetIsHideCheck();
	int GetBlockType();
	void SetAnimSpeed(float Speed) { if (Speed != 0) Speed /= 3; m_animSpeed = Speed; }
	void SetName(char*);
	void SetIsHideCheck(int framepos);
	void SetBlockType(int framepos);
	void HalfAlphaOff();
	void HalfAlphaOn();
	unsigned char GetFadeAlphaCnt();
	unsigned char IsHalfAlpha();
	void ProcessFadeAlpha();
private:
	void UpdateMatrix();
	void UpdateBound();
private:
	friend class CWorld;
	friend class CView;
	friend class C3dNode;
	friend struct SceneGraphNode;
	C3dNode* m_node;
	char m_name[128];
	vector3d m_pos;
	vector3d m_rot;
	vector3d m_scale;
	int m_shadeType;
	int m_curMotion;
	int m_animType;
	int m_animLen;
	float m_animSpeed;
	matrix m_wtm;
	matrix m_iwtm;
	vector3d m_posOffset;
	int m_isMatrixNeedUpdate;
	int m_isBbNeedUpdate;
	C3dOBB m_oBoundingBox;
	C3dAABB m_aaBoundingBox;
	int m_renderSignature;
	int m_isHideCheck;
	unsigned char m_isHalfAlpha;
	unsigned char m_fadeAlphaCnt;
	std::list<CVolumeBox *> m_volumeBoxList;
	int m_blockType;
};

inline void C3dActor::HalfAlphaOff()
{
	m_isHalfAlpha = false;
}

inline void C3dActor::HalfAlphaOn()
{
	m_isHalfAlpha = true;
}

inline unsigned char C3dActor::GetFadeAlphaCnt()
{
	return m_fadeAlphaCnt;
}

inline unsigned char C3dActor::IsHalfAlpha()
{
	return m_isHalfAlpha;
}

inline void C3dActor::ProcessFadeAlpha()
{
	if (m_isHalfAlpha == true) {
		if (m_fadeAlphaCnt > 90)
			m_fadeAlphaCnt -= 5;
	} else {
		if (m_fadeAlphaCnt < 255)
			m_fadeAlphaCnt += 5;
	}
}

class C3dNode {
public:
	struct ColorInfo {
		union {
			unsigned int color[3];
			COLOR argb[3];
		};
		vector2d uv[3];
	};

public:
	virtual void AssignModel(C3dNodeRes* modelNode);
	virtual void UpdateVertexColor(const matrix& ptm, const vector3d& light, const vector3d& diffuseCol, const vector3d& ambientCol, int shadeType);
	virtual void Render(const matrix& ptm, int fadeAlphaCnt, bool flipNormal);
protected:
	C3dNode();
	virtual ~C3dNode();
	void Reset();
	void SetFrame(int framepos);
	void CheckAABB(const CRayPicker& rayPicker);
	void UpdateAABB(const matrix& ptm, const matrix& wtm);
	void UpdateBound(C3dAABB* box, const matrix& ptm);
	C3dNode* FindNode(const char* name);
	void UpdateDestVertColor(CRenderer* renderer, const matrix& nodeTM, C3dGround15* pGround);
	void SetToAlpha();

	friend class C3dActor;
protected:
	C3dActor* m_master;
	C3dMesh* m_mesh;
	char m_name[128];
	int m_numTexture;
	std::vector<CTexture *> m_texture;
	float m_opacity;
	C3dNode* m_parent;
	std::list<C3dNode *> m_child;
	matrix m_ltm;
	C3dPosAnim m_posAnim;
	C3dRotAnim m_rotAnim;
	C3dScaleAnim m_scaleAnim;
	std::vector<ColorInfo> m_colorInfo;
	std::vector<int> m_destVertCol;
	int m_isAlphaForPlayer;
	C3dAABB m_aabb;
};

inline char* C3dActor::GetName()
{
	return m_name;
}

inline float C3dActor::GetAnimSpeed()
{
	return m_animSpeed;
}

inline int C3dActor::GetBlockType()
{
	return m_blockType;
}

inline int C3dActor::GetIsHideCheck()
{
	return m_isHideCheck;
}

inline void C3dActor::SetName(char* Name)
{
	strcpy(m_name, Name);
}

inline C3dNode* C3dActor::FindNode(const char* name)
{
	if (!m_node)
	{
		return NULL;
	}
	return m_node->FindNode(name);
}

inline void C3dActor::SetIsHideCheck(int Check)
{
	m_isHideCheck = Check;
}

inline void C3dActor::SetBlockType(int Type)
{
	m_blockType = Type;
}

inline void C3dActor::SetAnimType(int animType)
{
	m_animType = animType;
}

#endif
