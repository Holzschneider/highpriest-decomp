#include "common.h"
#include "framework/Locale.h"
#include "device/Sound.h"
#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>
#include <shellapi.h>

int g_mustPumpOutReceiveQueue = 0;
#include <process.h>
#include <atomic>
#include "Mode.h"
#include "GameMode.h"
#include "LoginMode.h"
#include "pdb_enums.h"
#include "Session.h"
#include "window/UISys.h"
#include "3dlib/Renderer.h"
#include "base/Res.h"
#include "device/Control.h"
#include "resource/Action.h"
#include "resource/Sprite.h"

extern UIWindowMgr g_windowMgr;
extern ServiceType g_serviceType;
extern void GDIFlip();
extern CRenderer *g_renderer;
extern CSession g_session;
extern CMouse g_mouse;



DWORD dwFadeStart;
std::atomic<int> FadeCnt(0);


inline BOOL CheckKeyPressed(int vkcode)
{
	return HIBYTE(GetAsyncKeyState(vkcode));
}

void ErrorMsg(const char *msg)
{
#if defined(BF_PORT)
	BF_TRACE(BFT_ERR, "ErrorMsg: %s", msg);
#endif
	GDIFlip();
	MessageBoxA(g_hMainWnd, msg, "Error", 0);
}

void ErrorMsg(int num)
{
	char buf[0x50];
	wsprintfA(buf, "%d\n", num);
#if defined(BF_PORT)
	BF_TRACE(BFT_ERR, "ErrorMsg: %d", num);
#endif
	GDIFlip();
	MessageBoxA(g_hMainWnd, buf, "Error", 0);
}

CMode::CMode()
{
	m_loopCond = TRUE;
	m_nextSubMode = -1;
	m_isConnected = true;
	m_helpBalloon = NULL;
	m_helpBalloonTick = bf_clock_ticks();
	m_cursorActNum = 0;
	m_cursorMotNum = 0;
	m_mouseAnimStartTick = bf_clock_ticks();
	m_isMouseLockOn = 0;
	m_screenShotNow = FALSE;
	m_mouseSnapDiff.x = 0;
	m_mouseSnapDiff.y = 0;
	m_fadeInCount = FALSE;
}

CMode::~CMode()
{
}

void CMode::Initialize()
{
	if (m_helpBalloon) {
		g_windowMgr.PostQuit((UIWindow*)m_helpBalloon);
		m_helpBalloon = NULL;
		m_helpBalloonTick = bf_clock_ticks();
	}
}

extern void ExitFindHack();
INT_PTR CMode::SendMsg(int messageId, INT_PTR val1, INT_PTR val2, INT_PTR val3)
{
	switch (messageId) {
		case MM_QUIT:
			ExitFindHack();
			g_modeMgr.Quit();
			break;
		case MM_SCREENSHOT:
			m_screenShotNow = TRUE;
			break;
	}
	return 0;
}

