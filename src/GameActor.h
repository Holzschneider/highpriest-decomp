#ifndef HIGHPRIEST_GAMEACTOR_H
#define HIGHPRIEST_GAMEACTOR_H

#include "3dlib/Prim.h"
#include "3dlib/Renderer.h"
#include "resource/Sprite.h"
#include "resource/Action.h"
#include "resource/Model.h"
#include "granny/granny.h"
#include "PathFinder.h"
#include "Session.h"
#include "window/UISys.h"
#include "MercenaryAI.h"

class C3dActor;
class CMsgEffect;
class CRagEffect;
class CGameActor;
struct RENDER_INFO_RECT;







enum	ACTORMESSAGE {
	AM_MOVE = 0, AM_SETPOSMOUSE, AM_HIDEMOUSE, AM_STANDENTRY, AM_INITSTANDENTRY, AM_MOVEENTRY, AM_INITMOVEENTRY,
	AM_CHAT, AM_FREEZE, AM_PREMOVE, AM_PROCEEDATTACK, AM_ACTION, AM_DEAD, AM_CANCELATTACK, AM_SETPOS, AM_ATTACKCLICKUP,
	AM_REQ_CHANGE_DIR, AM_REQ_MOVE, AM_PROCEEDPICKUPITEM, AM_STOPMOVE, AM_USEITEM_EFFECT, AM_EMOTION, AM_SETMASTERGID,
	AM_EFFECT_DEAD, AM_MASTER_DEAD, AM_CHILD_DEAD, AM_ATTACKED, AM_WILL_BE_ATTACKED, AM_WILL_BE_DEAD, AM_REQ_ATTACK,
	AM_REQ_STAND, AM_REQ_SIT, AM_MAKE_CHAT_TITLE, AM_DELETE_CHAT_TITLE, AM_UPDATEGAGE, AM_REMOVEGAGE, AM_MVP,
	AM_SKILL, AM_GROUNDSKILL, AM_ATTACKEDNOMOTION, AM_REQ_ATTACK_REPEAT, AM_PROCEEDSKILL, AM_REQ_SKILL,
	AM_ATTR_STATE_CHANGE, AM_SETCOUNT, AM_SKILLSTANDENTRY, AM_SETPARAM, AM_SETJOB, AM_MSGEFFECT_DEAD, AM_MSGMASTER_DEAD,
	AM_SET_MASTER_POINTER, AM_MAKE_CART, AM_DELETE_CART, AM_DISAPPEAR, AM_DELETE_SHOP_TITLE, AM_MAKE_SHOP_TITLE,
	AM_MAKE_PKEFFECT, AM_DELETE_PKEFFECT, AM_MAKE_ICEEFFECT, AM_DELETE_ICEEFFECT, AM_RESUMEPROCEEDATTACK,
	AM_CORRECT_POS, AM_MAKE_ICEBREAKE_FFECT, AM_MAKE_SMOKE_FFECT, AM_SETPOS2,
	AM_PROCEEDGROUNDSKILL, AM_REQ_GROUND_SKILL, AM_MAKE_CURSE_EFFECT, AM_DELETE_CURSE_EFFECT,
	AM_MAKE_SLEEP_EFFECT, AM_DELETE_SLEEP_EFFECT, AM_MAKE_STUN_EFFECT, AM_DELETE_STUN_EFFECT,
	AM_MAKE_SILENCE_EFFECT, AM_DELETE_SILENCE_EFFECT, AM_MAKE_ANGELUS_EFFECT, AM_DELETE_ANGELUS_EFFECT,
	AM_MAKE_BLUR_ACTOR, AM_MAKE_CRUCIS_EFFECT, AM_DELETE_CRUCIS_EFFECT, AM_SET_DURATION, AM_SET_TARGET_AID,
	AM_MAKE_SKILL_RECHARGEGAGE, AM_DELETE_SKILL_RECHARGEGAGE, AM_DELETE_BEGIN_SPELL, AM_MAKE_BEGIN_SPELL,
	AM_MAKE_MAGIC_TARGET, AM_DELETE_MAGIC_TARGET, AM_MAKE_NUMBER, AM_MAKE_MISS, AM_SET_SKILL_USE_LEVEL,
	AM_REQ_MOVE_PRIMITIVE, AM_MAKE_CRITICAL_EFFECT,	AM_ATTACKED_CRITICAL, AM_ATTACKED_LUCKY, AM_MAKE_LUCKY,
	AM_ATTACKED_SKILL, AM_ATTACKEDNOMOTION_SKILL, AM_RESURRECTION, AM_OUTOFSIGHT, AM_SET_HEAD_DIR,
	AM_MAKE_BIRD, AM_DELETE_BIRD, AM_PROCEEDTOUCHSKILL, AM_EFFECT_CONTINUE, AM_MAKE_CHICKEN, AM_DELETE_CHICKEN,
	AM_MAKE_FREEZINGEFFECT, AM_WILL_BE_ATTACK, AM_DELETE_EFFECT, AM_STATUS_CHANGE, AM_ATTACKED_MANYTIMES,
	AM_MAKE_RANKING, AM_DELETE_RANKING, AM_CHANGE_RANK, AM_SETTIME, AM_SHOW_GROUND_SCOPE, AM_SET_SKILL,
	AM_DELETE_PROHIBITEFFECT, AM_SKILL_UPDATE, AM_IS_HAVE_EFFECT, AM_INITSTANDENTRY2, AM_INITMOVEENTRY2,
	AM_MOVEENTRY2, AM_STANDENTRY2,  AM_UPDATEGAGE2,


