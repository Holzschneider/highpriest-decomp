#ifndef HIGHPRIEST_GAMEACTOR3D_H
#define HIGHPRIEST_GAMEACTOR3D_H

#include "GameActor.h"
#include "resource/Model.h"

struct granny_control;
struct granny_model_instance;
struct granny_world_pose;
class C3dGrannyGameActor;
class CGrannyCorpse3d;

class C3dGrannyGameActor : public CGameActor {
public:
	C3dGrannyGameActor(char* strJobFn);
	virtual ~C3dGrannyGameActor();
	virtual bool OnProcess();
	virtual void Render(matrix& view);
	virtual void SetAction(int action, int speed, int type);
	virtual void SetSprAct(int job, int sex);
	virtual void ProcessMotion();
	virtual void ProcessMotionWithDist();
	virtual void MakeCorpse();
	virtual int GetAttackMotion();
	void SetRenderType(int);
	int GetRenderType();
protected:
	CTexture* GetEmblemTexture();
	bool ProcessAni();
	void RenderNormal(matrix& view, GrannyLightInfo* grannyLInfo);
	void RenderBak(matrix& view);
	void RenderNormalFast(matrix& view);
	void RenderCell(matrix& view);
	void RenderGuildMark(matrix& view);
	void RenderEffect(int nMesh, granny_pnt332_vertex* vertArr, vector3d tv);
	void RenderShadow(matrix& vtm, vector3d lv, bool isBakRender);
protected:
	CTexture* m_emblemTex;
	C3dGrannyModelRes* m_GrannyActorRes;
	granny_model_instance* m_Instance;
	RPMesh* m_rp[30];
	granny_pnt332_vertex* m_matVer[30];
	granny_system_clock m_fAniCnt;
	granny_system_clock m_fLastAniCnt;
	float m_GameClock;
	int m_curAction;
	int m_baseAction;
	float m_fCurRot;
	unsigned char m_RenderAlpha;
	unsigned long m_nVertCol;
	int m_nLastActAnimation;
	int m_curFrame;
	char m_strJobFn[260];
	int m_nRenderType;
	CTexture* m_pCellTex;
	granny_control* m_Control[20];
	unsigned char m_isFirstProcess;
	CTexture* m_pTex[30];
	void* m_pFaceArr[30];
	int m_nIndexNo[30];
	matrix m_matPose[30];
private:
	granny_world_pose* m_pWorldPose;
	CTexture* m_shadowTex;
	std::vector<vector3d> m_shadowDotList;
};

class CGrannyCorpse3d : public CGameObject {
public:
	CGrannyCorpse3d(char* strJobFn);
	virtual ~CGrannyCorpse3d();
	void SetPos(vector3d pos) { m_pos = pos; }
	void SetRot(float fRot) { m_fCurRot = fRot; }
	void OnInit();
	void OnExit();
	virtual bool OnProcess();
	virtual void Render(matrix& view);
	bool ProcessAni();
protected:
	vector3d m_pos;
	C3dGrannyModelRes* m_GrannyActorRes;
	granny_model_instance* m_Instance;
	granny_control* m_Control;
	RPMesh* m_rp[30];
	granny_pnt332_vertex* m_matVer[30];
	granny_system_clock m_fAniCnt;
	granny_system_clock m_fLastAniCnt;
	float m_GameClock;
	int m_nStandAngle;
	int m_nStateCnt;
	float m_fCurRot;
	unsigned char m_RenderAlpha;
	unsigned long m_nVertCol;
	char m_strJobFn[260];
};

#endif