void CMode::DrawSprite(int x, int y, CActRes *act, CSprRes *spr, int actId, int motId, float zoom, float angle, DWORD color)
{
	const float align = 0.5f;
	float x1, y1, x2, y2;
	CTexture *tex;
	int i;
	CacheInfo info;
	CMotion *curMotion = act->GetMotion(actId, motId);
	for (i = 0; i < curMotion->numClips; ++i) {
		RPQuadFace& rpFace = g_renderer->BorrowQuadRP();
		tlvertex3d *sprVert = rpFace.m_verts;
		CSprClip *clip = curMotion->GetClip(i);
		SprImg *img = spr->m_sprites[clip->clipType][clip->sprIndex];
		x1 = clip->x * zoom * clip->zoomx;
		y1 = clip->y * zoom * clip->zoomy;
		x2 = (clip->x + img->width * (img->isHalfW + 1) - 1) * zoom * clip->zoomx;
		y2 = (clip->y + img->height * (img->isHalfH + 1) - 1) * zoom * clip->zoomy;
		if (clip->clipType == 0) {
			tex = g_renderer->GetSpriteIndex(*img, (INT_PTR)spr->m_pal, &info);
			if (tex == NULL)
				tex = g_renderer->AddSpriteIndex(*img, spr->m_pal, &info);
		} else {
			tex = img->tex;
		}
		rpFace.tex = tex;
		if (angle) {
			float cs = GetCos(angle);
			float sn = GetSin(angle);
			sprVert[0].x = x + (x1 * cs - y1 * sn) + align;
			sprVert[0].y = y + (x1 * sn + y1 * cs) + align;
			sprVert[1].x = x + (x2 * cs - y1 * sn) + align;
			sprVert[1].y = y + (x2 * sn + y1 * cs) + align;
			sprVert[2].x = x + (x2 * cs - y2 * sn) + align;
			sprVert[2].y = y + (x2 * sn + y2 * cs) + align;
			sprVert[3].x = x + (x1 * cs - y2 * sn) + align;
			sprVert[3].y = y + (x1 * sn + y2 * cs) + align;
		} else {
			float x11 = (!(clip->flags & 1) ? x1 : x2);
			float x22 = (!(clip->flags & 1) ? x2 : x1);
			sprVert[0].x = x + x11 + align;
			sprVert[0].y = y + y1 + align;
			sprVert[1].x = x + x22 + align;
			sprVert[1].y = y + y1 + align;
			sprVert[2].x = x + x11 + align;
			sprVert[2].y = y + y2 + align;
			sprVert[3].x = x + x22 + align;
			sprVert[3].y = y + y2 + align;
		}
		sprVert[0].z = 0.000001f; sprVert[0].oow = 0.999999f;
		sprVert[0].tu = info.tu;  sprVert[0].tv = info.tv;  sprVert[0].color = color;
		sprVert[1].z = 0.000001f; sprVert[1].oow = 0.999999f;
		sprVert[1].tu = info.tu1; sprVert[1].tv = info.tv;  sprVert[1].color = color;
		sprVert[2].z = 0.000001f; sprVert[2].oow = 0.999999f;
		sprVert[2].tu = info.tu;  sprVert[2].tv = info.tv1; sprVert[2].color = color;
		sprVert[3].z = 0.000001f; sprVert[3].oow = 0.999999f;
		sprVert[3].tu = info.tu1; sprVert[3].tv = info.tv1; sprVert[3].color = color;
		g_renderer->AddRP(&rpFace, RF_UI);
	}
}

void CMode::DrawTexture(RECT *rect, int angle, float oow, DWORD argb, CTexture *tex, UVRECT uvRect)
{
	RPQuadFace& rpFace = g_renderer->BorrowQuadRP();
	rpFace.tex = tex;
	tlvertex3d *verts = rpFace.verts;
	float z  = GetZFromOow(oow);
	float z1 = GetZFromOow(oow);
	if (angle) {
		float cs = GetCos(angle);
		float sn = GetSin(angle);
		float w = (rect->right - rect->left);
		float h = (rect->bottom - rect->top);
		float x = rect->left + w / 2;
		float y = rect->top  + h / 2;
		float x1 = -w / 2, y1 = -h / 2;
		float x2 =  w / 2, y2 =  h / 2;
		verts[0].x = x + (x1 * cs - y1 * sn);
		verts[0].y = y + (x1 * sn + y1 * cs);
		verts[1].x = x + (x2 * cs - y1 * sn);
		verts[1].y = y + (x2 * sn + y1 * cs);
		verts[2].x = x + (x1 * cs - y2 * sn);
		verts[2].y = y + (x1 * sn + y2 * cs);
		verts[3].x = x + (x2 * cs - y2 * sn);
		verts[3].y = y + (x2 * sn + y2 * cs);
	} else {
		verts[0].x = rect->left;  verts[0].y = rect->top;
		verts[1].x = rect->right; verts[1].y = rect->top;
		verts[2].x = rect->left;  verts[2].y = rect->bottom;
			verts[3].x = rect->right; verts[3].y = rect->bottom;
		}
		float uAdjust = tex->GetUAdjust();
		float vAdjust = tex->GetVAdjust();
		verts[0].z = z1; verts[0].oow = oow;
		verts[0].tu = uvRect.u1 * uAdjust; verts[0].tv = uvRect.v1 * vAdjust;
		verts[0].color = argb;
	verts[1].z = z1; verts[1].oow = oow;
	verts[1].tu = uvRect.u2 * uAdjust; verts[1].tv = uvRect.v1 * vAdjust;
	verts[1].color = argb;
	verts[2].z = z;  verts[2].oow = oow;
	verts[2].tu = uvRect.u1 * uAdjust; verts[2].tv = uvRect.v2 * vAdjust;
	verts[2].color = argb;
	verts[3].z = z;  verts[3].oow = oow;
	verts[3].tu = uvRect.u2 * uAdjust; verts[3].tv = uvRect.v2 * vAdjust;
		verts[3].color = argb;
		g_renderer->AddRP(&rpFace, RF_UI);
}

