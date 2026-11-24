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
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	int m_isDrawTitle;
	int m_isMoveableWnd;
	std::string m_bitmapName;
};

#endif
