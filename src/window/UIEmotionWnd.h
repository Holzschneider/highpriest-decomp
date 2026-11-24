#ifndef HIGHPRIEST_WINDOW_UIEMOTIONWND_H
#define HIGHPRIEST_WINDOW_UIEMOTIONWND_H

#include "UIFrameWnd.h"

class UIEmotionWnd;

class UIEmotionWnd : public UIFrameWnd {
public:
	UIEmotionWnd() {}
	virtual ~UIEmotionWnd() {}
	virtual void OnCreate(int cx, int cy);
	virtual void StoreInfo();
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	void InitFromReg();
	void SetUserDefineText(char* text, int Index);
	void WriteToReg();
protected:
	int m_curIndex;
	UIStaticText* m_staticText[10];
	UIEditCtrl* m_UserDefineText[10];
	EmotionWndInfo* m_info;
	UIBitmapButton* m_emotionListButton;
};

#endif
