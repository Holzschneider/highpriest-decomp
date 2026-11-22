#ifndef HIGHPRIEST_GAMEWORLD_H
#define HIGHPRIEST_GAMEWORLD_H

#include "3dlib/Prim.h"
#include "3dGround.h"

class C3dActor;
class CGameActor;
class CGameObject;
class CItem;
class CMode;
class CPlayer;
class CSkill;
class CRayPicker;
class CWorld;
struct SceneGraphNode;

struct SceneGraphNode {
public:
	SceneGraphNode();
	~SceneGraphNode();
	void Build(int level);
	void InsertObject(C3dAttr* actor, int level);
	void InsertObject(C3dGround* actor, int level);
	void InsertObject(C3dActor* actor, int level);
	void RemoveObject(C3dActor* actor, int level);
	void UpdateVolume(int level);
	void UpdateVolumeAfter(int level);
	std::vector<C3dActor *>* GetActorList(float x, float z, int level);
	void InsertObjectAfter(C3dActor* actor, int level);
	void CopySceneGraph(int Level, SceneGraphNode* graph);
	void LoadSceneGraph(CFile* fp, int Level);
	SceneGraphNode* m_parent;
	SceneGraphNode* m_child[4];
	C3dAABB m_aabb;
	vector3d m_center;
	vector3d m_halfSize;
	int m_needUpdate;
	std::vector<C3dActor *> m_actorList;
	C3dGround* m_ground;
	tagRECT m_groundArea;
	C3dAttr* m_attr;
	tagRECT m_attrArea;
};

class CWorld {
public:
	CWorld();
	virtual ~CWorld();
	void OnEnterFrame();
	void MakeFixedActor();
	void OnExitFrame();
	void ProcessActors();
	void RemoveGameObject(CGameObject* object);
	void InitPlayer();
	int CheckAttr(vector3d pos);
	int CheckGround(vector3d pos);
	void AdjustPos(vector3d& pos);
	void AdjustPos2(vector3d& pos);
	float CalcShadowHeight(vector3d& pos);
	SceneGraphNode* GetRootNode() { return &m_rootNode; }
	void GetServerCoor(float cx, float cz, int& sx, int& sy);
	void GetClientCoor(float sx, float sy, float& cx, float& cz);
	void GetClientCoor(int sx, int sy, float& cx, float& cz);
	void SetCellInfo(int, int, int);
	void Add3DActorToWorld(C3dActor* actor);
	void WaitForThreadEnd();
	void SetPKZone(int isPKZone) { m_isPKZone = isPKZone; }
	void SetSiegeMode(int isSiegeMode) { m_isSiegeMode = isSiegeMode; }
	void SetBattleFieldMode(int isBattleFieldMode) { m_isBattleFieldMode = isBattleFieldMode; }
	void SetEventPVPMode(int isEventPVPMode) { m_isEventPVPMode = isEventPVPMode; }
	int IsPKZone() { return m_isPKZone; }
	int IsSiegeMode() { return m_isSiegeMode; }
	int IsBattleFieldMode() { return m_isBattleFieldMode; }
	int IsEventPVPMode() { return m_isEventPVPMode; }
	CGameActor* GetGameActorByAID(unsigned long GID);
	CItem* GetItemByAID(unsigned long AID);
	CSkill* GetSkillByAID(unsigned long AID);
	bool RemoveSkill(unsigned long gameActor);
	bool RemoveGameActor(CGameObject* gameActor);
	bool RemoveGameActor(unsigned long gameActor);
	bool RemoveItem(unsigned long gameActor);
	void RemoveGameActorAll();
	CSkill* MakeSkill(int val_JT, int val_X, int val_Y);
	bool RemoveSkillbyJT(int val_JT);
private:
	void BuildSceneGraph();
	void FreeSceneGraph();
public:
	CMode* m_curMode;
	std::list<CGameObject *> m_gameObjectList;
	std::list<CGameActor *> m_actorList;
	std::list<CItem *> m_itemList;
	std::list<CSkill *> m_skillList;
	C3dGround* m_ground;
	CPlayer* m_player;
	C3dAttr* m_attr;
	std::vector<C3dActor *> m_bgObjList;
	long m_bgObjCount;
	long m_bgObjThread;
private:
	friend class CView;
	int m_isPKZone;
	int m_isSiegeMode;
	int m_isBattleFieldMode;
	int m_isEventPVPMode;
	SceneGraphNode m_rootNode;
	SceneGraphNode* m_Calculated;
};

class CRayPicker {
public:
	CRayPicker();
	void Build(const lineSegment3d& ray, SceneGraphNode* rootNode);
	void Build(const ray3d& ray, SceneGraphNode* rootNode);
	void Free();
	int CheckAABBIntersect(const C3dAABB& node) const;
private:
	int CheckAABBIntersect(SceneGraphNode* node);
	void CullSceneNode(SceneGraphNode* node, int level);
public:
	std::list<SceneGraphNode *> m_cubeletList;
private:
	lineSegment3d m_ray;
};

#endif