void CMode::DrawBattleMode()
{
	if (g_session.m_isBattle == TRUE) {
		float oow = 1.0f / NEARPLANE - 0.0001f;
		if (g_session.m_isBmChat != TRUE) {
			RECT rect = { 113, g_renderer->m_height - 24, 136, g_renderer->m_height - 1 };
			DrawTexture(&rect, 0, oow, 0xeeffffff, g_texMgr.GetTexture("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xb9\xdf\xb9\xb6.bmp", false));
		}
	}
}

void CMode::DrawMouseCursor()
{
	CActRes *cursorAct = (CActRes *)g_resMgr().Get("cursors.act", false);
	DWORD motionSpeed;
	CSprRes *cursorSpr;
	cursorSpr = (CSprRes *)g_resMgr().Get("cursors.spr", false);
	DWORD tick = (bf_clock_ticks() - m_mouseAnimStartTick) / 24;
	cursorAct->GetDelay(m_cursorActNum);
	motionSpeed = 4;
	switch (m_cursorActNum) {
		case 2:
			motionSpeed = 4;
			break;
		case 5:
			motionSpeed = 20;
			break;
		case 7:
			motionSpeed = 10;
			break;
		case 9:
			motionSpeed = 1;
			break;
		case 10:
			motionSpeed = 6;
			break;
		case 11:
			motionSpeed = 3;
			break;
	}

	switch (m_cursorActNum) {
		case 1: {
			DWORD frame = tick / motionSpeed;
			m_cursorMotNum = frame % cursorAct->GetMotionCount(1);
			if (frame / cursorAct->GetMotionCount(m_cursorActNum) >= 1)
				m_cursorMotNum = cursorAct->GetMotionCount(m_cursorActNum) - 1;
			break;
		}
		case 2:
		case 9:
		{
			int buttonState = g_mouse.GetLBtn();
			if (buttonState) {
				DWORD frame = tick / motionSpeed;
				m_cursorMotNum = frame % cursorAct->GetMotionCount(m_cursorActNum);
				if (frame / cursorAct->GetMotionCount(m_cursorActNum) >= 1)
					m_cursorMotNum = cursorAct->GetMotionCount(m_cursorActNum) - 1;
			} else {
				m_mouseAnimStartTick = bf_clock_ticks();
				m_cursorMotNum = 0;
			}
			break;
		}
		default: {
			DWORD frame = tick / motionSpeed;
			m_cursorMotNum = frame % cursorAct->GetMotionCount(m_cursorActNum);
			break;
		}
	}
	g_windowMgr.SendMsg(UIM_CURSOR_CHANGE_MOTION, m_cursorMotNum, 0, 0, 0);
	DrawSprite((int)(g_mouse.GetXPos() + m_mouseSnapDiff.x),
		(int)(g_mouse.GetYPos() + m_mouseSnapDiff.y),
		cursorAct, cursorSpr, m_cursorActNum, m_cursorMotNum, 1.0f, 0.0f, 0xfaffffff);
}

unsigned int __stdcall FadeBgm(void *res)
{
	FadeCnt++;
#if !defined(BF_PORT)
	if (g_session.m_BgmVolume == 0)
		return -1;
#endif
	while (FadeCnt != 1)
		bf_clock_sleep_ms(5);
	int a, vol = (INT_PTR)res;
	if (vol == 0) {
		vol = GetStreamVolume();
		for (a = vol; a > 0; a--) {
			if (FadeCnt != 1) { FadeCnt--; return 0; }
			SetStreamVolume(a);
			bf_clock_sleep_ms(10);
		}
		PauseStream(TRUE);
		FadeCnt--;
		return 0;
	} else {
		PauseStream(FALSE);
		for (a = 0; a < vol; a++) {
			if (FadeCnt != 1) { FadeCnt--; return 0; }
			SetStreamVolume(a);
			bf_clock_sleep_ms(10);
		}
		FadeCnt--;
	}
	return 0;
}