	AM_99,
	AM_MAKE_GUARD, AM_CHANGE_COLOR, AM_RETURN_COLOR,
	AM_ATTACKED_MANYTIMES2, AM_WILL_BE_ATTACKED2, AM_MAKE_BLUE_EFFECT, AM_ATTACKED_MANYTIMES2DASH, AM_WILL_BE_ATTACKED2DASH,
	AM_MAKE_CURSOR_EFFECT, AM_DELETE_CURSOR_EFFECT, AM_CAMPINFO,
	AM_UPDATEGAGE_HP,
	AM_SHAKE_ICEEFFECT,



	AM_REQ_GATHER ,
	AM_EFFECT_LOOP_COUNT ,
	AM_MAKE_MOVETO_EFFECT ,
	AM_MAKE_BOTPOINT_EFFECT ,
	AM_DELETE_MOVETO_EFFECT ,
	AM_DELETE_BOTPOINT_EFFECT ,
};





enum	MERCENARYTYPE {
	MT_HOMUN = 0,
	MT_MER,
};


enum	MOTIONTYPE {
	MT_LOOP = 0, MT_ONETIME,MT_STATIC
};





enum	STATEID {
	ST_STAND = 0,
	ST_MOVE = 1,
	ST_ATTACK = 2,
	ST_DEAD = 3,
	ST_DAMAGE = 4,
	ST_PICKUP = 5,
	ST_SIT = 6,
	ST_SKILL = 7,
	ST_BEGIN_SKILL = 8,
	ST_ATTACK2 = 9,
	ST_SLEEP = 10,
	ST_ATTACKWAIT = 11,
	ST_THROW = 12,
	ST_AUTOCOUNTER = 13,
	ST_DANCE = 16,
	ST_SING = 17,
	ST_READYSHOT = 18,
	ST_READYJUMP = 19,
	ST_LANDJUMP = 20,
	ST_R_STORM = 21,
	ST_R_DOWN = 22,
	ST_R_TURN = 23,
	ST_R_COUNTER = 24,
	ST_R_DODGE = 25,
	ST_A_COUNTER = 26,
	ST_A_JUMPKICK = 27,
	ST_THROW2 = 28,
	ST_FEED = 29,
	ST_A_STORM = 30,
	ST_A_DOWN = 31,
	ST_A_TURN = 32,
	ST_SG_DOWN = 33,
	ST_SG_TURN = 34,
	ST_NINJASKILL1 = 35,
	ST_NINJASKILL2 = 36,
	ST_NINJAREADY = 37,
	ST_GUNSKILL1 = 38,
	ST_GUNSKILL2 = 39,
	ST_GUNSKILL3 = 40,
	ST_GUNREADY = 41,
	ST_GUNREADY2 = 42,
};

struct ACTOR_COLOR {
	BYTE a;
	BYTE r;
	BYTE g;
	BYTE b;
};

struct ACTSTRUCT {
	DWORD damage;
	DWORD leftDamage;
	DWORD action;
	DWORD targetGID;
	DWORD startTime;
	int count;
	int tCellX;
	int tCellY;
	int attackMT;
	int attackedMT;
};

struct SKILLSTRUCT {
	DWORD skillId;
	DWORD damage;
	DWORD action;
	DWORD targetGID;
	DWORD startTime;
	int level;
	int count;
	int tCellX;
	int tCellY;
	int attackMT;
	int attackedMT;
};

struct GROUNDSKILLSTRUCT {
	DWORD skillId;
	DWORD startTime;
	int cellX, cellY;
	int level;
};

struct ATTRSTATESTRUCT {
	int bodyState;
	int effectState;
	int healthState;
	int isPKModeON;
};

struct MOVESTRUCT {
	DWORD startTime;
	int sx, sy;
	int cellX, cellY;
	int dx, dy;
};

struct ITEMSTANDENTRYSTRUCT {
	char name[32];
	DWORD ITAID;
	int x, y;
	int subX, subY;
	BOOL isJumpEntry;
	int count;
	BYTE isIdentified;
};



struct	STANDENTRYSTRUCT {
	STANDENTRYSTRUCT() {
		job = 0;
		sex = 0;
		x = 0;
		y = 0;
		dir = 0;
		speed = 0;
		xSize = 0;
		ySize = 0;
		weapon = 0;
		accessory = 0;
		state = 0;
		shield = 0;
		accessory2 = 0;
		accessory3 = 0;
		headpalette = 0;
		bodypalette = 0;
		headDir = 0;
		healthState = 0;
		bodyState = 0;
		effectState = 0;
		isPKModeON = 0;
		guildId = 0;
		guildEmblemVersion = 0;
		honor = 0;
		virtue = 0;
	}
	DWORD	GID;
	int		job;
	int		sex;
	int		x, y, dir;
	DWORD	speed;
	int		xSize;
	int		ySize;
	int		head;
	int		weapon;
	int		accessory;
	int		state;
	int		bodyState;
	int		healthState;
	int		effectState;
	int		isPKModeON;

	int		shield;
	int		accessory2;
	int		accessory3;
	int		headpalette;
	int		bodypalette;
	int		headDir;

	DWORD	guildId;
	int		guildEmblemVersion;
	int		honor;
	int		virtue;
};

struct MOVEENTRYSTRUCT {
	DWORD GID;
	int job;
	int sex;
	DWORD moveStartTime;
	int sx, sy;
	int cellX, cellY;
	int dx, dy;
	DWORD speed;
	int head;
	int weapon;
	int accessory;
	int bodyState;
	int healthState;
	int effectState;
	int isPKModeON;
	int shield;
	int accessory2;
	int accessory3;
	int headpalette;
	int bodypalette;
	int headDir;
	DWORD guildId;
	int guildEmblemVersion;
	int honor;
	int virtue;
};

