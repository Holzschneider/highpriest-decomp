#ifndef HIGHPRIEST_UIGUILDWND_H
#define HIGHPRIEST_UIGUILDWND_H

#include "GuildInfo.h"
#include "SkillInfo.h"
#include "window/UIFrameWnd.h"

class UIGuildBanReasonDescWnd;
class UIGuildBanishedMemberWnd;
class UIGuildInfoManageWnd;
class UIGuildLeaveReasonDescWnd;
class UIGuildMemberManageWnd;
class UIGuildNoticeWnd;
class UIGuildPositionManageWnd;
class UIGuildSkillWnd;
class UIGuildTotalInfoWnd;
class UIGuildWnd;

class UIGuildWnd : public UIFrameWnd {
public:
	UIGuildWnd()
	{
		m_info = 0;
		for (int i = 0; i < 6; ++i)
			m_tabText[i] = 0;
	}
	virtual ~UIGuildWnd() {}
	virtual void OnCreate(int x, int y) = 0;
	virtual void OnLBtnDown(int x, int y);
	virtual void OnDraw();
	virtual void StoreInfo();
	virtual int GetTab() = 0;
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	void DrawTab(int curTab);
	void InitTabText();
protected:
	GuildWndInfo* m_info;
	UIStaticText* m_tabText[6];
};

