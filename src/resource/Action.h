#ifndef HIGHPRIEST_RESOURCE_ACTION_H
#define HIGHPRIEST_RESOURCE_ACTION_H

#include "../common.h"
#include "../base/Res.h"

struct CMotion;

struct CSprClip {
	int x, y;
	int sprIndex;
	int flags;
	BYTE r, g, b, a;
	float zoomx;
	float zoomy;
	int angle;
	int clipType;
};

struct CAttachPointInfo {
	int x;
	int y;
	int m_attr;
};

struct CMotion {
public:
	void Create(unsigned int clip_count);
	CSprClip* GetClip(unsigned int clip_number);
	tagRECT range1;
	tagRECT range2;
	std::vector<CSprClip> sprClips;
	int numClips;
	int m_eventId;
	std::vector<CAttachPointInfo> attachInfo;
	int attachCnt;
};

struct CAction {
public:
	void Create(unsigned int motion_count);
	CMotion* GetMotion(unsigned int motion_number);
	std::vector<CMotion> motions;
};

class CActRes : public CRes {
public:
	DECLARE_RESOURCE(CActRes)
	CActRes();
	virtual bool Load(const char* fName);
	virtual void Reset();
	void Create(unsigned int action_count);
	CAction* GetAction(unsigned int action_number);
	CMotion* GetMotion(unsigned int actIndex, unsigned int motIndex);
	int GetMotionCount(unsigned int actIndex);
	int GetActionCount() { return actions.size(); }
	float GetDelay(unsigned int actIndex);
	const char* GetEventName(int eventId);
	void ReCalcClipXY(CSprClip* clip, int cx, int cy, int isHalfW, int isHalfH);
	int GetNumMaxClipPerMotion() { return numMaxClipPerMotion; }
private:
	std::vector<CAction> actions;
	int numMaxClipPerMotion;
	std::vector<std::string> m_events;
	std::vector<float> m_delay;
};

class CRmaRes : public CRes {
public:
	struct FrameNum {
	public:
		FrameNum() : startFrame(-1), endFrame(-1), motionSpeed(4) {}
		int startFrame;
		int endFrame;
		int motionSpeed;
		std::vector<int> attackFrameList;
	};

public:
	DECLARE_RESOURCE(CRmaRes)
	CRmaRes();
	virtual ~CRmaRes();
	virtual bool Load(const char* fName);
	virtual void Reset();
	std::string& GetRsxName(int action);
	int IsValidAction(int action);
	int GetMotionSpeed(int action);
	int GetStartFrame(int action);
	int GetEndFrame(int action);
	int IsAttackFrame(int action, int frame);
	int GetFirstAttackFrame(int action);
	std::vector<FrameNum> m_actionList;
	std::vector<std::string> m_rsxNameList;
	std::vector<int> m_rsxNameIndexList;
	float m_interiorZoom;
	float m_shadowZoom;
	int m_interiorOffsetX;
	int m_interiorOffsetY;
	int m_interiorStartFrame;
	int m_interiorEndFrame;
};

#endif