struct SKILLSTANDENTRYSTRUCT {
	DWORD AID;
	DWORD creatorAID;
	int job;
	int xPos;
	int yPos;
	BYTE isVisible;
};

struct WBA {
	WBA(int nMessage, int ndamage, DWORD ntime, DWORD ntargetGid, int nTCellX, int nTCellY,
		int nEffectId1, int nEffectId2, int nEffectLevel, float nAttackedSpeedFactor)
	{
		message = nMessage;
		damage = ndamage;
		time = ntime;
		targetGid = ntargetGid;
		effectId1 = nEffectId1;
		effectId2 = nEffectId2;
		effectLevel = nEffectLevel;
		tCellX = nTCellX;
		tCellY = nTCellY;
		attackedSpeedFactor = nAttackedSpeedFactor;
	}
	int message;
	int damage;
	DWORD time;
	DWORD targetGid;
	int tCellX;
	int tCellY;
	int effectId1;
	int effectId2;
	int effectLevel;
	float attackedSpeedFactor;
};

class CGameObject {
public:
	CGameObject();
	virtual ~CGameObject();
	void OnInit();
	void OnExit();
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1 = 0, INT_PTR arg2 = 0, INT_PTR arg3 = 0);
	virtual void Render(matrix& ptm) {}
	virtual int Get8Dir(float rot);
	int Get8Dir2(float rot, int isReal8Dir);
	int GetDir(float rot);
};

class CRenderObject : public CGameObject {
public:
	CRenderObject();
	virtual ~CRenderObject();
	virtual bool OnProcess() { return false; }
	virtual void Render(matrix& vtm);
	virtual void SetRenderInfo(RENDER_INFO_RECT& info, const float tlvertX, const float tlvertY);
	virtual void SetTlvert(float x, float z);
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1 = 0, INT_PTR arg2 = 0, INT_PTR arg3 = 0);
	virtual void SetAction(int action, int speed, int type);
	virtual void ProcessMotion();
	void OnInit();
	void OnExit();
	void UpdateTlvertex(matrix& vtm);
	void RenderBody(matrix& vtm, char FLAG1);
	void RenderMercenary(matrix& vtm, int ML);
	void RenderShadow(matrix& vtm, float zoom);
	void RenderSprite(const RPSprite& spr, SYSTEMPALETTE pal[], int renderFlag);
	float CalcDir(float, float, float, float);
	void CalcDir(float x, float z);
	void ModifyArgb(int a, int r, int g, int b);
	void SetArgb(int a, int r, int g, int b);
	void ProcessSound();
	void SetLmIntensity(float& lmRIntensity, float& lmGIntensity, float& lmBIntensity);
	float CalcDist(float, float, float);
	float CalcDist(float x, float z);
	float GetStateCnt();
	float GetLastPixelRatio() { return m_lastPixelRatio; }
	bool GetTargetPos(unsigned int targetGid, vector3d& tPos);
	int GetLastTlvertX() { return m_lastTlvertX; }
	int GetLastTlvertY() { return m_lastTlvertY; }
	void SetForceAnimSpeed(unsigned char speed);
	void SetForceAnimEndTick(unsigned int tick);
	void SetForceAnimation(int act, int mot, int maxMot, int finishAct, int finishMot, int isAnim, unsigned int endTick, int isLoop = 1);
	void SetForceAnimation2(int act, int mot, int act2, int mot2, int act3, int mot3, int act4, int mot4, int act5, int mot5, int isAnim, unsigned int endTick, int isLoop);
	void SetForceAnimation3(int act, int mot, int maxMot, int isAnim, unsigned int endTick, int isLoop);
	void SetSprName(const char* sprName);
	const char* GetSprName();
	CSprRes* GetSprRes();
	void Palette_BW(CPaletteRes* palRes);
	void SetActName(const char* sprName);
	void SetCharacterSquare(float* top, float* btm, float* left, float* right);
	void SetCharacterAngle(float* pixelratio);
	void SetCharacterPreAngle(int layer, int* imfOffX, int* imfOffY, int* head_imfOffX, int* head_imfOffY);
	void SetCharacterLight(RPSprite spr, SYSTEMPALETTE pal[]);
	void SetCharacterPixelRatio(float* pixelratio);
	short SetCharacterFlying();
	CActRes* SetCharacterActRes(int m_accessory2, int layer);
	CSprRes* SetCharacterSprRes(int m_accessory2, int layer);
	const char* GetActName();
	CActRes* GetActRes();
	vector3d m_pos;
	std::string m_bodyPaletteName;
	int m_baseAction;
	int m_curAction;
	int m_curMotion;
	int m_oldBaseAction;
	int m_oldMotion;
	int m_bodyPalette;
	float m_roty;
	float m_zoom;
	float m_shadowZoom;
	float m_motionSpeed;
	float m_lastPixelRatio;
	float m_loopCountOfmotionFinish;
	float m_modifyFactorOfmotionSpeed;
	float m_modifyFactorOfmotionSpeed2;
	int m_motionType;
	int m_stateId;
	int m_oldstateId;
	int m_sprShift;
	int m_sprAngle;
	int m_offsetOow;
	unsigned int m_colorOfSingleColor;
	unsigned int m_singleColorStartTick;
	unsigned int m_stateStartTick;
	ACTOR_COLOR m_oldColor;
	ACTOR_COLOR m_curColor;
	union {
		unsigned int m_sprArgb;
		COLOR m_sprColor;
	};
	unsigned char m_isLieOnGround;
	unsigned char m_isMotionFinished;
	unsigned char m_isMotionFreezed;
	unsigned char m_isSingleColor;
	unsigned char m_isVisible;
	unsigned char m_isVisibleBody;
	unsigned char m_alwaysTopLayer;
	unsigned char m_isSprArgbFixed;
	unsigned char m_shadowOn;
	unsigned char m_shouldAddPickInfo;
	int m_isPc;
	int m_lastTlvertX;
	int m_lastTlvertY;
	unsigned char m_forceAct;
	unsigned char m_forceMot;
	unsigned char m_forceAct2[5];
	unsigned char m_forceMot2[5];
	unsigned char m_forceMaxMot;
	unsigned char m_forceAnimSpeed;
	unsigned char m_forceFinishedAct;
	unsigned char m_forceFinishedMot;
	unsigned char m_forceStartMot;
	int m_isForceState;
	int m_isForceAnimLoop;
	int m_isForceAnimation;
	int m_isForceAnimFinish;
	int m_isForceState2;
	int m_isForceState3;
	unsigned int m_forceStateCnt;
	unsigned int m_forceStateEndTick;
	int m_BodyLight;
	char m_BeZero;
	char m_BodyFlag;
	unsigned char m_BodySin;
	unsigned char m_BodySin2;
	unsigned char m_BodySin3;
	unsigned char m_BodySin4;
	unsigned char m_BodySin5;
	short m_BodyTime;
	unsigned char m_BodyTime2;
	unsigned char m_BodyTime3;
	short m_FlyMove;
	unsigned char m_FlyNow;
	char m_camp;
	short m_charfont;
	unsigned char m_BodyAni;
	unsigned char m_BodyAct;
	unsigned char m_BodyAniFrame;
