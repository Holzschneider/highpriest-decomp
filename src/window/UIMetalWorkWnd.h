#ifndef HIGHPRIEST_WINDOW_UIMETALWORKWND_H
#define HIGHPRIEST_WINDOW_UIMETALWORKWND_H

#include "../ItemInfo.h"
#include "UIFrameWnd.h"

struct DRAG_INFO;
class UIItemRepairWnd;
class UIMakeTargetListWnd;
class UIMakeTargetProcessWnd;
class UIMakeTargetResultWnd;
class UIWeaponRefineWnd;

class UIItemRepairWnd : public UIFrameWnd {
public:
	UIItemRepairWnd();
	virtual ~UIItemRepairWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
protected:
	UIListBox* m_metalList;
	std::vector<int> m_metalListId;
	std::vector<int> m_metalListIndex;
	std::list<REPAIRITEM_INFO> m_repairItemList;
};

class UIWeaponRefineWnd : public UIFrameWnd {
public:
	UIWeaponRefineWnd();
	virtual ~UIWeaponRefineWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
protected:
	UIListBox* m_metalList;
	std::vector<int> m_metalListId;
	std::vector<int> m_metalListIndex;
};

class UIMakeTargetProcessWnd : public UIFrameWnd {
public:
	UIMakeTargetProcessWnd();
	virtual ~UIMakeTargetProcessWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnDraw();
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
protected:
	ITEM_INFO GetItemInfo(int __$ReturnUdt, int x);
	bool IsWeaponTargetItem();
protected:
	int m_targetItem;
	ITEM_INFO m_propertyiteminfo[3];
};

class UIMakeTargetListWnd : public UIFrameWnd {
public:
	UIMakeTargetListWnd();
	virtual ~UIMakeTargetListWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
protected:
	UIListBox* m_metalList;
	std::vector<int> m_metalListId;
};

class UIMakeTargetResultWnd : public UIFrameWnd {
public:
	UIMakeTargetResultWnd();
	virtual ~UIMakeTargetResultWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
protected:
	int m_targetItem;
	unsigned char m_succeed;
};

#endif
