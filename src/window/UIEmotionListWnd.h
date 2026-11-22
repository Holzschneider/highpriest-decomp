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
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
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
