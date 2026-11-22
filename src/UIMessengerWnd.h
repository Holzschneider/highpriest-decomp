#ifndef HIGHPRIEST_UIMESSENGERWND_H
#define HIGHPRIEST_UIMESSENGERWND_H

#include "window/UIFrameWnd.h"

class UIFriendOptionWnd;
class UIMessengerGroupWnd;

class UIMessengerGroupWnd : public UIFrameWnd {
public:
	UIMessengerGroupWnd();
	virtual ~UIMessengerGroupWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual void OnRBtnUp(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnWheel(int wheel);
	virtual void StoreInfo();
	FRIEND_INFO GetItemInfo(int x, int y);
	FRIEND_INFO GetItemInfoBySequence(int sequence);
	void RecalcScrbarPos();
protected:
	void ResetGageListPos();
protected:
	MessengerGroupWndInfo* m_info;
	UIScrollBar* m_scrollBar;
	UIRadioBtn* m_radioBtns[2];
	UIBitmapButton* m_sysButton;
	UIBitmapButton* m_button[5];
	UIResizer* m_resizer;
	UIStaticText* m_titleText;
	std::list<FRIEND_INFO> m_itemList;
	std::vector<StringId> m_menuItems;
	int m_vertScrEnabled;
	int m_viewOffset;
	int m_itemHorzNum;
	int m_itemVertNum;
	int m_curItem;
	int m_itemHeight;
	UIPcGage* m_gageList[40];
public:
	int m_curRadioBtn;
};

class UIFriendOptionWnd : public UIFrameWnd {
public:
	UIFriendOptionWnd();
	virtual ~UIFriendOptionWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual void StoreInfo();
protected:
	FriendOptionWndInfo* m_info;
	UICheckBox* m_playWhisperOpenSoundCheckBox;
	UICheckBox* m_showFriendWhisperWndCheckBox;
	UICheckBox* m_showWhisperWndCheckBox;
	UIBitmapButton* m_sysButton;
};

#endif
