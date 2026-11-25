#ifndef HIGHPRIEST_GAMEMODE_H
#define HIGHPRIEST_GAMEMODE_H


#include "Mode.h"
#include "3dlib/Prim.h"
#include "base/Util.h"

class CGameObject;
class CDummyEffectLauncher;

struct CONTACTNPC {
	DWORD nid;
	int job;
	int type;
};

struct CellPos {
	int x;
	int y;
};

struct ColorCellPos {
	int x;
	int y;
	DWORD color;
};

struct ColorCellPos2 {
	int x;
	int y;
	int type;
	DWORD color;
	DWORD startTime;
};

struct DRAG_INFO {
public:
	ENUM_DRAGTYPE m_dragType;
	int m_dragItemIndex;
	int m_dragItemPrice;
	int m_dragItemRealPrice;
	int m_numDragItem;
	int m_slotNum;
	int m_dragItemType;
	int m_refiningLevel;
	unsigned char m_isIdentified;
	std::string m_dragSprName;
	std::string m_dragItemName;
	std::string m_skillName;
	int m_skillUseLevel;
	int m_slot[4];
};

enum ENUM_SKILL_USE_TYPE {
	SUT_NOSKILL, SUT_TO_GROUND, SUT_TO_CHARACTER, SUT_TO_ITEM, SUT_TO_ALL, SUT_TO_SKILL, SUT_TO_SKILLGROUND_WITHTALKBOX
};

struct SKILL_USE_INFO {
	ENUM_SKILL_USE_TYPE m_skillUseType;
	int m_skillId;
	int m_attackRange;
	int m_useLevel;
};

struct SHOW_IMAGE_INFO {
public:
	int type;
	std::string imageName;
};

struct PLAY_WAVE_INFO {
public:
	std::string wavName;
	unsigned int nAID;
	unsigned int term;
	unsigned int endTick;
};

struct NamePair {
	std::string cName;
	std::string pName;
	std::string gName;
	std::string rName;
	void Clear() { cName.resize(0); pName.resize(0); gName.resize(0); rName.resize(0); }
	unsigned char IsEmpty() const { return cName.size() ? false : true; }
};

struct SprActName {
	std::string sprName;
	std::string actName;
};


class CActorPickInfo;
class CGameActor;
const int	AVG_ATTACKED_SPEED = 288;
class CMousePointer;
class CMsgEffect;
class CRenderObject;
class CView;
class CWorld;


enum MSGEFFECTTYPE {
	MET_JUST_ANIMATE, MET_EMOTION, MET_MVP, MET_CART, MET_PKEFFECT, MET_ICEEFFECT,
	MET_ICEBREAKEFFECT, MET_CURSE_EFFECT, MET_SLEEP_EFFECT, MET_STUN_EFFECT,
	MET_SILENCE_EFFECT, MET_ANGELUS_EFFECT, MET_CRUSIS_EFFECT, MET_MISS,
	MET_NUMBER, MET_CRITICAL_EFFECT, MET_NUMBER_CRITICAL, MET_LUCKY_EFFECT,
	MET_BIRD, MET_BIRDEFFECT,
	MET_BIRD_ATTACK, MET_NUMBER_HALF, MET_NUMBER_TOTAL, MET_NUMBER_RANK,
	MET_GUARD, MET_BLUE_EFFECT, MET_CURSOR_EFFECT,
	MET_MOVETO_EFFECT, MET_BOTPOINT_EFFECT,
	MET_NUMBER_TOTAL_CRITICAL
};

enum MAKE_MSG_EFFECT_TYPE {
	MAKETYPE_MVP_EFFECT,
	MAKETYPE_CART_EFFECT,
	MAKETYPE_PK_EFFECT,
	MAKETYPE_ICE_EFFECT,
	MAKETYPE_ICEBREAK_EFFECT,
	MAKETYPE_SMOKE_EFFECT,
	MAKETYPE_CURSE_EFFECT,
	MAKETYPE_SLEEP_EFFECT,
	MAKETYPE_STUN_EFFECT,
	MAKETYPE_SILENCE_EFFECT,
	MAKETYPE_ANGELUS_EFFECT,
	MAKETYPE_CRUSIS_EFFECT,
	MAKETYPE_CRITICAL_EFFECT,
	MAKETYPE_LUCKY_EFFECT,
	MAKETYPE_BIRD_EFFECT,
	MAKETYPE_RANK_EFFECT,
	MAKETYPE_CART_EFFECT2,
	MAKETYPE_CART_EFFECT3,
	MAKETYPE_CART_EFFECT4,
	MAKETYPE_CART_EFFECT5,
	MAKETYPE_GUARD_EFFECT,
	MAKETYPE_CART_EFFECT6,
	MAKETYPE_BLUE_EFFECT,
	MAKETYPE_BIRD_EFFECT2,
	MAKETYPE_BIRD_EFFECT3,
	MAKETYPE_CART_EFFECT11,
	MAKETYPE_CART_EFFECT12,
	MAKETYPE_CART_EFFECT13,
	MAKETYPE_CART_EFFECT14,
	MAKETYPE_CART_EFFECT15,
	MAKETYPE_CART_EFFECT16,
	MAKETYPE_CURSOR_EFFECT,
	MAKETYPE_BIRD_EFFECT4,
	MAKETYPE_BIRD_EFFECT5,
	MAKETYPE_BIRD_EFFECT6
};