class UIGuildInfoManageWnd : public UIGuildWnd {
public:
	UIGuildInfoManageWnd();
	virtual ~UIGuildInfoManageWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnRBtnUp(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual int GetTab() { return 0; }
	void DeleteCompressedEmblemBitmapFileImage();
	void RefreshEmblemList();
	void SetEmblemResName();
	void RefreshCompressedEmblemFileImage(const char* emblemBitmapFileName);
protected:
	UIBitmapButton* m_emblemChangeButton;
	std::list<std::string> m_emblemList;
	std::vector<StringId> m_menuItems;
	std::string m_emblemResName;
	char* m_compressedEmblemBitmapFileImage;
	unsigned int m_compressedEmblemBitmapFileImageSize;
	int m_gdid;
	int m_emblemVersion;
	int m_curAllyGuild;
	int m_curHostileGuild;
};

class UIGuildMemberManageWnd : public UIGuildWnd {
public:
	UIGuildMemberManageWnd();
	virtual ~UIGuildMemberManageWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual int GetTab() { return 1; }
	virtual void OnWheel(int wheel);
	virtual void OnRBtnUp(int cx, int cy);
	void RecalcScrbarPos();
	bool GetMemberInfo(int curItem, GUILD_MEMBER_INFORMATION* guildMemberInfo);
	int GetCurItem(int x, int y);
protected:
	UIStaticText* m_memo[5];
	UIStaticText* m_memberName[5];
	UIStaticText* m_positionName[5];
	UIStaticText* m_jobName[5];
	UIBitmapButton* m_posComboButton[5];
	UIScrollBar* m_scrollBar;
	UIBitmapButton* m_applyButton;
	std::vector<StringId> m_menuItems;
	int m_viewOffset;
	int m_curItem;
	int m_itemHorzNum;
	int m_itemVertNum;
	int m_posComboSeq;
	int m_vertScrEnabled;
	std::list<GUILD_MEMBER_INFORMATION> m_memberInfoList;
	std::map<int, MEMBER_POSITION_INFORMATION> m_memberPositionInfoListForChange;
};

class UIGuildPositionManageWnd : public UIGuildWnd {
public:
	UIGuildPositionManageWnd();
	virtual ~UIGuildPositionManageWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual void Invalidate();
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual int GetTab() { return 2; }
	virtual void OnWheel(int wheel);
	void RecalcScrbarPos();
	int GetCurItem(int x, int y);
	GUILD_POSITION_INFORMATION& GetPosInfo(int curItem, int& isValid);
protected:
	UICheckBox* m_joinCheckBox[11];
	UICheckBox* m_punishCheckBox[11];
	UIEditCtrlVarId* m_payRateEdit[11];
	UIEditCtrlVarId* m_posNameEdit[11];
	UIStaticText* m_payRate[11];
	UIStaticText* m_posName[11];
	UIScrollBar* m_scrollBar;
	UIBitmapButton* m_applyButton;
	int m_viewOffset;
	int m_curItem;
	int m_itemHorzNum;
	int m_itemVertNum;
	int m_vertScrEnabled;
	std::list<GUILD_POSITION_INFORMATION> m_posInfoList;
	std::map<int, GUILD_POSITION_INFORMATION> m_changedPositionInfoList;
};

class UIGuildSkillWnd : public UIGuildWnd {
public:
	UIGuildSkillWnd();
	virtual ~UIGuildSkillWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnUp(int cx, int cy);
	virtual void OnWheel(int wheel);
	virtual int GetTab() { return 3; }
	int GetCurItem(int x, int y);
	void RefreshButton();
	void RecalcScrbarPos();
	SKILL_INFO GetSkillInfo(int __$ReturnUdt);
	SKILL_INFO GetSkillInfoByViewCnt(int __$ReturnUdt);
protected:
	UIResizer* m_resizer;
	UIScrollBar* m_scrollBar;
	UIBitmapButton* m_button[2];
	UIBitmapButton* m_lvlSelectUpButton[7];
	UIBitmapButton* m_lvlSelectDnButton[7];
	UIBitmapButton* m_incArrowButton[7];
	int m_curItem;
	int m_viewOffset;
	int m_itemHorzNum;
	int m_itemVertNum;
	int m_vertScrEnabled;
	std::list<SKILL_INFO> m_itemList;
	std::vector<StringId> m_menuItems;
};

class UIGuildBanishedMemberWnd : public UIGuildWnd {
public:
	UIGuildBanishedMemberWnd();
	virtual ~UIGuildBanishedMemberWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual int GetTab() { return 4; }
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnWheel(int wheel);
	void RecalcScrbarPos();
	int GetCurItem(int x, int y);
	SGUILD_BANISH_INFO GetBanishInfo(int __$ReturnUdt, int& curItem);
protected:
	UIStaticText* m_memberName[11];
	UIStaticText* m_reason[11];
	UIScrollBar* m_scrollBar;
	int m_viewOffset;
	int m_curItem;
	int m_itemHorzNum;
	int m_itemVertNum;
	int m_vertScrEnabled;
};

class UIGuildNoticeWnd : public UIGuildWnd {
public:
	UIGuildNoticeWnd();
	virtual ~UIGuildNoticeWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual void OnLBtnDown(int cx, int cy);
	virtual int GetTab() { return 5; }
protected:
	UIEditCtrlVarId* m_noticeTitleEdit;
	UIEditCtrlVarId* m_noticeEdit[9];
	UIBitmapButton* m_sendButton;
	int m_noticeTitleEdited;
	int m_noticeEdited;
};

class UIGuildTotalInfoWnd : public UIGuildWnd {
public:
	UIGuildTotalInfoWnd();
	virtual ~UIGuildTotalInfoWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnWheel(int wheel);
	virtual int GetTab() { return 6; }
	void RecalcScrbarPos();
	int GetCurItem(int x, int y);
	SGUILD_INFO GetBanishInfo(int __$ReturnUdt, int& curItem);
protected:
	UIStaticText* m_guildName[12];
	UIScrollBar* m_scrollBar;
	int m_viewOffset;
	int m_curItem;
	int m_itemHorzNum;
	int m_itemVertNum;
	int m_vertScrEnabled;
};

class UIGuildLeaveReasonDescWnd : public UIFrameWnd {
public:
	UIGuildLeaveReasonDescWnd();
	virtual ~UIGuildLeaveReasonDescWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw() { DrawLatticeFrame(0, 0, 0, 0); }
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
protected:
	UIEditCtrl* m_editCtrl;
	UIStaticText* m_titleText;
};

class UIGuildBanReasonDescWnd : public UIGuildLeaveReasonDescWnd {
public:
	UIGuildBanReasonDescWnd();
	virtual ~UIGuildBanReasonDescWnd();
	virtual void OnCreate(int cx, int cy);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
protected:
	int m_vars[3];
};

#endif