void CMode::ProcessFadeIn()
{
	if (m_fadeInCount) {
		dwFadeStart = bf_clock_ticks();
		unsigned int hFadeBgmThread;
#if defined(BF_PORT)
		// M8.5 (docs/port/threads.md): the volume is tested here, on the
		// main thread that sets it, not in the thread. The 2008 thread
		// returned on 0 without taking its count back, so FadeCnt stayed
		// at 1 or more and every later fade thread waited forever.
		if (g_session.m_BgmVolume)
#endif
		_beginthreadex(NULL, 0, FadeBgm, (void*)(INT_PTR)g_session.m_BgmVolume, 0, &hFadeBgmThread);
		m_fadeInCount = FALSE;
	}
	DWORD dwFadeCurrent = (bf_clock_ticks() - dwFadeStart);
	if (dwFadeCurrent < 0xFF) {
		DrawBoxScreen(0, 0, g_renderer->m_width, g_renderer->m_height,
			(0xFF - dwFadeCurrent) * 0x1000000);
		}
}

extern unsigned int dwFadeStart;
void CMode::RunFadeOut(int bDrawLogo)
{
	unsigned int hFadeBgmThread;
#if defined(BF_PORT)
	if (g_session.m_BgmVolume)   // M8.5: see ProcessFadeIn
#endif
	_beginthreadex(NULL, 0, FadeBgm, (void *)0, 0, &hFadeBgmThread);
	dwFadeStart = bf_clock_ticks();
	g_renderer->BackupFrame();
	DWORD dwFadeCurrent;
	while ((dwFadeCurrent = bf_clock_ticks() - dwFadeStart) < 0xFF) {
		DrawBoxScreen(0, 0, g_renderer->m_width, g_renderer->m_height,
			dwFadeCurrent << 24);
		if (bDrawLogo) {
			CBitmapRes *logo = (CBitmapRes *)g_resMgr().Get("rag_logo.bmp", 0);
			g_windowMgr.DrawBitmapToFrame(g_renderer->m_width - 0xc0,
				g_renderer->m_height - 0x7a, logo);
		}
		if (g_renderer->DrawScene())
			g_renderer->Flip(true);
		g_renderer->RestoreFrame();
	}
}

void CMode::MakeHelpBalloon(const char *helpMsg, int x, int y, DWORD fntColor)
{
	char nHelpMsg[512];
	if (strlen(helpMsg) == 0)
		return;

	g_session.RemoveUnderbar(helpMsg, nHelpMsg);
	int color[3];
	color[0] = (fntColor >> 16) & 0xff;
	color[1] = (fntColor >> 8) & 0xff;
	color[2] = fntColor & 0xff;
	m_helpBalloonTick = bf_clock_ticks();
	if (m_helpBalloon == NULL) {
		m_helpBalloon = new UITransBalloonText;
		if (m_helpBalloon == NULL)
			return;
		m_helpBalloon->Create(8, 8);
		g_windowMgr.AddWindow(m_helpBalloon);
	} else {
		g_windowMgr.MakeTopLayer(m_helpBalloon);
	}
	m_helpBalloon->SetText(nHelpMsg, 35);
	m_helpBalloon->SetFntColor(RGB(color[0], color[1], color[2]), 0);
	m_helpBalloon->SetTransBoxColor(0x88000000);

	int cx = x;
	int cy = y;
	if (cx < -3)
		cx = -3;
	if (cx > g_renderer->m_width - m_helpBalloon->GetWidth() + 3)
		cx = g_renderer->m_width - m_helpBalloon->GetWidth() + 3;
	if (cy < -3)
		cy = -3;
	if (cy > g_renderer->m_height - m_helpBalloon->GetHeight() + 3)
		cy = g_renderer->m_height - m_helpBalloon->GetHeight() + 3;
	m_helpBalloon->Move(cx, cy);

}
void CMode::ProcessHelpBalloon()
{
	if (m_helpBalloon) {
		if (bf_clock_ticks() >= m_helpBalloonTick + 100) {
			g_windowMgr.PostQuit((UIWindow*)m_helpBalloon);
			m_helpBalloon = NULL;
		}
	}
}