private:
	CSprRes* m_sprRes;
	CActRes* m_actRes;
};

class CAbleToMakeEffect : public CRenderObject {
public:
	CAbleToMakeEffect();
	virtual ~CAbleToMakeEffect();
	virtual void SendMsg(CGameObject* sender, int msgid, INT_PTR val1 = 0, INT_PTR val2 = 0, INT_PTR val3 = 0);
	void OnInit();
	void OnExit();
	CRagEffect* LaunchEffect(int effectId, vector3d fDeltaPos = vector3d(0.0f, 0.0f, 0.0f), float fRot = 0.0f);
	bool IsExistEffect(CRagEffect* pEffect);
	int IsEffectExist(int EffectId);
	int m_efId;
	int m_Sk_Level;
	int m_isLoop;
private:
	std::list<CRagEffect *> m_effectList;
	CRagEffect* m_beginSpellEffect;
	CRagEffect* m_magicTargetEffect;
};

class CDummyEffectLauncher : public CAbleToMakeEffect {
public:
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3, INT_PTR arg4);
private:
	int m_lastCnt;
};

class CGroundScope : public CAbleToMakeEffect {
public:
	CGroundScope();
	virtual void Render(matrix& vtm);
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
private:
	int GetScopeSize(int skill_id);
private:
	int m_master_gid;
	int m_ground_pos_x;
	int m_ground_pos_y;
	unsigned int m_lastingTime;
	int m_scope_size;
	float m_rot;
	float m_rot_speed;
};

class CGndRiser : public CAbleToMakeEffect {
public:
	CGndRiser();
	virtual ~CGndRiser();
	virtual void Render(matrix& vtm) {}
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
	void OnInit();
	void OnExit();
private:
	int m_lastCnt;
	int m_sx;
	int m_sy;
};

class CEffectMaker : public CAbleToMakeEffect {
public:
	CEffectMaker();
	virtual ~CEffectMaker();
	virtual void Render(matrix& vtm) {}
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
	void OnInit();
	void OnExit();
private:
	unsigned int m_processCnt;
	unsigned int m_emitSpeed;
	int m_type;
	float m_param[4];
};

class CSoundMaker : public CRenderObject {
public:
	CSoundMaker();
	virtual ~CSoundMaker();
	virtual void Render(matrix& vtm) {}
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
	void OnInit();
	void OnExit();
private:
	std::string m_waveName;
	float m_vol;
	int m_width;
	int m_height;
	float m_range;
	float m_diagonal;
	unsigned int m_lastPlayTime;
	unsigned int m_cycle;
};

