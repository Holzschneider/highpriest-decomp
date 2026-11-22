#ifndef HIGHPRIEST_WINDOW_UIILLUSTWND_H
#define HIGHPRIEST_WINDOW_UIILLUSTWND_H

#include "UIFrameWnd.h"

class UIIllustWnd;

class UIIllustWnd : public UIFrameWnd {
public:
	UIIllustWnd();
	virtual ~UIIllustWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDestroy();
	virtual bool ShouldDoHitTest();
	virtual void OnMouseMove(int cx, int cy);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
protected:
	int m_isDrawTitle;
	int m_isMoveableWnd;
	std::string m_bitmapName;
};

#endif
