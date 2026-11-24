#ifndef HIGHPRIEST_WINDOW_UIIMEWND_H
#define HIGHPRIEST_WINDOW_UIIMEWND_H

#include "UIFrameWnd.h"

class UICandidateWnd;
class UICompositionWnd;
class UIKeyStrokeWnd;

class UICandidateWnd : public UIFrameWnd {
public:
	UICandidateWnd();
	virtual ~UICandidateWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	std::string m_candidateList[9];
	int m_selection;
	int m_num_curPage;
	int m_num_totalPage;
	int m_mun_item;
};

class UICompositionWnd : public UIFrameWnd {
public:
	UICompositionWnd();
	virtual ~UICompositionWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	std::string m_compositionString;
};

class UIKeyStrokeWnd : public UIFrameWnd {
public:
	UIKeyStrokeWnd();
	virtual ~UIKeyStrokeWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

#endif
