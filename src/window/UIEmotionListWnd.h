#ifndef HIGHPRIEST_WINDOW_UIEMOTIONLISTWND_H
#define HIGHPRIEST_WINDOW_UIEMOTIONLISTWND_H

#include "UIFrameWnd.h"

class UIEmotionListWnd;

class UIEmotionListWnd : public UIFrameWnd {
public:
	UIEmotionListWnd() {}
	virtual ~UIEmotionListWnd() {}
	virtual void OnCreate(int cx, int cy);
	virtual void OnProcess();
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnMouseMove(int cx, int cy);
	virtual void StoreInfo();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void OnDraw();
	void InitEmotionName();
	void InitFromReg();
	void WriteToReg();
protected:
	int m_oldIndex;
	int m_curIndex;
	EmotionListWndInfo* m_info;
	std::vector<char *> m_emotionName;
	UISpriteButton* m_emotionButton[49];
};

#endif