class CGameMode : public CMode {
	friend class CWorld;
#if defined(BF_PORT)
	friend struct BfClientGame;   // port/client/src/bf_client_game.cpp reads the actor names, without asking the server
#endif
	static unsigned int m_lastLockOnPcGid;
	static unsigned int m_dwOldAutoFollowTime;
	static unsigned int m_dwOldDisappearTime;
	virtual void OnChangeState(int time);
	void ProcessNeverDie();
	void PollNetworkStatus();
	void OnProcessState(int time);
	void DeleteCompassPos(int time);
	void MakeNameBalloonTextWindow(UIWindow** window);
	void MoveBalloonTextWindow(UIWindow* window, CRenderObject* renderObject);
	void ProcessAutoFollow();
	void MonsterTalk(CGameActor* theActor, int job, int stateId, int skillId, unsigned int gid);
	void DrawMinimapTriangle(float fX, float fY, float trisize, unsigned int argb);
	void SetMemberVisibleState(unsigned int timeTick, int showType);
	int IsExceptionState(int processType);
	void DrawBossMonsterOnMiniMap(const UVRECT& uvRect);
public:
	CGameMode();
	virtual void OnInit(const char* modeName);
	virtual int OnRun();
	virtual void OnExit();
	virtual void OnUpdate();