class CGameActor : public CAbleToMakeEffect {
public:
	CGameActor();
	virtual ~CGameActor();
	void OnInit();
	void OnExit();
	virtual bool OnProcess() = 0;
	virtual bool ProcessState();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1 = 0, INT_PTR arg2 = 0, INT_PTR p = 0);
	virtual void SetRenderInfo(RENDER_INFO_RECT& info, const float tlvertX, const float tlvertY);
	virtual void SetState(int job);
	virtual void RegisterPos();
	virtual void UnRegisterPos();
	virtual void SetAction(int action, int speed, int type);
	virtual int Get8Dir(float rot);

	virtual void SetSprAct(int job, int sex, int head, int weapon, int accessory, int accessory2, int accessory3, int shield, int headPal, int bodyPal) {}
	virtual void SetSprAct2(int job, int sex, int head, int weapon, int accessory, int accessory2, int accessory3, int shield, int headPal, int bodyPal, int effectState) {}
	virtual void SetSprAct(int job, int sex) {}
	virtual void SetSprJob(int job) {}
	virtual void SetSprHead(int head) {}
	virtual void SetSprJobDye(int job) {}
	virtual void SetSprWeapon(int weapon) {}
	virtual void SetSprWeapon2(int weapon) {}
	virtual void SetSprAccessory(int accessory) {}
	virtual void SetSprAccessory2(int accessory) {}
	virtual void SetSprAccessory3(int accessory) {}
	virtual void SetSprShield(int shield) {}
	virtual void SetSprShoe(int shoe) {}
	virtual void SetImfFileName() {}
	virtual void SetHeadPaletteName() {}
	virtual void SetBodyPaletteName() {}
	virtual void SetBodyPalette(int bodyPalette) {}
	virtual void SetHeadPalette(int headPalette) {}
	virtual int GetWeapon() { return -1; }
	virtual void ProcessMotionWithDist();
	virtual int GetAttackMotion();
	virtual void MakeCorpse();
	virtual void SetModifyFactorOfmotionSpeed(int job);
	virtual void SetHonor(int honor) {}
	virtual void SetPropensityInfo(int honor, int virtue) {}
	bool FindPath(int mssx, int mssy, int mscellX, int mscellY, int msdx, int msdy, int msspeed);
	void ProcessWillBeAttacked();
	void MakeSpecialAttackEffect(unsigned int targetGid, float hideTime);
	void CalcAccel(float speed);
	vector3d CalcAccel3d(vector3d __$ReturnUdt, float to);
	void ProcessActionWithDir();
	void AddDist(vector3d& lastPos, vector3d& pos);
	unsigned int GetGID() { return m_gid; }
	void ProcessChatBalloon();
	void DeleteChatBalloon();
	void SetJob(int job);
	int GetJob();
	int IsThisPC(int job);
	void SetSex(int job);
	int GetSex() { return m_sex; }
	UIChatRoomTitle* GetChatRoomTitle() { return m_chatTitle; }
	void SetPKState(int job);
	void SetAttrState(int bodyState, int effectState, int healthState, bool changedNow = false);
	void ProcessEffectState();
	void DeleteMatchingEffect(CMsgEffect* msgEffect);
	bool ProcessVanish();
	void ProcessBodyState();
	void SetVanish();
	CMsgEffect* GetBird() { return m_birdEffect; }
	int GetProperJob(int job, int effectState);
	int GetBodyState() { return m_bodyState; }
	int GetEffectState() { return m_effectState; }
	int GetHealthState() { return m_healthState; }
	void SetEffectState(int effectState) { m_effectState = effectState; }
	void SetHeadType(int headType) { m_headType = headType; }
	int GetHeadType() { return m_headType; }
	int GetActorType() { return m_actorType; }
	void Am_Skill(CGameObject* sender, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
	void Am_Action(CGameObject* sender, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
	void Am_Groundskill(CGameObject* sender, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
	void Am_Make_Number(CGameObject* sender, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
	void MakePartyEffect(int job);
	void DeleteTotalNumber();
	void AddMsgEffect(CMsgEffect* msgEffect);
	int IsDarkEffect(int job);
	void Delete99LevelEffects();
	virtual void SetGuildInfo(int job, int sex);
	virtual int GetGdid() { return m_gdid; }
	virtual int GetEmblemVersion() { return m_emblemVersion; }
	void DeleteEmblemWnd();
	void MakeEmblemWnd();
	void DeleteWordDisplayWnd();
	void MakeWordDisplayWnd();
	int IsFreezeState();
	void SetGroupMemberAndVisible(int job);
	int GetGroupMemberAndVisible() { return m_bIsMemberAndVisible; }
	void SetFreezeEndTick(unsigned int freezeEndTick) { m_freezeEndTick = freezeEndTick; }
	int GetTargetGid() { return m_targetGid; }
	virtual _MSG2AI& GetMsg2AI(int merType);
	virtual void SetMsg2AI(_MSG2AI& msg, int merType);
	virtual _MSG2AI& GetResMsg2AI(int merType);
	virtual void SetResMsg2AI(_MSG2AI& msg, int merType);
	bool HomunAIIsOpened() { return m_homunAI && m_homunAI->IsOpened(); }
	bool MerAIIsOpened() { return m_merAI && m_merAI->IsOpened(); }
	int m_moveDestX;
	int m_moveDestY;
	unsigned int m_speed;
	int m_isCounter;
	int m_isTrickDead;
	int m_isPlayHitWave;
	int m_isAsuraAttack;
	char* m_emblemWnd;
	char* m_WordDisplayWnd;
	char m_hitWaveName[128];
	unsigned int m_colorEndTick;
	short m_clevel;
	short m_MaxHp;
	short m_Hp;
	short m_MaxSp;
	short m_Sp;
	int m_Exp;
	short m_Str;
	short m_Int;
	short m_Dex;
	short m_Vit;
	short m_Luk;
	short m_Agi;
protected:
	vector3d m_accel;
	CPathInfo m_path;
	unsigned int m_moveStartTime;
	unsigned char m_isNeverAnimation;
	int m_pathStartCell;
	float m_dist;
	unsigned int m_lastProcessStateTime;
	unsigned int m_lastServerTime;
	unsigned int m_chatTick;
	unsigned int m_targetGid;
	float m_attackMotion;
	int m_isBladeStop;
	unsigned int m_gid;
	int m_job;
	int m_sex;
	UIBalloonText* m_balloon;
	UIChatRoomTitle* m_chatTitle;
	UIMerchantShopTitle* m_merchantShopTitle;
	UIRechargeGage* m_skillRechargeGage;
	unsigned int m_freezeEndTick;
	unsigned int m_petEmotionStartTick;
	unsigned int m_skillRechargeEndTick;
	unsigned int m_skillRechargeStartTick;
	int m_chatWidth;
	int m_chatHeight;
	int m_nameWidth;
	int m_xSize;
	int m_ySize;
	int m_headType;
	std::list<WBA> m_willBeAttackList;
	std::list<WBA> m_willBeAttackedList;
	int m_willBeDead;
	int m_is99;
	char m_99;
	int m_bodyState;
	int m_effectState;
	int m_healthState;
	int m_pkState;
	float m_damageDestX;
	float m_damageDestZ;
	unsigned int m_effectLaunchCnt;
	unsigned int m_vanishTime;
	int m_actorType;
	int m_bIsMemberAndVisible;
	int m_gdid;
	int m_emblemVersion;
public:
	CMercenaryAI* m_homunAI;
	CMercenaryAI* m_merAI;
	unsigned char m_objectType;
	_MSG2AI m_homunMsg;
	_MSG2AI m_homunResMsg;
	_MSG2AI m_merMsg;
	_MSG2AI m_merResMsg;
private:
	CMsgEffect* m_birdEffect;
	CMsgEffect* m_cartEffect;
	CMsgEffect* m_pkEffect;
	CMsgEffect* m_iceEffect;
	CMsgEffect* m_curseEffect;
	CMsgEffect* m_sleepEffect;
	CMsgEffect* m_stunEffect;
	CMsgEffect* m_silenceEffect;
	CMsgEffect* m_angelusEffect;
	CMsgEffect* m_crusisEffect;
	CMsgEffect* m_chatProhibitEffect;
	CMsgEffect* m_cursorEffect;
	std::list<CMsgEffect *> m_msgEffectList;
#if defined(BF_PORT)
	friend struct BfClientGame;   // port/client/src/bf_client_game.cpp reads the emote balloons (m_msgEffectList)
#endif
};
const int	PC_LAYER_SIZE = 4;

class CPc : public CGameActor {
public:
	CPc();
	virtual ~CPc();
	void OnInit();
	void OnExit();
	virtual bool OnProcess();
	virtual bool ProcessState();
	virtual void SetState(int job);
	virtual void Render(matrix& vtm);
	virtual void SetSprAct(int job, int sex, int head, int weapon, int accessory, int accessory2, int accessory3, int shield, int headPal, int bodyPal);
	virtual void SetSprAct2(int job, int sex, int head, int weapon, int accessory, int accessory2, int accessory3, int shield, int headPal, int bodyPal, int effectState);
	virtual void SetSprJob(int job);
	virtual void SetSprHead(int job);
	virtual void SetSprJobDye(int job);
	virtual void SetSprWeapon(int job);
	virtual void SetSprWeapon2(int job);
	virtual void SetSprAccessory(int job);
	virtual void SetSprAccessory2(int job);
	virtual void SetSprAccessory3(int job);
	virtual void SetSprShield(int job);
	virtual void SetSprShoe(int job);
	virtual void SetImfFileName();
	virtual void SetHeadPaletteName();
	virtual void SetBodyPaletteName();
	virtual int GetWeapon() { return m_weapon; }
	virtual void SetModifyFactorOfmotionSpeed(int job);
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
	virtual void SetBodyPalette(int job);
	virtual void SetHeadPalette(int job);
	virtual void SetHonor(int job);
	virtual void SetPropensityInfo(int honor, int virtue);
	int GetPKRank() { return m_pk_rank; }
	int GetPKTotal() { return m_pk_total; }
	void SetPKRank(int rank) { m_pk_rank = rank; }
	void SetPKTotal(int total) { m_pk_total = total; }
	void RenderBody2(matrix& vtm, int changed);
	void RenderBodyLayer(matrix& vtm, int changed);
	void RenderShadowLayer(matrix& vtm);
	void ProcessGage();
	void SetRank(int honor, int virtue);
	const std::vector<std::string> GetSprNames();
	void SetSprNames(const std::vector<std::string>& sprNames);
	const std::vector<std::string> GetActNames();
	void SetActNames(const std::vector<std::string>& sprNames);
	const std::string& GetImfNames() { return m_imfName; }
	int GetHeadPalette() { return m_headPalette; }
	std::string GetHeadPaletteName() { return m_headPaletteName; }
	void RenderShoe(int job);
	int GetHp();
	int GetMaxHp();
protected:
	void SetSprNameList(int layer, const char* sprName);
	const char* GetSprNameList(int layer);
	CSprRes* GetSprResList(int layer);
	CSprRes* GetIllusionSprResList();
	void SetActNameList(int layer, const char* sprName);
	const char* GetActNameList(int layer);
	CActRes* GetActResList(int layer);
	CActRes* GetIllusionActResList();
	int MakeWeaponType(int left, int right);
protected:
	std::string m_imfName;
	int m_honor;
	int m_virtue;
	int m_headDir;
	int m_head;
	int m_headPalette;
	int m_weapon;
	int m_accessory;
	int m_accessory2;
	int m_accessory3;
	int m_shield;
	int m_shoe;
	int m_shoe_count;
	vector3d shoe_pos;
	int m_renderWithoutLayer;
	std::string m_headPaletteName;
	UIPcGage* m_gage;
	int m_pk_rank;
	int m_pk_total;
private:
	std::vector<CSprRes *> m_sprResList;
	std::vector<CActRes *> m_actResList;
};

class CGrannyPc : public CGameActor {
public:
	CGrannyPc();
	virtual ~CGrannyPc();
	void OnInit();
	void OnExit();
	void FreeRes();
	virtual bool OnProcess();
	virtual bool ProcessState();
	virtual void SetState(int weapon);
	virtual void Render(matrix& vtm);
	virtual void SetSprAct(int job, int sex, int head, int weapon, int accessory, int accessory2, int accessory3, int shield, int headPal, int bodyPal);
	virtual void SetSprAct2(int job, int sex, int head, int weapon, int accessory, int accessory2, int accessory3, int shield, int headPal, int bodyPal, int effectState);
	virtual void SetSprJob(int weapon);
	virtual void SetSprHead(int weapon);
	virtual void SetSprJobDye(int weapon);
	virtual void SetSprWeapon(int weapon);
	virtual void SetSprWeapon2(int weapon) {}
	virtual void SetSprAccessory(int weapon);
	virtual void SetSprAccessory2(int weapon);
	virtual void SetSprAccessory3(int weapon);
	virtual void SetSprShield(int weapon);
	virtual void SetImfFileName();
	virtual void SetHeadPaletteName();
	virtual int GetWeapon() { return m_weapon; }
	virtual void SetModifyFactorOfmotionSpeed(int weapon);
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
	virtual void SetHonor(int weapon);
	virtual void SetPropensityInfo(int honor, int virtue);
	virtual void ProcessMotion();
	virtual void SetAction(int action, int speed, int type);
	int GetPKRank();
	int GetPKTotal();
	void SetPKRank(int weapon);
	void SetPKTotal(int weapon);
	void ProcessGage();
	void SetRank(int honor, int virtue);
	const std::vector<std::string> GetGr2Names();
	void SetGr2Names(const std::vector<std::string>& sprNames);
	const std::vector<std::string> GetActNames();
	void SetActNames(const std::vector<std::string>& sprNames);
	const std::string& GetImfNames();
	int GetHeadPalette();
	std::string GetHeadPaletteName();
protected:
	granny_world_pose* ProcessAni(C3dGrannyModelRes* pRes);
	void SetGrannyAction(int weapon);
	void RenderAndAni(matrix& vtm, GrannyLightInfo* grannyLInfo, C3dGrannyModelRes* pModelRes, int nPartMeshIdx, C3dGrannyModelRes* pAniModelRes);
	void RenderBak(matrix& vtm, int nPartMeshIdx, C3dGrannyModelRes* pModelRes);
	void RenderShadow(matrix& vtm, vector3d lv, bool isBakRender);
protected:
	std::string m_imfName;
	int m_honor;
	int m_virtue;
	int m_headDir;
	int m_head;
	int m_headPalette;
	int m_weapon;
	int m_accessory;
	int m_accessory2;
	int m_accessory3;
	int m_shield;
	int m_shoe;
	int m_renderWithoutLayer;
	std::string m_headPaletteName;
	UIPcGage* m_gage;
	int m_pk_rank;
	int m_pk_total;
	C3dGrannyModelRes* m_GrannyActorRes;
	C3dGrannyModelRes* m_GrannyPartRes[10];
	granny_model_instance* m_Instance;
	RPMesh* m_rp[30];
	RPMesh* m_rpPart[10][30];
	granny_pnt332_vertex* m_matVer[30];
	granny_pnt332_vertex* m_matVerPart[10][30];
	granny_system_clock m_fAniCnt;
	granny_system_clock m_fLastAniCnt;
	float m_GameClock;
	int m_curAction;
	int m_baseAction;
	float m_fCurRot;
	unsigned char m_RenderAlpha;
	unsigned int m_nVertCol;
	int m_nLastActAnimation;
	int m_curFrame;
	char m_strJobFn[260];
	int m_nRenderType;
	CTexture* m_pCellTex;
	granny_control* m_Control[20];
	unsigned char m_isFirstProcess;
	int m_nUpdateAniFlag;
	CTexture* m_pTex[10][30];
	void* m_pFaceArr[10][30];
	int m_nIndexNo[10][30];
	matrix m_matPose[30];
	char m_strJobSymbol[3];
	char m_strPartSymbol[8][30];
	char m_strBoneSymbol[3];
private:
	granny_world_pose* m_pWorldPose;
	CTexture* m_shadowTex;
	std::vector<vector3d> m_shadowDotList;
	HairInfo m_hair;
};

enum PROCEEDTYPE { PT_NOTHING, PT_ATTACK, PT_PICKUPITEM, PT_SKILL, PT_GROUNDSKILL, PT_ATTACK_2, PT_TOUCH_SKILL, };

class CPlayer : public CPc {
public:
	CPlayer();
	virtual ~CPlayer();
	void OnInit();
	void OnExit();
	virtual bool OnProcess();
	virtual bool ProcessState();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
	virtual void SetState(int stateId);
	virtual void RegisterPos() {}
	virtual void UnRegisterPos() {}
	void ProcessProceed();
	void ProcessPreengageMove();
	void ReLaunchBlurEffects();
	int m_destCellX;
	int m_destCellZ;
	unsigned int m_attackReqTime;
	unsigned int m_preMoveStartTick;
	PROCEEDTYPE m_proceedType;
	int m_preMoveOn;
	int m_attackMode;
	int m_isAttackRequest;
	int m_isWaitingMoveAck;
	int m_isPreengageStateOfMove;
	unsigned int m_proceedTargetGid;
	unsigned int m_totalAttackReqCnt;
	unsigned int m_tickOfMoveForAttack;
	unsigned int m_moveReqTick;
	unsigned int m_standTick;
	int m_skillId;
	int m_skillAttackRange;
	int m_skillUseLevel;
	int m_gSkillDx;
	int m_gSkillDy;
	int m_preengageXOfMove;
	int m_preengageYOfMove;
	CRagEffect* m_statusEffect;
};

class CNpc : public CGameActor {
public:
	CNpc();
	virtual ~CNpc();
	void OnInit();
	void OnExit();
	void ProcessContact();
	void ProcessLaunchEffect();
	virtual void Render(matrix& vtm);
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
	virtual void SetState(int head);
	virtual void RegisterPos();
	virtual void UnRegisterPos();
	virtual void SetSprAct(int job, int sex);
	virtual void SetSprJob(int head);
	virtual void SetSprHead(int head) {}
	virtual void SetSprJobDye(int head);
	virtual void SetSprWeapon(int weapon) {}
	virtual void SetSprAccessory(int accessory) {}
	virtual void SetImfFileName() {}
	void ProcessGage();
	float m_x1;
	float m_z1;
	float m_x2;
	float m_z2;
	int m_playerIswithinBox;
	unsigned int m_lastEffectLaunchTick;
	int m_doesLaunchEffect;
	UIPlayerGage* m_gage;
	UIPcGage* m_gage2;
};

class CMousePointer : public CRenderObject {
public:
	CMousePointer();
	virtual ~CMousePointer();
	void OnInit();
	void OnExit();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
	virtual bool OnProcess();
	int m_visibleTime;
};

class CBlowEffect : public CRenderObject {
public:
	CBlowEffect();
	virtual ~CBlowEffect();
	void OnInit();
	void OnExit();
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
};

class CCorpse : public CRenderObject {
public:
	CCorpse();
	virtual ~CCorpse();
	void OnInit();
	void OnExit();
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
};

class CBlurPC : public CPc {
public:
	CBlurPC(CPc* pc);
	virtual ~CBlurPC();
	void OnInit();
	void OnExit();
	virtual bool OnProcess();
	virtual void Render(matrix& ptm);
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
protected:
	float m_motionY;
	unsigned int m_alphaDelta;
	unsigned int m_r;
	unsigned int m_g;
	unsigned int m_b;
};

class CItem : public CRenderObject {
public:
	CItem();
	virtual ~CItem();
	void OnInit();
	void OnExit();
	virtual bool OnProcess();
	virtual void Render(matrix& vtm);
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
	virtual void SetRenderInfo(RENDER_INFO_RECT& info, const float tlvertX, const float tlvertY);
	unsigned int GetAID() { return m_aid; }
	std::string m_itemName;
#if defined(BF_PORT)
	int m_bfItemId;   // the entry's ITID, which m_itemName no longer holds (the control socket's `actors`)
#endif
protected:
	unsigned int m_aid;
	int m_isJumping;
	float m_sfallingSpeed;
	float m_sPosY;
};

class CSkill : public CGameActor {
public:
	CSkill();
	virtual ~CSkill();
	void OnInit();
	void OnExit();
	virtual void Render(matrix& vtm);
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1 = 0, INT_PTR arg2 = 0, INT_PTR arg3 = 0);
	virtual void SetRenderInfo(RENDER_INFO_RECT& info, const float tlvertX, const float tlvertY);
	unsigned int GetAID() { return m_aid; }
protected:
	char* GetTrapModelName();
	void ShowModel(char* modelName);
	void HideModel();
	void SetAID(unsigned int aid) { m_aid = aid; }
private:
	unsigned int m_launchCnt;
	unsigned int m_SkillStartTime;
	unsigned int m_aid;
	C3dActor* m_3dactor;
	CRagEffect* m_LoopEffect;
	int m_effectId;
};

class CGraffiSkill : public CSkill {
public:
	CGraffiSkill();
	virtual ~CGraffiSkill();
	virtual void OnInit();
	virtual void OnExit();
	virtual void Render(matrix& vtm);
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
private:
	int m_master_gid;
	int m_ground_pos_x;
	int m_ground_pos_y;
	int m_lastingTime;
	CTexture* m_tex;
	CTexture* m_fontTex;
	char m_strMsg[256];
	int m_offsX;
	int m_offsY;
	float m_rot;
};

class CArrowEffect : public CRenderObject {
public:
	CArrowEffect();
	virtual ~CArrowEffect();
	void OnInit();
	void OnExit();
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3, INT_PTR arg4);
protected:
	vector3d m_sPos;
	vector3d m_moveSpeed;
	vector3d m_tPos;
	float m_fastFactor;
	float m_hideTime;
	unsigned int m_targetGid;
	int m_process;
};

class CMandraAttackEffect : public CRenderObject {
public:
	CMandraAttackEffect();
	virtual ~CMandraAttackEffect();
	void OnInit();
	void OnExit();
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
protected:
	float m_hideTime;
};

class CMasterChaser : public CRenderObject {
public:
	CMasterChaser();
	virtual ~CMasterChaser();
	void OnInit();
	void OnExit();
protected:
	CGameActor* m_pursuedMaster;
};

class CNumEffect : public CRenderObject {
public:
	CNumEffect();
	virtual ~CNumEffect();
	void OnInit();
	void OnExit();
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);
protected:
	float m_orgPosY;
	float m_orgZoom;
};

class CMsgEffect : public CAbleToMakeEffect {
public:
	CMsgEffect();
	virtual ~CMsgEffect();
	void OnInit();
	void OnExit();
	virtual bool OnProcess();
	virtual void SendMsg(CGameObject* sender, int message, INT_PTR arg1 = 0, INT_PTR arg2 = 0, INT_PTR arg3 = 0);
	void ProcessPosition();
	int GetMsgEffectType() { return m_msgEffectType; }
protected:
	CGameActor* m_masterActor;
	vector3d m_destPos;
	vector3d m_targetPos;
	vector3d m_destPos2;
	unsigned int m_masterGid;
	int m_isDisappear;
	float m_dist;
	float m_orgPosY;
	float m_orgPosX;
	float m_orgZoom;
	float m_moveDist;
	int m_stateCnt;
	int m_oldAction;
	int m_msgEffectType;
	int m_orgSprShift;
	int m_isAttackTarget;
	int m_isAnimation;
};

struct RENDER_INFO_RECT {
public:
	void SetInfo(float fleft, float ftop, float fright, float fbottom, float foow);
	void Update(const RPSprite& spr);
	float left;
	float top;
	float right;
	float bottom;
	float oow;
};

#endif
