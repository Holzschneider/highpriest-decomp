#ifndef HIGHPRIEST_VIEW_H
#define HIGHPRIEST_VIEW_H

#include "resource/Skybox.h"

class CView;
class CViewFrustum;

struct ViewInfo3d {
public:
	vector3d at;
	float latitude;
	float longitude;
	float distance;
};

class CViewFrustum {
public:
	CViewFrustum();
	void Free();
	void Build(float hratio, float vratio, const matrix& viewMatrix, SceneGraphNode* rootNode);
private:
	void CullSceneNode(SceneGraphNode* node, int level, bool isTotallyInside);
	int CheckAABBIntersect(SceneGraphNode* node);
	int CheckOBBIntersect(const C3dOBB& bb);
private:
	std::list<SceneGraphNode *> m_cubeletListTotal;
	std::list<SceneGraphNode *> m_cubeletListPartial;
	plane3d m_planes[6];
	vector3d m_planeNormals[6];
};

class CView {
public:
	CView();
	virtual ~CView();
	virtual void OnEnterFrame();
	virtual void OnExitFrame();
	virtual void OnRender();
	virtual void OnCalcViewInfo();
	float GetCurLongitude() { return m_cur.longitude; }
	float GetCurLatitude() { return m_cur.latitude; }
	float GetCurDistance() { return m_cur.distance; }
	float GetDestLongitude() { return m_dest.longitude; }
	float GetDestLatitude() { return m_dest.latitude; }
	float GetDestDistance() { return m_dest.distance; }
	vector3d GetCurAt() { return m_cur.at; }
	vector3d GetFrom() { return m_from; }
	matrix GetViewMatrix() { return m_viewMatrix; }
	void SetDestLongitude(float longitude) { m_dest.longitude = longitude; }
	void SetDestDistance(float dist) { m_dest.distance = dist; }
	void SetDestLatitude(float lati) { m_dest.latitude = lati; }
	void SetDestAt(float x, float y, float z) { m_dest.at.Set(x, y, z); }
	void SetCurLongitude(float longitude) { m_cur.longitude = longitude; }
	void SetCurDistance(float distance) { m_cur.distance = distance; }
	void SetCurLatitude(float latitude) { m_cur.latitude = latitude; }
	void SetCurAt(float x, float y, float z) { m_cur.at.Set(x, y, z); }
	void AdjustDestLongitude(float longitude) { m_dest.longitude -= longitude; }
	void HoldAt() { m_cur.at = m_dest.at; }
	void ResetLongitude(float longitude) { m_cur.longitude = m_dest.longitude = longitude; }
	void ResetLatitude(float latitude) { m_cur.latitude = m_dest.latitude = latitude; }
	void ResetDistance(float dist) { m_cur.distance = m_dest.distance = dist; }
	void ResetAt(float x, float y, float z) { m_cur.at.Set(x, y, z); m_dest.at.Set(x, y, z); }
	void PushCamera() { m_backupCur = m_cur; m_backupDest = m_dest; }
	void PopCamera() { m_cur = m_backupCur; m_dest = m_backupDest; }
	void AddLongitude(float deltaLongitude);
	void AddLatitude(float deltaLongitude);
	void AddDistance(float deltaLongitude);
	void SetQuake(int isQuake, int Type, float sideQuake, float frontQuake, float latitudeQuake);
	void SetQuakeInfo(float sideQuake, float frontQuake, float latitudeQuake);
	void GetEeyeVector(vector3d* eyeVector);
	vector3d* GetEeyeFromVector() { return &m_from; }
	unsigned char IsFPSmode() { return m_isFPSmode; }
protected:
	void InterpolateViewInfo();
	void ProcessQuake();
	void BuildViewMatrix();
protected:
	friend class CGameMode;
	float m_sideQuake;
	float m_frontQuake;
	float m_latitudeQuake;
	unsigned char m_isFPSmode;
	int m_isQuake;
	unsigned int m_quakeStartTick;
	unsigned int m_QuakeTime;
	ViewInfo3d m_cur;
	ViewInfo3d m_dest;
	ViewInfo3d m_backupCur;
	ViewInfo3d m_backupDest;
	vector3d m_from;
	vector3d m_up;
	matrix m_viewMatrix;
	matrix m_invViewMatrix;
	CViewFrustum m_viewFrustum;
	CWorld* m_world;
	CSkyBoxEllipse* m_skyBox;
};

#endif