	virtual INT_PTR SendMsg(int messageId, INT_PTR val1 = 0, INT_PTR p = 0, INT_PTR st = 0);
	CWorld* GetWorld() { return m_world; }
	CView* GetView() { return m_view; }
	void ChatDori();
	void Initialize();
	void MakeFog(int time);
	void PlayBGM();
	void SetCheckGndAlpha();
	void SetCamera();
	void SaveCamera();
	void PushGameObject(CGameObject* gameObject);
	void RemoveGameActorAllAndMake();
	void MakeMousePointer(vector2d&);
	void MakeMenu(CActorPickInfo* actorInfo, int processType);
	void ProcessMouseSnap(CActorPickInfo* actorInfo, int processType);
	void ProcessWheel(int time);
	void ProcessRBtn();
	int ProcessGroundPick(int processType);
	void ProceedAttack(unsigned int timeTick, int showType);
	bool AttackSkillOverWeightMessage();
	void UnloadRarelyUsedCaches();
	void ProcessReqEmblem();
	void ProcessPeriodicalJobs();
	void ProcessNameWaitingTimer();
	void GetServerCoor(float cx, float cz, int& sx, int& sy, int& cellX, int& cellY);
	void GetServerCoor(float cx, float cz, int& sx, int& sy);
	void GetClientCoor(int, int, int, int, float&, float&);
	void GetClientCoor(int sx, int sy, float& cx, float& cz);
	void ConvertToNumCellClose(int& sx, int& sy, int& dx, int& dy, int numCell);
	void ConvertToNoRedCell(int& sx, int& sy, int& dx, int& dy);
	void ConvertToNumCellCloseAndFindNoRedCell(float x1, float y1, float x2, float y2, int& dx, int& dy, int numCell);
	bool IsInsideCell(int& sx, int& sy, int& dx, int& dy, int numCell);
	bool IsRedCell(int x, int y);
	int GetCellFlag(int x, int y);
	bool IsValidCell(int x, int y);
	bool CanFindPath(int sx, int sy, int cellX, int cellY, int dx, int dy);
	bool GetClosestValidCell(int& dx, int& dy);
	bool IsRegisterCell(int x, int y);
	void MakeGroundMousePointer();
	void ShowMousePointer(float cx, float cy, int color);
	void HideMousePointer(int time);
	void SynchronizeTimeWithServer(bool syncNow);
	void ProcessInput();
	bool GetGroundAttrPos(int& dx, int& dy);
	void RecalcAveragePingTime(unsigned int itemNaid);
	int ProcessObjClick(CActorPickInfo* actorInfo, int processType);
	const NamePair& GetNullNamePair() { static NamePair nullnamepair; return nullnamepair; }
	unsigned int GetActorNameRequestTimer(unsigned int aid);
	void RemoveActorNameRequestTimer(unsigned int itemNaid);
	const NamePair& GetActorName(unsigned int aid);
	void RequestActorName(unsigned int itemNaid);
	unsigned int GetActorNameByGIDRequestTimer(unsigned int aid);
	void RemoveActorNameByGIDRequestTimer(unsigned int itemNaid);
	const NamePair& GetActorNameByGID(unsigned int aid);
	void RequestActorNameByGID(unsigned int itemNaid);
	void CheckGuildMemberStatusCache(unsigned int gid, const NamePair& actorName);
	unsigned int GetActorNameColor(int job, unsigned int aid);
	void MakeNameBalloon(unsigned int itemNaid);
	void MakeActorNameBalloon(CGameActor* theActor);
	void MakeAttackedActorNameBalloon(CGameActor* theActor);
	void ProcessNameBalloon();
	CGameActor* MakeGameActor(int job);
	CGameActor* NewNpc(int job);
	void PlaceEffect(vector3d* pos, int effectId, int lastTime, int F1 = 0);
	void MakeBlowEffect(float cx, float cy, float cz);
	void MakeArrowEffect(vector3d& sPos, unsigned int targetGid, float hideTime, int job);
	void MakeArrowEffectPos(vector3d& sPos, const vector3d& tPos, float hideTime);
	void MakeMandraAttackEffect(unsigned int targetGid, int job, float hideTime);
	void MakeCorpse(float cx, float cy, float cz, const char* actName, const char* sprName, int action, int motion, float roty);
	void MakeCorpse3d(float cx, float cy, float cz, const char* rmaName, int curFrame, float roty, vector3d pivot, float modelZoom);
	void MakeGrannyCorpse3d(float cx, float cy, float cz, float fRot, const char* Gr2Name);
	void MakePotionEffect(int itemId, unsigned int aid);
	void MakeNumEffect(vector3d& sPos, int number, unsigned int sprArgb, unsigned int masterGid);
	CMsgEffect* MakeMissEffect(vector3d& sPos, unsigned int sprArgb, unsigned int masterGid);
	CMsgEffect* MakeEmotion(vector3d& sPos, int emotionType, unsigned int masterGid);
	CMsgEffect* MakeMsgEffect(MAKE_MSG_EFFECT_TYPE EffectMakeType, vector3d& sPos, unsigned int masterGid);
	void MakeEmblemWnd(CGameActor* theActor, int gdid, int emblemVer);
	void MakeWordDisplayWnd(CGameActor* theActor);
	void InvalidatePlayerGage();
	void HideCheck();
	void DrawCompass();
	void DrawMiniMap();
	void DrawActorsOnMiniMap(const UVRECT& uvRect);
	void DrawCompassOnMiniMap(const UVRECT& uvRect);
	void DrawPKRanking();
	void DrawPKZoneTime();
	void DrawTime(unsigned int timeTick, int showType);
	void DrawTimeAttack();
	void ShowTime();
	void AutoStatusUp();
	void CheckFrame();
	void DrawArenaRemainTime();
	void DrawDragImage();
	void DrawIllustImage();
	void DrawUseSkillLevelNumber();
	void RotatePoint(float cx, float cy, float& x, float& y, int angle);
	const char* GetDragItemName();
	DRAG_INFO* GetDragInfo() { return &m_dragInfo; }
	ENUM_DRAGTYPE GetDragType() { return m_dragInfo.m_dragType; }
	int CanRotateView() { return m_canRotateView; }
	int GetNumDragItem() { return m_dragInfo.m_numDragItem; }
	int GetDragItemIndex() { return m_dragInfo.m_dragItemIndex; }
	ChatRoomInfo* GetLastChatroomInfo() { return &m_lastChatroomInfo; }
	unsigned int GetAid(const char* cName);
	void ProcessTalkType(int val1, INT_PTR val2);
	void ProcessLockOnMouse();
	void ProcessAssignAID();
	int ProcessActorPick(int processType, CActorPickInfo* actorInfo);
	void ProcessPcPick(int& processType, CActorPickInfo* actorInfo, CGameActor* pc);
	void ProcessMonsterPick(int& processType, CActorPickInfo* actorInfo, CGameActor* pc);
	void WriteNameBalloonPosToReg();
	void InitNameBalloonPosFromReg();
	bool IsReqPickupItemNaid(unsigned int itemNaid);
	void RemovePickupItemNaid(unsigned int itemNaid);
	void AddPickupItemNaid(unsigned int itemNaid);
	void ProcessPlayWave();
	void ResetDrag();
	void PrintDebugString();
	int GetSkillAttackRange() { return m_skillUseInfo.m_attackRange; }
	int IsPlayerDead() { return m_isPlayerDead; }
	void SetWaitUseItemAck(int isAck) { m_waitingUseItemAck = isAck; }
	void WriteRagPathToReg();
	bool IsKillerAid(unsigned int itemNaid);
	bool HaveSiegfriedItem();
	bool HaveBookItem(int itemId);
	void ProceedSkillToFriendList(unsigned int itemNaid);
	void InsertScheduler();
	const char* SetPaddingValue(char* pad, int len);
	void Zc_Notify_Standentry(const char* modeName);
	void Zc_Notify_Newentry(const char* modeName);
	void Zc_Refuse_Quit(const char* modeName);
	void Zc_Notify_Move(const char* modeName);
	void Zc_Notify_Playermove(const char* modeName);
	void Zc_Notify_Act(unsigned int GID, unsigned int targetGID, unsigned int startTime, int attackMT, int attackedMT, int damage, short count, unsigned char action, int leftDamage);
	void Zc_Notify_Playerchat(const char* modeName);
	void Zc_Notify_Chat(const char* modeName);
	void Zc_Notify_Time(const char* modeName);
	void Zc_Npcack_Mapmove(const char* modeName);
	void Zc_Npcack_Enable(const char* modeName);
	void Zc_Npcack_Servermove(const char* modeName);
	void Zc_Accept_Enter(const char* modeName);
	void Zc_Accept_Enter2(const char* modeName);
	void Zc_Ack_Reqname(const char* modeName);
	void Zc_Broadcast(const char* modeName);
	void Zc_Whisper(const char* modeName);
	void Zc_Ack_Whisper(const char* modeName);
	void Zc_Change_Direction(const char* modeName);
	void Zc_Ack_Disconnect_Character(const char* modeName);
	void Zc_Item_Entry(const char* modeName);
	void Zc_Item_Pickup_Ack(const char* modeName);
	void Zc_Item_Disappear(const char* modeName);
	void Zc_Notify_Actentry(const char* modeName);
	void Zc_Notify_Moveentry(const char* modeName);
	void Zc_Notify_Vanish(const char* modeName);
	void Sc_Notify_Ban(const char* modeName);
	void Zc_Accept_Quit(const char* modeName);
	void Zc_Normal_Itemlist(const char* modeName);
	void Zc_Normal_Itemlist2(const char* modeName);
	void Zc_Normal_Itemlist3(const char* modeName);
	void Zc_Equipment_Itemlist(const char* modeName);
	void Zc_Item_Fall_Entry(const char* modeName);
	void Zc_Stopmove(const char* modeName);
	void Zc_HighJump(const char* modeName);
	void Zc_Par_Change(const char* modeName);
	void Zc_Longpar_Change(const char* modeName);
	void Zc_Use_Item_Ack(const char* modeName);
	void Zc_Req_Wear_Equip_Ack(const char* modeName);
	void Zc_Req_Takeoff_Equip_Ack(const char* modeName);
	void Zc_Item_Throw_Ack(const char* modeName);
	void Zc_Notify_Standentry_Npc(const char* modeName);
	void Zc_Say_Dialog(const char* modeName);
	void Zc_Wait_Dialog(const char* modeName);
	void Zc_Close_Dialog(const char* modeName);
	void Zc_Menu_List(const char* modeName);
	void Zc_Restart_Ack(const char* modeName);
	void Zc_Status_Change_Ack(const char* modeName);
	void Zc_Status(const char* modeName);
	void Zc_Status_Change(const char* modeName);
	void Zc_Emotion(const char* modeName);
	void Zc_User_Count(const char* modeName);
	void Zc_Sprite_Change(const char* modeName);
	void Zc_Select_Dealtype(const char* modeName);
	void Zc_Pc_Purchase_Itemlist(const char* modeName);
	void Zc_Pc_Sell_Itemlist(const char* modeName);
	void Zc_Pc_Purchase_Result(const char* modeName);
	void Zc_Pc_Sell_Result(const char* modeName);
	void Zc_Setting_Whisper_Pc(const char* modeName);
	void Zc_Setting_Whisper_State(const char* modeName);
	void Zc_Whisper_List(const char* modeName);
	void Zc_Ack_Create_Chatroom(const char* modeName);
	void Zc_Room_Newentry(const char* modeName);
	void Zc_Destroy_Room(const char* modeName);
	void Zc_Enter_Room(const char* modeName);
	void Zc_Member_Newentry(const char* modeName);
	void Zc_Member_Exit(const char* modeName);
	void Zc_Role_Change(const char* modeName);
	void Zc_Refuse_Enter_Room(const char* modeName);
	void Zc_Change_Chatroom(const char* modeName);
	void Zc_Req_Exchange_Item(const char* modeName);
	void Zc_Req_Exchange_Item2(const char* modeName);
	void Zc_Ack_Exchange_Item(const char* modeName);
	void Zc_Ack_Exchange_Item2(const char* modeName);
	void Zc_Add_Exchange_Item(const char* modeName);
	void Zc_Ack_Add_Exchange_Item(const char* modeName);
	void Zc_Conclude_Exchange_Item(const char* modeName);
	void Zc_Cancel_Exchange_Item(const char* modeName);
	void Zc_Exec_Exchange_Item(const char* modeName);
	void Zc_Store_Normal_Itemlist(const char* modeName);
	void Zc_Store_Normal_Itemlist2(const char* modeName);
	void Zc_Store_Normal_Itemlist3(const char* modeName);
	void Zc_Store_Equipment_Itemlist(const char* modeName);
	void Zc_Add_Item_To_Store(const char* modeName);
	void Zc_Delete_Item_From_Store(const char* modeName);
	void Zc_Close_Store(const char* modeName);
	void Zc_Notify_Storeitem_Countinfo(const char* modeName);
	void Zc_Ack_Make_Group(const char* modeName);
	void Zc_Group_List(const char* modeName);
	void Zc_Ack_Req_Join_Group(const char* modeName);
	void Zc_Req_Join_Group(const char* modeName);
	void Zc_Req_Couple(const char* modeName);
	void Zc_StarPlace(const char* modeName);
	void Zc_NotifyBindOnEquip(const char* modeName);
	void Zc_Battlefield_Notify_Campinfo(const char* modeName);
	void Zc_Battlefield_Chat(const char* modeName);
	void Zc_Battlefield_Notify_Point(const char* modeName);
	void Zc_Battlefield_Notify_Position(const char* modeName);
	void Zc_Battlefield_Notify_Hp(const char* modeName);
	void Zc_Notify_Font(const char* modeName);
	void Zc_Progress(const char* modeName);
	void Zc_Progress_Cancel(const char* modeName);
	void Zc_Req_Baby(const char* modeName);
	void Zc_StarSkill(const char* modeName);
	void Zc_Ack_Status_GM(const char* modeName);
	void Zc_Add_Member_To_Group(const char* modeName);
	void Zc_Add_Member_To_Group2(const char* modeName);
	void Zc_Delete_Member_From_Group(const char* modeName);
	void Zc_Notify_Hp_To_Groupm(const char* modeName);
	void Zc_Notify_Position_To_Groupm(const char* modeName);
	void Zc_Notify_Chat_Party(const char* modeName);
	void Zc_Exchangeitem_Undo(const char* modeName);
	void Zc_Mvp_Getting_Item(const char* modeName);
	void Zc_Mvp_Getting_Special_Exp(const char* modeName);
	void Zc_Mvp(const char* modeName);
	void Zc_Throw_Mvpitem(const char* modeName);
	void Zc_Skillinfo_Update(const char* modeName);
	void Zc_Skillinfo_List(const char* modeName);
	void Zc_All_Quest_List(const char* modeName);
	void Zc_All_Quest_Mission(const char* modeName);
	void Zc_Update_Mission_Hunt(const char* modeName);
	void Zc_Active_Quest(const char* modeName);
	void Zc_Add_Quest(const char* modeName);
	void Zc_Del_Quest(const char* modeName);
	void Zc_Ack_Touseskill(const char* modeName);
	void Zc_SKILLMSG(const char* modeName);
	void Zc_BABYMSG(const char* modeName);
	void Zc_Add_Skill(const char* modeName);
	void Zc_Notify_Skill(const char* modeName);
	void Zc_Notify_Groundskill(const char* modeName);
	void Zc_State_Change(const char* modeName);
	void Zc_State_Change3(const char* modeName);
	void Zc_Use_Skill(const char* modeName);
	void Zc_Skill_Entry(const char* modeName);
	void Zc_Skill_Disappear(const char* modeName);
	void Zc_Notify_Act_Position(const char* modeName);
	void Zc_Notify_Skill_Position(const char* modeName);
	void Zc_Warplist(const char* modeName);
	void Zc_Ack_Remember_Warppoint(const char* modeName);
	void Zc_Notify_Cartitem_Countinfo(const char* modeName);
	void Zc_Cart_Normal_Itemlist(const char* modeName);
	void Zc_Cart_Normal_Itemlist2(const char* modeName);
	void Zc_Cart_Normal_Itemlist3(const char* modeName);
	void Zc_Cart_Equipment_Itemlist(const char* modeName);
	void Zc_Cartoff(const char* modeName);
	void Zc_Ack_Additem_To_Cart(const char* modeName);
	void Zc_Add_Item_To_Cart(const char* modeName);
	void Zc_Delete_Item_From_Cart(const char* modeName);
	void Zc_openstore(const char* modeName);
	void Zc_store_entry(const char* modeName);
	void Zc_disappear_entry(const char* modeName);
	void Zc_pc_purchase_itemlist_frommc(const char* modeName);
	void Zc_pc_purchase_result_frommc(const char* modeName);
	void Zc_pc_purchase_myitemlist(const char* modeName);
	void Zc_deleteitem_from_mcstore(const char* modeName);
	void Zc_attack_failure_for_distance(const char* modeName);
	void Zc_attack_range(const char* modeName);
	void Zc_action_failure(const char* modeName);
	void Zc_equip_arrow(const char* modeName);
	void Zc_recovery(const char* modeName);
	void Zc_useskill_ack(const char* modeName);
	void Zc_groupinfo_change(const char* modeName);
	void Zc_couplestatus(const char* modeName);
	void Zc_open_editdlg(const char* modeName);
	void Zc_compass(const char* modeName);
	void Zc_show_image(const char* modeName);
	void Zc_show_image2(const char* modeName);
	void Zc_autorun_skill(const char* modeName);
	void Zc_resurrection(const char* modeName);
	void Zc_guild_emblem_img(const char* modeName);
	void Zc_guild_info(const char* modeName);
	void Zc_myguild_basic_info(const char* modeName);
	void Zc_ack_guild_menuinterface(const char* modeName);
	void Zc_position_id_name_info(const char* modeName);
	void Zc_ack_req_change_members(const char* modeName);
	void Zc_membermgr_info(const char* modeName);
	void Zc_ack_open_member_info(const char* modeName);
	void Zc_ack_leave_guild(const char* modeName);
	void Zc_ack_ban_guild(const char* modeName);
	void Zc_ack_disorganize_guild_result(const char* modeName);
	void Zc_ack_disorganize_guild(const char* modeName);
	void Zc_position_info(const char* modeName);
	void Zc_ban_list(const char* modeName);
	void Zc_other_guild_list(const char* modeName);
	void Zc_result_make_guild(const char* modeName);
	void Zc_ack_req_join_guild(const char* modeName);
	void Zc_req_join_guild(const char* modeName);
	void Zc_update_gdid(const char* modeName);
	void Zc_update_charstat(const char* modeName);
	void Zc_update_charstat2(const char* modeName);
	void Zc_ack_give_manner_point(const char* modeName);
	void Zc_notify_manner_point_given(const char* modeName);
	void Zc_guild_notice(const char* modeName);
	void Zc_guild_chat(const char* modeName);
	void Zc_req_ally_guild(const char* modeName);
	void Zc_ack_req_ally_guild(const char* modeName);
	void Zc_ack_change_guild_positioninfo(const char* modeName);
	void Zc_itemidentify_list(const char* modeName);
	void Zc_ack_itemidentify(const char* modeName);
	void Zc_itemcomposition_list(const char* modeName);
	void Zc_ack_itemcomposition(const char* modeName);
	void Zc_ack_req_hostile_guild(const char* modeName);
	void Zc_member_add(const char* modeName);
	void Zc_delete_related_guild(const char* modeName);
	void Zc_add_related_guild(const char* modeName);
	void Zc_Notify_Position_To_Guildm(const char* modeName);
	void Zc_Guild_Member_Map_Change(const char* modeName);
	void Zc_ack_itemrefining(const char* modeName);
	void Zc_notify_mapinfo(const char* modeName);
	void Zc_ack_req_disconnect(const char* modeName);
	void Zc_monster_info(const char* modeName);
	void Zc_makable_item_list(const char* modeName);
	void Zc_repair_item_list(const char* modeName);
	void Zc_notify_weapon_item_list(const char* modeName);
	void Zc_ack_weaponrefine(const char* modeName);
	void Zc_ack_itemrepair(const char* modeName);
	void Zc_ack_req_itemmaking(const char* modeName);
	void Zc_talkbox_chat_contents(const char* modeName);
	void Zc_update_mapinfo(const char* modeName);
	void Zc_Ack_Reqname_bygid(const char* modeName);
	void Zc_Ack_Reqnameall(const char* modeName);
	void Zc_Msg_State_Change(const char* modeName);
	void Zc_Npc_Showefst_Update(const char* modeName);
	void Zc_Notify_Mapproperty(const char* modeName);
	void Zc_Notify_Ranking(const char* modeName);
	void Zc_Notify_Effect(const char* modeName);
	void Zc_Notify_Effect2(const char* modeName);
	void Zc_Notify_Effect3(const char* modeName);
	void Zc_Start_Capture(const char* modeName);
	void Zc_Start_Couple(const char* modeName);
	void Zc_Start_Baby(const char* modeName);
	void Zc_CoupleName(const char* modeName);
	void Zc_Congratulation(const char* modeName);
	void Zc_TryCapture_Monster(const char* modeName);
	void Zc_Change_Pet_Status(const char* modeName);
	void Zc_PetEgg_List(const char* modeName);
	void Zc_Property_Pet(const char* modeName);
	void Zc_Feed_Pet(const char* modeName);
	void Zc_Pet_Act(const char* modeName);
	void Zc_Par_Change_User(const char* modeName);
	void Zc_Skill_Update(const char* modeName);
	void Zc_MakingArrowList(const char* modeName);
	void Zc_MakingItemList(const char* modeName);
	void Zc_ChangeCartList(const char* modeName);
	void Zc_NpcSpriteChange(const char* modeName);
	void Zc_ShowDigit(const char* modeName);
	void Zc_ChangeGuild(const char* modeName);
	void Zc_GuildInfo2(const char* modeName);
	void Zc_GuildZenyAck(const char* modeName);
	void Zc_Dispel(const char* modeName);
	void Zc_Reply_RemainTime(const char* modeName);
	void Zc_Info_RemainTime(const char* modeName);
	void Zc_Broadcast2(const char* modeName);
	void Zc_GuildSkillInfo(const char* modeName);
	void Zc_AddItemToStore2(const char* modeName);
	void Zc_Add_Item_To_Cart2(const char* modeName);
	void Sc_Ack_Encryption(const char* modeName);
	void Zc_Use_Item_Ack2(const char* modeName);
	void Zc_Skill_Entry2(const char* modeName);
	void Zc_Monster_Talk(const char* modeName);
	void Zc_AutoSpellList(const char* modeName);
	void Zc_DevotionList(const char* modeName);
	void Zc_Spirits(const char* modeName);
	void Zc_Spirits2(const char* modeName);
	void Zc_BladeStop(const char* modeName);
	void Zc_Sound(const char* modeName);
	void Zc_Open_EditDlgStr(const char* modeName);
	void Zc_ComboDelay(const char* modeName);
	void Zc_Notify_Mapproperty2(const char* modeName);
	void Zc_Sprite_Change2(const char* modeName);
	void Zc_Notify_Newentry2(const char* modeName);
	void Zc_Notify_Standentry2(const char* modeName);
	void Zc_Notify_Moveentry2(const char* modeName);
	void Zc_Notify_Standentry3(const char* modeName);
	void Zc_Notify_Standentry4(const char* modeName);
	void Zc_Notify_Newentry3(const char* modeName);
	void Zc_Notify_Newentry4(const char* modeName);
	void Zc_Notify_Moveentry3(const char* modeName);
	void Zc_Notify_Moveentry4(const char* modeName);
	void GameModeShowEFST(int GID, int effectState, int clevel, int showEFST);
	void Zc_Notify_Skill2(const char* modeName);
	void Zc_Ack_AccountName(const char* modeName);
	void Zc_Friends_List(const char* modeName);
	void Zc_Req_Join_Friend(const char* modeName);
	void Zc_Add_Friends_List(const char* modeName);
	void Zc_Req_Delete_Friend(const char* modeName);
	void Zc_Friends_State(const char* modeName);
	void Zc_Divorce(const char* modeName);
	void ZC_BLACKSMITH_RANK(const char* modeName);
	void ZC_ALCHEMIST_RANK(const char* modeName);
	void ZC_TAEKWON_RANK(const char* modeName);
	void ZC_GANGSI_RANK(const char* modeName);
	void ZC_BLACKSMITH_POINT(const char* modeName);
	void ZC_ALCHEMIST_POINT(const char* modeName);
	void ZC_TAEKWON_POINT(const char* modeName);
	void ZC_GANGSI_POINT(const char* modeName);
	void ZC_DEATH_QUESTION(const char* modeName);
	void Zc_Ack_PvpPoint(const char* modeName);
	void Zc_Notify_PKinfo(const char* modeName);
	void Zc_Notify_CrazyKiller(const char* modeName);
	void Zc_Game_Guard(const char* modeName);
	void Zc_Change_Mer_Status(const char* modeName);
	void Zc_Property_Homun(const char* modeName);
	void Zc_Mer_Init(const char* modeName);
	void Zc_Mer_Par_Change(const char* modeName);
	void ZC_KILLER_POINT(const char* modeName);
	void ZC_KILLER_RANK(const char* modeName);
	void Zc_Mail_Req_Get_List(const char* modeName);
	void Zc_Mail_Req_Open(const char* modeName);
	void Zc_Mail_Req_Get_Item(const char* modeName);
	void Zc_Mail_Req_Send(const char* modeName);
	void Zc_Mail_Receive(const char* modeName);
	void Zc_Ack_Mail_Add_Item(const char* modeName);
	void Zc_Ack_Mail_Delete(const char* modeName);
	void Zc_Mail_Windows(const char* modeName);
	void Zc_Ack_Mail_Return(const char* modeName);
	void Zc_Ack_Auction_Add_Item(const char* modeName);
	void Zc_Auction_Result(const char* modeName);
	void Zc_Auction_Item_Req_Search(const char* modeName);
	void Zc_Auction_My_Sell_Stop(const char* modeName);
	void Zc_Auction_Windows(const char* modeName);
	void Zc_HuntingList(const char* modeName);
	void Zc_Pcbang_Effect(const char* modeName);
	void Zc_Aid(const char* modeName);
	void Zc_Pc_Cash_Point_ItemList(const char* modeName);
	void Zc_Pc_Cash_Point_Update(const char* modeName);
	void Zc_Boss_Info(const char* modeName);
	void Zc_Msg(const char* modeName);
	void Zc_Read_Book(const char* modeName);
	void Zc_Equipment_Itemlist2(const char* modeName);
	void Zc_Store_Equipment_Itemlist2(const char* modeName);
	void Zc_Cart_Equipment_Itemlist2(const char* modeName);
	void Zc_Cash_Time_Counter(const char* modeName);
	void Zc_Cash_Item_Delete(const char* modeName);
	void Zc_Item_Pickup_Ack2(const char* modeName);
	void Zc_Equipment_Itemlist3(const char* modeName);
	void Zc_Store_Equipment_Itemlist3(const char* modeName);
	void Zc_Cart_Equipment_Itemlist3(const char* modeName);
	void Zc_Item_Pickup_Ack3(const char* modeName);
	void Zc_Item_Pickup_Party(const char* modeName);
	void Zc_EquipItem_Damaged(const char* modeName);
	void Zc_ShortCut_Key_List(const char* modeName);
	void Zc_Npc_Chat(const char* modeName);
	void Zc_Formatstring_Msg(const char* modeName);
	void Zc_Party_Config(const char* modeName);
	void Zc_MemorialDungeon_Subscription_Info(const char* modeName);
	void Zc_MemorialDungeon_Subscription_Notify(const char* modeName);
	void Zc_MemorialDungeon_Info(const char* modeName);
	void Zc_MemorialDungeon_Notify(const char* modeName);
	void Zc_Isvr_Disconnect(const char* modeName);
	void Zc_EquipwinMicroscope(const char* modeName);
	void Zc_Config(const char* modeName);
	void Zc_Config_Notify(const char* modeName);
	void Zc_Mapproperty(const char* modeName);
	void WriteChat(char* txt);
	void MakeChatFile();
	void CloseChatFile();
	void SetAutoFollowTime(unsigned int oldFollowTime, unsigned int oldDisappearTime);
	void Inner_Zc_Item_Fall_Entry2(const unsigned int in_ITAID, const unsigned short in_ITID, const bool in_IsIdentified, const short in_count, const short in_xPos, const short in_yPos, const unsigned char in_subX, const unsigned char in_subY);
	void Zc_Item_Fall_Entry2(const char* modeName);
	void Zc_Item_Fall_Entry2__m1(const char* modeName);
	void Zc_Item_Fall_Entry2__m2(const char* modeName);
	void Zc_Item_Fall_Entry2__m3(const char* modeName);
	void Zc_Item_Fall_Entry2__m4(const char* modeName);
	void Zc_Item_Fall_Entry2__m5(const char* modeName);
	void Zc_Item_Fall_Entry2__m6(const char* modeName);
	void Inner_Zc_Notify_Moveentry5(const unsigned char in_objecttype, const unsigned int in_GID, const short in_speed, const short in_bodyState, const short in_healthState, const int in_effectState, const short in_job, const short in_head, const int in_weapon, const short in_accessory, const unsigned int in_moveStartTime, const short in_accessory2, const short in_accessory3, const short in_headpalette, const short in_bodypalette, const short in_headDir, const unsigned int in_GUID, const short in_GEmblemVer, const short in_honor, const int in_virtue, const bool in_isPKModeON, const unsigned char in_sex, const unsigned char* in_pMoveData, const unsigned char in_xSize, const unsigned char in_ySize, const short in_clevel);
	void Zc_Notify_Moveentry5(const char* modeName);
	void Zc_Notify_Moveentry5__m1(const char* modeName);
	void Zc_Notify_Moveentry5__m2(const char* modeName);
	void Zc_Notify_Moveentry5__m3(const char* modeName);
	void Zc_Notify_Moveentry5__m4(const char* modeName);
	void Zc_Notify_Moveentry5__m5(const char* modeName);
	void Zc_Notify_Moveentry5__m6(const char* modeName);
	void Inner_Zc_Notify_Npc_Standentry(const unsigned char in_objecttype, const unsigned int in_GID, const short in_speed, const short in_bodyState, const short in_healthState, const short in_effectState, const short in_job, const short in_head, const short in_weapon, const short in_accessory, const short in_shield, const short in_accessory2, const short in_accessory3, const short in_headpalette, const short in_bodypalette, const short in_headDir, const unsigned int in_GUID, const short in_GEmblemVer, const short in_honor, const short in_virtue, const bool in_isPKModeON, const unsigned char in_sex, const unsigned char* in_PosDir, const unsigned char in_xSize, const unsigned char in_ySize, const unsigned char in_state, const short in_clevel);
	void Zc_Notify_Npc_Standentry(const char* modeName);
	void Zc_Notify_Npc_Standentry__m1(const char* modeName);
	void Zc_Notify_Npc_Standentry__m2(const char* modeName);
	void Zc_Notify_Npc_Standentry__m3(const char* modeName);
	void Zc_Notify_Npc_Standentry__m4(const char* modeName);
	void Zc_Notify_Npc_Standentry__m5(const char* modeName);
	void Zc_Notify_Npc_Standentry__m6(const char* modeName);
private:
	int m_areaLeft;
	int m_areaRight;
	int m_areaTop;
	int m_areaBottom;
	char m_rswName[40];
	char m_minimapBmpName[60];
	CWorld* m_world;
	CView* m_view;
	CMousePointer* m_mousePointer;
	unsigned int m_leftBtnClickTick;
	int m_oldMouseX;
	int m_oldMouseY;
	int m_rBtnClickX;
	int m_rBtnClickY;
	unsigned int m_lastPcGid;
	unsigned int m_lastMonGid;
	unsigned int m_lastLockOnMonGid;
	int m_isAutoMoveClickOn;
	int m_isWaitingWhisperSetting;
	int m_isWaitingEnterRoom;
	int m_isWaitingAddExchangeItem;
	unsigned int m_waitingWearEquipAck;
	unsigned int m_waitingTakeoffEquipAck;
	int m_isReqUpgradeSkillLevel;
	int m_exchangeItemCnt;
	int m_isWaitingCancelExchangeItem;
	std::string m_refuseWhisperName;
	std::string m_streamFileName;
	std::string m_lastExchangeCharacterName;
	std::map<unsigned int, NamePair> m_actorNameList;
	std::map<unsigned int, unsigned int> m_actorNameReqTimer;
	std::map<unsigned int, NamePair> m_actorNameListByGID;
	std::map<unsigned int, unsigned int> m_actorNameByGIDReqTimer;
	std::map<unsigned int, int> m_guildMemberStatusCache;
	std::map<unsigned int, CellPos> m_actorPosList;
	std::list<unsigned int> m_pickupReqItemNaidList;
	std::map<unsigned int, unsigned int> m_aidList;
	std::map<unsigned int, ColorCellPos> m_partyPosList;
	std::map<unsigned int, ColorCellPos> m_guildPosList;
	std::map<unsigned int, ColorCellPos2> m_compassPosList;
	std::vector<int> m_menuIdList;
	std::list<unsigned int> m_visibleTrapList;
	std::list<unsigned int> m_emblemReqGdidQueue;
	unsigned int m_lastEmblemReqTick;
	unsigned int m_lastNameWaitingListTick;
	std::vector<PLAY_WAVE_INFO> m_playWaveList;
	std::vector<unsigned int> m_KillerList;
	std::string m_lastWhisperMenuCharacterName;
	std::string m_lastWhisper;
	std::string m_lastWhisperName;
	int m_noMove;
	unsigned int m_noMoveStartTick;
	int m_isOnQuest;
	int m_isPlayerDead;
	int m_canRotateView;
	int m_hasViewPoint;
	short ViewPointData[9];
	int m_receiveSyneRequestTime;
	unsigned int m_syncRequestTime;
	unsigned int m_usedCachesUnloadTick;
	unsigned int m_reqEmotionTick;
	unsigned int m_reqTickChatRoom;
	unsigned int m_reqTickMerchantShop;
	int m_isReqEmotion;
	float m_fixedLongitude;
	unsigned int m_lastCouplePacketAid;
	unsigned int m_lastCouplePacketGid;
	char m_CoupleName[24];
	UINameBalloonText* m_nameBalloon;
	UINameBalloonText* m_targetNameBalloon;
	UITransBalloonText* m_broadcastBalloon;
	UIPlayerGage* m_playerGage;
	UITransBalloonText* m_skillNameBalloon;
	UITransBalloonText* m_skillMsgBalloon;
	UITransBalloonText* m_skillUsedMsgBalloon;
	unsigned int m_skillUsedTick;
	unsigned int m_broadCastTick;
	int m_nameDisplayed;
	int m_nameDisplayed2;
	int m_waitingUseItemAck;
	int m_waitingItemThrowAck;
	int m_waitingReqStatusAck;
	unsigned int m_nameActorAid;
	unsigned int m_lastNaid;
	unsigned int m_menuTargetAID;
	int m_nameBalloonWidth;
	int m_nameBalloonHeight;
	int m_dragType;
	DRAG_INFO m_dragInfo;
	ChatRoomInfo m_lastChatroomInfo;
	SKILL_USE_INFO m_skillUseInfo;
	SHOW_IMAGE_INFO m_showImageInfo;
	std::string m_lastChat;
	int m_sameChatRepeatCnt;
	int m_numNotifyTime;
	int m_isCheckGndAlpha;
	int m_lastCardItemIndex;
	int m_SkillBallonSkillId;
	unsigned int m_nameBalloonType;
	unsigned int m_showTimeStartTick;
	int m_recordChatNum;
	std::string m_recordChat[11];
	unsigned int m_recordChatTime[11];
	int m_strikeNum;
	unsigned int m_strikeTime[3];
	unsigned int m_doritime[6];
	int m_isCtrlLock;
	int m_isUseYgdrasil;
	int m_isMakeYgdrasil;
	int m_autoSaveChatCnt;
	CellPos m_posOfBossMon;
	unsigned char m_isBossAlarm;
	unsigned char m_onCopyName;
	CScheduler m_scheduler;
public:
	unsigned char m_SiyeonStatus[6];
	int m_MouseRollCount;
	int m_MouseRollMax;
};
extern	class CPathFinder	g_pathFinder;

struct RoomMemberData {
public:
	std::string name;
	unsigned int role;
};

#endif