void CMode::ProcessKeyBoard()
{
}

void CMode::SetCursorAction(int cursorActNum)
{
	if (m_cursorActNum != cursorActNum) {
		m_cursorActNum = cursorActNum;
		m_cursorMotNum = 0;
		m_mouseAnimStartTick = bf_clock_ticks();
		g_windowMgr.SendMsg(UIM_CURSOR_CHANGE_ACTION, m_cursorActNum, 0, 0, 0);
	}
}

void CMode::ScreenShot()
{
	if (m_screenShotNow) {
		{
			SYSTEMTIME st;
			GetLocalTime(&st);
			char msg[128];
			const char *week[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", };

			if (g_serviceType != ServiceKorea)
				sprintf(msg, "%d/%02d/%02d/%s  %02d: %02d: %02d", st.wYear, st.wMonth, st.wDay, week[st.wDayOfWeek], st.wHour, st.wMinute, st.wSecond);
			else
			{

				if (g_session.GetGid() > 0)
				{
					char ID[12];
					memset(ID, 0, 12);

					{
						int total, digit, num, zero;
						total = g_session.GetGid();
						digit = 100000000;

						zero = 0;
						while (digit > 0)
						{
							num = 0;
							while (total >= digit)
							{
								num++;
								total -= digit;
							}
							digit /= 10;

							if (zero > 0)
								zero++;
							else if (num > 0)
								zero = 1;

							if (num == 0)
							{
								if (zero > 0)
									ID[zero - 1] = 'R';
							}
							else if (num == 1)
								ID[zero - 1] = 'O';
							else if (num == 2)
								ID[zero - 1] = 'H';
							else if (num == 3)
								ID[zero - 1] = 'U';
							else if (num == 4)
								ID[zero - 1] = 'T';
							else if (num == 5)
								ID[zero - 1] = 'N';
							else if (num == 6)
								ID[zero - 1] = 'A';
							else if (num == 7)
								ID[zero - 1] = 'S';
							else if (num == 8)
								ID[zero - 1] = 'E';
							else if (num == 9)
								ID[zero - 1] = 'W';
						}
					}

					sprintf(msg, "%d/%02d/%02d/%s  %02d: %02d: %02d     %s", st.wYear, st.wMonth, st.wDay, week[st.wDayOfWeek], st.wHour, st.wMinute, st.wSecond, ID);
				}
			}

			g_renderer->TextOutScreen(4 + 1, g_renderer->m_height - 16, msg, RGB(0, 0, 0), 12, 0);
			g_renderer->TextOutScreen(4 - 1, g_renderer->m_height - 16, msg, RGB(0, 0, 0), 12, 0);
			g_renderer->TextOutScreen(4, g_renderer->m_height - 16 + 1, msg, RGB(0, 0, 0), 12, 0);
			g_renderer->TextOutScreen(4, g_renderer->m_height - 16 - 1, msg, RGB(0, 0, 0), 12, 0);
			g_renderer->TextOutScreen(4, g_renderer->m_height - 16, msg, RGB(255, 255, 255), 12, 0);
		}

		m_screenShotNow = FALSE;
		static int captureCnt = 0;
		char tmp[2048];

		sprintf(tmp, "%s\\ScreenShot", g_baseDir3);
		if (access(tmp, 0)) {
			if (_mkdir(tmp))
				return;
		}

		std::string serverName = g_session.m_lastServerName;
		FixInvalidNameForFile(serverName);

		if (CheckKeyPressed(VK_CONTROL)) {
			if (ServiceVietnam != g_serviceType)
			{
				sprintf(tmp, "%s\\ScreenShot\\screen%s%03d.bmp", g_baseDir3, serverName.c_str(), captureCnt++);
				while (access(tmp, 0) == 0)
					sprintf(tmp, "%s\\ScreenShot\\screen%s%03d.bmp", g_baseDir3, serverName.c_str(), captureCnt++);
			}
			else
			{
				sprintf(tmp, "%s\\ScreenShot\\screen%03d.jpg", g_baseDir3, captureCnt++);
				while (access(tmp, 0) == 0)
					sprintf(tmp, "%s\\ScreenShot\\screen%03d.jpg", g_baseDir3, captureCnt++);
			}

			CBitmapRes *logoBitmap = (CBitmapRes *)g_resMgr().Get("scr_logo.bmp", false);
			g_renderer->SaveBmp(tmp, logoBitmap);
		} else {
			if (ServiceVietnam != g_serviceType)
			{
				sprintf(tmp, "%s\\ScreenShot\\screen%s%03d.jpg", g_baseDir3, serverName.c_str(), captureCnt++);
				while (access(tmp, 0) == 0)
					sprintf(tmp, "%s\\ScreenShot\\screen%s%03d.jpg", g_baseDir3, serverName.c_str(), captureCnt++);
			}
			else
			{
				sprintf(tmp, "%s\\ScreenShot\\screen%03d.jpg", g_baseDir3, captureCnt++);
				while (access(tmp, 0) == 0)
					sprintf(tmp, "%s\\ScreenShot\\screen%03d.jpg", g_baseDir3, captureCnt++);
			}

			CBitmapRes *logoBitmap = (CBitmapRes *)g_resMgr().Get("scr_logo.bmp", false);
			g_renderer->SaveJPG(tmp, logoBitmap);
		}

#if defined(BF_PORT)
		// The message names the file where it is, in Pictures/<name> (spec
		// section 5), not under the game folder the client's path names.
		char shown[1024];
		if (bf_file_resolve(tmp, BF_RESOLVE_READ, shown, sizeof(shown)) >= 0)
			strcpy(tmp, shown);
#endif
		std::string screenShotMsg = tmp;
		screenShotMsg += " is Saved.";
		g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)screenShotMsg.c_str(), 0x0000ff00, 0, 0);
	}
}

#if defined(BF_PORT)
// Declared in Mode.h, never defined in the reconstruction; the control
// socket's `mode` reads it (port/client/src/bf_client_state.cpp).
int CMode::GetSubMode()
{
	return m_subMode;
}
#endif

CModeMgr g_modeMgr;
CModeMgr::CModeMgr()
{
	m_loopCond = TRUE;
}

CLoginMode * CModeMgr::GetLoginMode()
{
	if (MT_LOGIN == m_curModeType)
		return (CLoginMode *)m_curMode;
	else
		return NULL;
}

CGameMode * CModeMgr::GetGameMode()
{
	if (MT_GAME == m_curModeType)
		return (CGameMode *)m_curMode;
	else
		return NULL;
}

#if defined(BF_PORT)
int g_bfModeRunning = 0;
#endif

void CModeMgr::Run(int modeType, const char *worldName)
{
	m_nextModeType = m_curModeType = modeType;
	strcpy(m_curModeName, worldName);
	strcpy(m_nextModeName, worldName);
	while (m_loopCond && !g_sysQuit) {
		m_curModeType = m_nextModeType;
		strcpy(m_curModeName, m_nextModeName);
		switch (m_curModeType) {
			case MT_LOGIN:
				m_curMode = new CLoginMode;
				break;
			case MT_GAME:
				m_curMode = new CGameMode;
				break;
		}
#if defined(BF_PORT)
		BF_TRACE(BFT_MODE, "mode %s \"%s\"", m_curModeType == MT_LOGIN ? "login" : "game", m_curModeName);
#endif
		m_curMode->OnInit(m_curModeName);
#if defined(BF_PORT)
		g_bfModeRunning = 1;   // the port may draw a frame during a live resize (WinMain.cpp)
#endif
		m_curMode->OnRun();
#if defined(BF_PORT)
		g_bfModeRunning = 0;
#endif
		m_curMode->OnExit();
		delete m_curMode;
		m_curMode = NULL;
	}
}

void CModeMgr::Switch(int modeType, const char *modeName)
{
	m_curMode->PostQuitMsg();
	strcpy(m_nextModeName, modeName);
	m_nextModeType = modeType;
}

void CModeMgr::Quit()
{
	m_curMode->PostQuitMsg();
	if (g_serviceType == ServiceIndia) {
		ShellExecuteA(g_hMainWnd, "open", "http://www.ragnarok.co.in/index.php", NULL, NULL, 1);
	}
	m_loopCond = 0;
}
