#include "common.h"
#include "window/UIFrameWnd.h"
#include "Session.h"
#include "device/Control.h"
#include "GameMode.h"
#include "resource/ImfRes.h"
#include "Insult.h"
#include "MsgStrings.h"
#include "base/Util.h"
#include "TipOfTheDay.h"
#include "device/Sound.h"
#include "skin.h"
#include "GuildInfo.h"
#include "framework/Locale.h"
#include "framework/Language.h"
#include "3dlib/DrawUtil.h"
#include <time.h>
#include "GameWorld.h"
#include "device/Connection.h"
#include "UIRectInfo.h"
#include "window/UISys.h"
#include "base/Res.h"
#include "resource/Bitmap.h"
#include "resource/Action.h"
#include "resource/PaletteRes.h"
#include "resource/Sprite.h"
#include "3dlib/Renderer.h"
#include "Mode.h"
#include "pdb_enums.h"
#include "resource/Ground.h"

extern CModeMgr g_modeMgr;
extern CSession g_session;
extern ServiceType g_serviceType;
extern const char* MsgStr(MSGSTRINGID id);


extern const char* UIBmp(const char* uiBmpName);
extern UIWindowMgr g_windowMgr;
extern CMouse g_mouse;
extern const char* g_regPath;
extern CRenderer* g_renderer;
extern "C" int __cdecl access(const char *, int);
extern "C" int __cdecl _mkdir(const char *);
void CutString(const char* msg, std::vector<std::string>& strings, int maxNumCharLine);

UICharInfoBalloonText::UICharInfoBalloonText() {}

UICharInfoBalloonText::~UICharInfoBalloonText() {}


void UICharInfoBalloonText::OnDraw()
{
	ClearDC(0xffff00ff);

	int height = 6;
	for (int i = 0; i < m_strings.size(); ++i) {
		int x = 6;
		if (i % 2 != 0)
			x = 50;
		TextOutWithShadow(x, height, m_strings[i].c_str(), 0,
						  m_fontColor, m_bgColor, 0, m_fontSize);
		if (i % 2 != 0)
			height += GetTextHeight(m_strings[i].c_str(), 0, 0, m_fontSize, false) + 4;
	}
}

UISelCharForUServerWnd::UISelCharForUServerWnd()
{
	m_selCharNum = 0;
	m_stateCnt = 0;
	m_okButton = NULL;
	for (int i = 0; i < 12; ++i)
		m_isEmpty[i] = 1;
	m_stateStartTick = timeGetTime();
}

UISelCharForUServerWnd::~UISelCharForUServerWnd()
{}

#include "rag_budget_fill.h"
void UISelCharForUServerWnd::OnCreate(int cx, int cy)
{
	{
		std::string pathName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\");

		m_okButton = new UIBitmapButton;
		m_okButton->SetBitmapName((pathName + "btn_ok" + ".bmp").c_str(), BS_NORMAL);
		m_okButton->SetBitmapName((pathName + "btn_ok_a" + ".bmp").c_str(), BS_MOUSEON);
		m_okButton->SetBitmapName((pathName + "btn_ok_b" + ".bmp").c_str(), BS_PRESSED);
		m_okButton->Create(m_okButton->GetBitmapWidth(), m_okButton->GetBitmapHeight());
		m_okButton->Move(m_w - 46, m_h - 24);
		m_okButton->SetId(ID_OK);
		AddChild(m_okButton);
	}
	m_defPushId = 0x76;

	for (int i = 0; i < 12; ++i) {
		RAG_BUDGET_FILL(4)
		m_checkBox[i] = new UICheckBox;
		m_checkBox[i]->Create(10, 10);
		m_checkBox[i]->SetBitmap2(
			"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_1.bmp",
			"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_0.bmp",
			"");
		m_checkBox[i]->Move(-100, -100);
		AddChild(m_checkBox[i]);
	}

	for (int m = 0; m < 12; ++m) {
		CHARACTER_INFO* charInfo = (CHARACTER_INFO*)g_modeMgr.GetCurMode()->SendMsg(MM_QUERYCHARICTORINFO, m, 0, 0);
		if (!charInfo)
			continue;

		m_checkBox[m]->Move((m % 6) * 81 + 63, (m / 6) * 149 + 77);

		int slot = charInfo->CharNum;
		m_isEmpty[slot] = 0;
		m_viewChar[slot].x = (slot % 6) * 81 + 68;
		m_viewChar[slot].y = (slot / 6) * 149 + 191;
		m_viewChar[slot].baseAction = 0;
		m_viewChar[slot].curAction = 0;
		m_viewChar[slot].curMotion = 0;

		char tmpName[256]; m_viewChar[slot].x = m_viewChar[slot].x;
		m_viewChar[slot].actName[0] = g_session.GetJobActName(charInfo->job, g_session.GetSex(), tmpName);
		m_viewChar[slot].sprName[0] = g_session.GetJobSprName(charInfo->job, g_session.GetSex(), tmpName);

		int head = charInfo->head;
		m_viewChar[slot].actName[1] = g_session.GetHeadActName(charInfo->job, head, g_session.GetSex(), tmpName);
		m_viewChar[slot].sprName[1] = g_session.GetHeadSprName(charInfo->job, head, g_session.GetSex(), tmpName);
		m_viewChar[slot].actName[2] = g_session.GetAccessoryActName(charInfo->job, head, g_session.GetSex(), charInfo->accessory, tmpName);
		m_viewChar[slot].sprName[2] = g_session.GetAccessorySprName(charInfo->job, head, g_session.GetSex(), charInfo->accessory, tmpName);
		charInfo->head = head;
		m_viewChar[slot].actName[3] = g_session.GetAccessoryActName(charInfo->job, head, g_session.GetSex(), charInfo->accessory2, tmpName);
		m_viewChar[slot].sprName[3] = g_session.GetAccessorySprName(charInfo->job, head, g_session.GetSex(), charInfo->accessory2, tmpName);
		m_accessory_top[slot] = charInfo->accessory2;
		m_viewChar[slot].actName[4] = g_session.GetAccessoryActName(charInfo->job, head, g_session.GetSex(), charInfo->accessory3, tmpName);
		m_viewChar[slot].sprName[4] = g_session.GetAccessorySprName(charInfo->job, head, g_session.GetSex(), charInfo->accessory3, tmpName);

		if (charInfo->job > 4000 && charInfo->job <= 4007)
			m_viewChar[slot].bodyPalette = 1;
		else
			m_viewChar[slot].bodyPalette = charInfo->bodypalette;

		if (m_viewChar[slot].bodyPalette) {
			char palName[64];
			m_viewChar[slot].bodyPaletteName = g_session.GetBodyPaletteName(charInfo->job, g_session.GetSex(), m_viewChar[slot].bodyPalette, palName);
		}

		m_viewChar[slot].headPalette = charInfo->headpalette;
		if (m_viewChar[slot].headPalette) {
			char palName[64];
			m_viewChar[slot].headPaletteName = g_session.GetHeadPaletteName(head, charInfo->job, g_session.GetSex(), m_viewChar[slot].headPalette, palName);
		}

		m_viewChar[slot].imfName = g_session.GetImfName(charInfo->job, charInfo->head, g_session.GetSex(), tmpName);
	}
}

void UISelCharForUServerWnd::OnDraw()
{
	DrawBitmap(0, 0,
		(CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\login_interface\\selCharForIntergration.bmp"), false),
		false);

	static int head_imfOffX = 0;
	static int head_imfOffY = 0;

	for (int j = 0; j < 12; ++j) {
		if (m_isEmpty[j])
			continue;

		CTexture* tex;
		CacheInfo info;

		for (int i = 0; i < 5; ++i) {
			if (m_viewChar[j].actName[i].size() == 0 || m_viewChar[j].sprName[i].size() == 0)
				continue;

			g_resMgr().Get(m_viewChar[j].imfName.c_str(), false);
			CActRes* actRes = (CActRes*)g_resMgr().Get(m_viewChar[j].actName[i].c_str(), false);
			CSprRes* sprRes = (CSprRes*)g_resMgr().Get(m_viewChar[j].sprName[i].c_str(), false);
			CMotion* curMotion = actRes->GetMotion(m_viewChar[j].curAction, m_viewChar[j].curMotion);

			int offX = 0;
			int offY = 0;

			if (i == 1) {
				CMotion* headMotion = curMotion;
				CMotion* bodyMotion = ((CActRes*)g_resMgr().Get(m_viewChar[j].actName[0].c_str(), false))->GetMotion(m_viewChar[j].curAction, m_viewChar[j].curMotion);

				if (headMotion->attachCnt && bodyMotion->attachCnt &&
					headMotion->attachInfo[0].m_attr == bodyMotion->attachInfo[0].m_attr) {
					offX += bodyMotion->attachInfo[0].x - headMotion->attachInfo[0].x;
					offY += bodyMotion->attachInfo[0].y - headMotion->attachInfo[0].y;
					head_imfOffX = offX;
					head_imfOffY = offY;
				}
			} else if (i == 2 || i == 3 || i == 4) {
				CMotion* accMotion = curMotion;
				CMotion* headMotion = ((CActRes*)g_resMgr().Get(m_viewChar[j].actName[1].c_str(), false))->GetMotion(m_viewChar[j].curAction, m_viewChar[j].curMotion);
				CHARACTER_INFO* charInfo = (CHARACTER_INFO*)g_modeMgr.GetCurMode()->SendMsg(MM_QUERYCHARICTORINFO, j, 0, 0);

				int accessory = 0;
				if (i == 2)
					accessory = charInfo->accessory;
				else if (i == 3)
					accessory = charInfo->accessory2;
				else if (i == 4)
					accessory = charInfo->accessory3;

				if (g_windowMgr.IsAniAccessory(accessory)) {
					int headMotionCount = ((CActRes*)g_resMgr().Get(m_viewChar[j].actName[1].c_str(), false))->GetMotionCount(m_viewChar[j].curAction);
					int accMotionCount = ((CActRes*)g_resMgr().Get(m_viewChar[j].actName[i].c_str(), false))->GetMotionCount(m_viewChar[j].curAction);

					if (headMotionCount > 0 && accMotionCount > 0 &&
						accMotionCount != headMotionCount && accMotionCount > headMotionCount &&
						accMotionCount % headMotionCount == 0) {
						int motionStep = accMotionCount / headMotionCount;
						int motionSpeed = (int)((CActRes*)g_resMgr().Get(m_viewChar[j].actName[i].c_str(), false))->GetDelay(m_viewChar[j].curAction);
						float frame = (timeGetTime() - m_stateStartTick) / 24.0f;
						if (motionSpeed == 0)
							motionSpeed = 4;

						int motion = (int)(frame / motionSpeed) % motionStep;
						int tmpMotion = m_viewChar[j].curMotion * motionStep + motion;
						curMotion = accMotion = ((CActRes*)g_resMgr().Get(m_viewChar[j].actName[i].c_str(), false))->GetMotion(m_viewChar[j].curAction,
							tmpMotion);
					}
				}

				if (accMotion->attachCnt && headMotion->attachCnt &&
					accMotion->attachInfo[0].m_attr == headMotion->attachInfo[0].m_attr) {
					offX = headMotion->attachInfo[0].x - accMotion->attachInfo[0].x;
					offY = headMotion->attachInfo[0].y - accMotion->attachInfo[0].y;
				}

				offX += head_imfOffX;
				offY += head_imfOffY;

				if (i == 3) {
					switch (m_accessory_top[j]) {
					case 0xb5:
					case 0xc5:
						offY -= 7;
						break;
					}
				}
			}

			unsigned long* palette = sprRes->m_pal;
			switch (i) {
			case 0:
				if (m_viewChar[j].bodyPalette)
					palette = ((CPaletteRes*)g_resMgr().Get(m_viewChar[j].bodyPaletteName.c_str(), false))->m_pal;
				break;
			case 1:
				if (m_viewChar[j].headPalette)
					palette = ((CPaletteRes*)g_resMgr().Get(m_viewChar[j].headPaletteName.c_str(), false))->m_pal;
				break;
			}

			int clipIndex = i;
			if (i > 1)
				clipIndex = 0;

			CSprClip* clip = curMotion->GetClip(clipIndex);
			if (clip->sprIndex == -1)
				continue;

			SprImg* img = sprRes->m_sprites[clip->clipType][clip->sprIndex];
			if (clip->clipType == 0) {
				tex = g_renderer->GetSpriteIndex(*img, (unsigned long)palette, &info);
				if (!tex)
					tex = g_renderer->AddSpriteIndex(*img, palette, &info);
			} else {
				tex = img->tex;
			}

			m_dc->BltTexture2(m_viewChar[j].x + clip->x + offX,
				m_viewChar[j].y + clip->y + offY,
				tex,
				(int)(info.tu * 256.0f),
				(int)(info.tv * 256.0f),
				img->width,
				img->height,
				clip->flags,
				img->isHalfW + 1,
				img->isHalfH + 1);
		}
	}
}

int UISelCharForUServerWnd::SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{

	switch (message) {
	case UM_COMMAND:
		switch (val1) {
		case ID_OK:
			if (g_windowMgr.ErrorMsg(MsgStr(MSI_CONFIRM_SELECT_CHARACTERS), ME_YESNO, FALSE, FALSE, 0) == ID_YES) {
				unsigned long m_selCharGID[9] = { 0 };

				int j = 0;
				for (int i = 0; i < 12; ++i) {
					if (m_checkBox[i]->GetCheck()) {
						CHARACTER_INFO* charInfo = (CHARACTER_INFO*)g_modeMgr.GetCurMode()->SendMsg(MM_QUERYCHARICTORINFO, i, 0, 0);
						if (charInfo) {
							m_selCharGID[j] = charInfo->GID;
							++j;
						}
					}
				}

				if (j == 0) {
					g_windowMgr.ErrorMsg(MsgStr(MSI_NO_SELECT_CHARACTERS), ME_OK, TRUE, TRUE, 0);
					return 0;
				}

				PACKET_CH_SELECT_CHAR_GOINGTOBEUSED p;
				p.PacketType = HEADER_CH_SELECT_CHAR_GOINGTOBEUSED;
				p.dwAID = g_session.GetAid();
				p.nCountSelectedChar = j;
				memcpy(p.ardwSelectedGID, m_selCharGID, sizeof(m_selCharGID));

				CRagConnection::instanceR().SendPacket(
					CRagConnection::instanceR().GetPacketSize(HEADER_CH_SELECT_CHAR_GOINGTOBEUSED),
					(const char*)&p);
				g_windowMgr.PostQuit(this);
			}
			break;

		case ID_CHECK:
			if (((UICheckBox*)sender)->GetCheck())
				++m_selCharNum;
			else
				--m_selCharNum;

			if (m_selCharNum > 9) {
				g_windowMgr.ErrorMsg(MsgStr(MSI_ERROR_SELECT_CHARACTERS), ME_OK, TRUE, TRUE, 0);
				((UICheckBox*)sender)->SetCheck(0);
				return 0;
			}
			break;
		}
		break;

	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}

CHARACTER_INFO* UISelCharForUServerWnd::GetCharInfo(int x, int y)
{
	if (x < 27 || x >= m_w - 26 || y < 95 || y >= 365 || (y < 250 && y > 210))
		return 0;
	int col = (x - 27) / 81;
	int row = (y - 95) / 135;
	int index = col + row * 6;
	CHARACTER_INFO* info = (CHARACTER_INFO*)g_modeMgr.GetCurMode()->SendMsg(MM_QUERYCHARICTORINFO, index, 0, 0);
	return info;
}

void UISelCharForUServerWnd::OnMouseMove(int x, int y)
{
	ShowCharInfoBalloon(x, y, (unsigned long)-1);
	UIFrameWnd::OnMouseMove(x, y);
}

void UISelCharForUServerWnd::ShowCharInfoBalloon(int x, int y, unsigned long fntColor)
{
	int color[3];
	color[0] = (fntColor >> 16) & 0xff;
	color[1] = (fntColor >> 8) & 0xff;
	color[2] = fntColor & 0xff;

	if (!m_charInfoBalloon) {
		m_charInfoBalloon = new UICharInfoBalloonText;
		m_charInfoBalloon->Create(140, 200);
		g_windowMgr.AddWindow(m_charInfoBalloon);
	}

	m_charInfoBalloon->Move(-400, -400);
	{
		int textW;

		m_strings.clear();

		CHARACTER_INFO* charInfo = GetCharInfo(x, y);
		if (charInfo) {
			char nameBuf[25];
			memcpy(nameBuf, charInfo->name, 24);
			nameBuf[24] = 0;

			textW = GetTextWidth(nameBuf, 0, 0, 12, false) + 60;

			char buf[100];
			m_strings.push_back("Name");
			m_strings.push_back(nameBuf);
			m_strings.push_back("Job");
			m_strings.push_back(g_session.m_jobNameTable[charInfo->job]);
			m_strings.push_back("Lv. ");
			m_strings.push_back(itoa(charInfo->level, buf, 10));
			m_strings.push_back("EXP");
			m_strings.push_back(itoa(charInfo->exp, buf, 10));
			m_strings.push_back("HP");
			m_strings.push_back(itoa(charInfo->hp, buf, 10));
			m_strings.push_back("SP");
			m_strings.push_back(itoa(charInfo->sp, buf, 10));
			m_strings.push_back("STR");
			m_strings.push_back(itoa(charInfo->Str, buf, 10));
			m_strings.push_back("AGI");
			m_strings.push_back(itoa(charInfo->Agi, buf, 10));
			m_strings.push_back("VIT");
			m_strings.push_back(itoa(charInfo->Vit, buf, 10));
			m_strings.push_back("INT");
			m_strings.push_back(itoa(charInfo->Int, buf, 10));
			m_strings.push_back("DEX");
			m_strings.push_back(itoa(charInfo->Dex, buf, 10));
			m_strings.push_back("LUK");
			m_strings.push_back(itoa(charInfo->Luk, buf, 10));
		} else {
			g_windowMgr.PostQuit(m_charInfoBalloon);
			m_charInfoBalloon = NULL;
			return;
		}

		if (textW < 140)
			textW = 140;
		m_charInfoBalloon->Resize(textW, 200);
		m_charInfoBalloon->SetFntColor(RGB(color[0], color[1], color[2]), 0);
		m_charInfoBalloon->SetTransBoxColor(0x88000000);
		m_charInfoBalloon->SetText(m_strings);
	}

	int gx = x + 20;
	int gy = y;
	GetGlobalCoor(gx, gy);

	if (gx < -3)
		gx = -3;
	if (gx > g_renderer->m_width - m_charInfoBalloon->GetWidth() + 3)
		gx = g_renderer->m_width - m_charInfoBalloon->GetWidth() + 3;
	if (gy < -3)
		gy = -3;
	if (gy > g_renderer->m_height - m_charInfoBalloon->GetHeight() + 3)
		gy = g_renderer->m_height - m_charInfoBalloon->GetHeight() + 3;

	m_charInfoBalloon->Move(gx, gy);
}

UIChangeNameWnd::UIChangeNameWnd() {}

UIChangeNameWnd::~UIChangeNameWnd() {}

void UIChangeNameWnd::OnCreate(int cx, int cy)
{
	{
		std::string pathName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\");

		RAG_BUDGET_FILL(11)
		m_okButton = new UIBitmapButton;
		m_okButton->SetBitmapName((pathName + "btn_ok" + ".bmp").c_str(), BS_NORMAL);
		m_okButton->SetBitmapName((pathName + "btn_ok_a" + ".bmp").c_str(), BS_MOUSEON);
		m_okButton->SetBitmapName((pathName + "btn_ok_b" + ".bmp").c_str(), BS_PRESSED);
		m_okButton->Create(m_okButton->GetBitmapWidth(), m_okButton->GetBitmapHeight());
		m_okButton->Move(m_w - 93, m_h - 24);
		m_okButton->SetId(ID_OK);
		AddChild(m_okButton);
	}

	{
		std::string pathName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\");

		m_cancelButton = new UIBitmapButton;
		m_cancelButton->SetBitmapName((pathName + "btn_cancel" + ".bmp").c_str(), BS_NORMAL);
		m_cancelButton->SetBitmapName((pathName + "btn_cancel_a" + ".bmp").c_str(), BS_MOUSEON);
		m_cancelButton->SetBitmapName((pathName + "btn_cancel_b" + ".bmp").c_str(), BS_PRESSED);
		m_cancelButton->Create(m_cancelButton->GetBitmapWidth(), m_cancelButton->GetBitmapHeight());
		m_cancelButton->Move(m_w - 46, m_h - 24);
		m_cancelButton->SetId(ID_CANCEL);
		AddChild(m_cancelButton);
	}

	CMode* mode = g_modeMgr.GetCurMode();
	mode->SendMsg(LMM_GET_MAKING_CHAR_NAME, 0, 0, 0);

	m_nameEditCtrl = new UIBitmapEditCtrl;
	m_nameEditCtrl->SetBitmap("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\login_interface\\name-edit.bmp");
	m_nameEditCtrl->Create(m_nameEditCtrl->GetWidth(), m_nameEditCtrl->GetHeight());
	m_nameEditCtrl->Move(60, 44);
	m_nameEditCtrl->SetTextLimit(NAME_LENGTH);
	m_nameEditCtrl->SetId(ID_OK);
	AddChild(m_nameEditCtrl);
	g_windowMgr.SetFocusEdit(m_nameEditCtrl);

	m_defPushId = ID_OK;
}

void UIChangeNameWnd::OnDraw()
{
	DrawBitmap(0, 0, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\login_interface\\change_name.bmp"), false), false);
}

int UIChangeNameWnd::SendMsg(UIWindow * sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_COMMAND:
		switch (val1) {
		case ID_OK:
			if (m_nameEditCtrl->GetTextSize() >= 4) {
				BOOL isGood = g_insultFilter.IsGoodSentence(m_nameEditCtrl->GetText());
				BOOL isBadNameWithSpaceChar = g_session.IsBadNameWithSpaceChar(m_nameEditCtrl->GetText());

				if (isGood && !isBadNameWithSpaceChar) {
					if (m_nameEditCtrl->GetTextSize() >= NAME_LENGTH) {
						g_windowMgr.ErrorMsg(MsgStr(MSI_NAMELENGTH_TOO_LONG), ME_OK, TRUE, 0, 0);
						m_nameEditCtrl->SetText("");
						break;
					}

					char tmpName[NAME_LENGTH];
					memset(tmpName, 0, NAME_LENGTH);
					strcpy(tmpName, m_nameEditCtrl->GetText());
					memcpy(&g_charInfo, m_charInfo, sizeof(CHARACTER_INFO));
					memcpy(g_charInfo.name, tmpName, sizeof(g_charInfo.name));

					if (strstr((const char*)g_charInfo.name, "\xb1\xd7\xb6\xf3\xba\xf1\xc6\xbc"))
						break;

					strlwr(tmpName);
					if (strstr(tmpName, "gravity"))
						break;

					if (tmpName[0] == '[' && tmpName[1] == 'g' && tmpName[2] == 'm')
						break;
					if (tmpName[1] == '[' && tmpName[2] == 'g' && tmpName[3] == 'm')
						break;
					if (tmpName[2] == '[' && tmpName[3] == 'g' && tmpName[4] == 'm')
						break;

					PACKET_CH_REQ_IS_VALID_CHARNAME p;
					p.PacketType = HEADER_CH_REQ_IS_VALID_CHARNAME;
					p.dwAID = g_session.GetAid();
					DWORD gid = m_charInfo->GID;
					g_session.m_changeNameGID = gid;
					p.dwGID = gid;
					memcpy(p.szCharName, g_charInfo.name, sizeof(p.szCharName));
					memcpy(g_session.m_changeCharName, g_charInfo.name, sizeof(g_charInfo.name));
					CRagConnection::instanceR().SendPacket(CRagConnection::instanceR().GetPacketSize(HEADER_CH_REQ_IS_VALID_CHARNAME), (const char*)&p);
					g_windowMgr.PostQuit(this);
					break;
				}

				if (!isGood)
					g_windowMgr.ErrorMsg(MsgStr(MSI_BAD_NAME), ME_OK, TRUE, 0, 0);
				if (isBadNameWithSpaceChar)
					g_windowMgr.ErrorMsg(MsgStr(MSI_NO_SPACE_IN_NAME), ME_OK, TRUE, 0, 0);
			} else {
				g_windowMgr.ErrorMsg(MsgStr(MSI_ENTER_NAME_MORE_THAN_4_CHAR), ME_OK, TRUE, 0, 0);
			}
			break;
		case ID_CANCEL:
			g_windowMgr.PostQuit(this);
			break;
		}
		break;
	case UM_SETINFO:
		m_charInfo = (CHARACTER_INFO*)val1;
		m_nameEditCtrl->SetText((const char*)m_charInfo->name);
		break;
	case UM_DEFPUSH:
		SendMsg(NULL, UM_COMMAND, m_defPushId, 0, 0, 0);
		break;
	}

	return 0;
}

UIRebirthWnd::UIRebirthWnd() {}

UIRebirthWnd::~UIRebirthWnd() {}

void UIRebirthWnd::OnCreate(int cx, int cy)
{
	std::string pathName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\");

	char* buttonName[5][5] = {
		{"esc_05a", "esc_05b", "esc_05c"},
		{"esc_04a", "esc_04b", "esc_04c"},
		{"esc_01a", "esc_01b", "esc_01c"},
		{"esc_03a", "esc_03b", "esc_03c"},
		{"esc_02a", "esc_02b", "esc_02c"},
	};
	int ids[5] = {ID_REBIRTH, ID_RESTART, ID_TO_CHARACTER_SELECT, ID_QUIT, ID_CANCEL};

	int pos[5][4] = {
		{29, 22},
		{29, 45},
		{29, 68},
		{29, 91},
		{29, 119},
	};

	for (int i = 0; i < 5; ++i) {
		UIBitmapButton* button = new UIBitmapButton;
		button->SetBitmapName((pathName + buttonName[i][0] + ".bmp").c_str(), BS_NORMAL);
		button->SetBitmapName((pathName + buttonName[i][1] + ".bmp").c_str(), BS_MOUSEON);
		button->SetBitmapName((pathName + buttonName[i][2] + ".bmp").c_str(), BS_PRESSED);
		button->Create(button->GetBitmapWidth(), button->GetBitmapHeight());
		button->Move(pos[i][0], pos[i][1]);
		button->SetId(ids[i]);
		AddChild(button);
	}

	m_defPushId = 0xba;
}

void UIRebirthWnd::OnDraw()
{
	CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\win_msgbox1.bmp"), false);
	DrawBitmap(0, 0, bmp, false);
}

int UIRebirthWnd::SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	CGameMode* gameMode = g_modeMgr.GetGameMode();

	switch (message) {
	case UM_COMMAND:
		switch (val1) {
		case ID_REBIRTH:
			gameMode->SendMsg(MM_REBIRTH, 0, 0, 0);
			g_windowMgr.DeleteWindow(WID_REBIRTHWND);
			break;
		case ID_RESTART:
			gameMode->SendMsg(MM_RESTART, 0, 0, 0);
			g_windowMgr.DeleteWindow(WID_REBIRTHWND);
			break;
		case ID_TO_CHARACTER_SELECT:
			gameMode->SendMsg(MM_RESTART, 1, 0, 0);
			g_windowMgr.DeleteWindow(WID_REBIRTHWND);
			break;
		case ID_QUIT:
			g_windowMgr.DeleteWindow(WID_REBIRTHWND);
			gameMode->SendMsg(MM_QUIT_GAME, 0, 0, 0);
			break;
		case ID_CANCEL:
			g_windowMgr.DeleteWindow(WID_REBIRTHWND);
			break;
		}
		break;
	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}

UINewChatWnd::UINewChatWnd()
{
	m_commonChat = NULL;
	m_whisperChat = NULL;
	m_lastFocusChat = NULL;
	m_resizer = NULL;
	m_listBox = NULL;
	m_cmdListBox = NULL;
	m_state = 0;
	m_comboMode = 0;
	m_tab = 0;
	m_saveState = 0;
	m_titleHeight = 16;
	m_titleWidth = 71;
	m_chatEditHeight = 24;
	m_colorTab1 = 0x66000000;
	m_colorTab2 = 0x99000000;

	switch (g_serviceType) {
	case ServiceGermany:
		m_titleWidth = 71;
		break;
	case ServiceFrance:
		m_titleWidth = 90;
		break;
	case ServiceAmerica:
	case ServicePhilippine:
	case ServiceIndia:
		m_titleWidth = 110;
		break;
	}
}

UINewChatWnd::~UINewChatWnd() {}

void UINewChatWnd::OnCreate(int cx, int cy)
{
	int v;
	if (cy <= m_chatEditHeight) {
		m_state = 0;
	} else {
		v = m_titleHeight + m_state + 74;
		m_state = (cy - v) / 42 + 1;
		if (m_state > 4)
			m_state = 4;
	}

	{
		std::string pathName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\stickoff");

		m_btnStickOff = new UIBitmapButton;
		m_btnStickOff->SetBitmapName((pathName + "_a.bmp").c_str(), BS_NORMAL);
		m_btnStickOff->SetBitmapName((pathName + "_b.bmp").c_str(), BS_MOUSEON);
		m_btnStickOff->SetBitmapName((pathName + "_b.bmp").c_str(), BS_PRESSED);
		m_btnStickOff->Create(m_btnStickOff->GetBitmapWidth(), m_btnStickOff->GetBitmapHeight());
		m_btnStickOff->Move(m_titleWidth * 2 + 4, 4);
		m_btnStickOff->SetId(ID_STICK_OFF);
		AddChild(m_btnStickOff);
	}

	{
		std::string pathName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\battle_option");

		m_btnOption = new UIBitmapButton;
		m_btnOption->SetBitmapName((pathName + "_a.bmp").c_str(), BS_NORMAL);
		m_btnOption->SetBitmapName((pathName + "_b.bmp").c_str(), BS_MOUSEON);
		m_btnOption->SetBitmapName((pathName + "_b.bmp").c_str(), BS_PRESSED);
		m_btnOption->Create(m_btnOption->GetBitmapWidth(), m_btnOption->GetBitmapHeight());
		m_btnOption->Move(m_titleWidth * 2 + 19, 5);
		m_btnOption->SetId(ID_OPTION);
		AddChild(m_btnOption);
	}

	if (!g_windowMgr.DeleteWindow(WID_SUBCHATWND_ST))
		g_windowMgr.MakeWindow(WID_SUBCHATWND_ST);
	if (!g_windowMgr.DeleteWindow(WID_SUBCHATWND_BT))
		g_windowMgr.MakeWindow(WID_SUBCHATWND_BT);
	if (!g_windowMgr.DeleteWindow(WID_SUBCHAT_MINIWND))
		g_windowMgr.MakeWindow(WID_SUBCHAT_MINIWND);

	m_commonChat = new UIChatEditCtrl;
	m_commonChat->Create(460, 16);
	m_commonChat->Move(110, m_h - m_chatEditHeight + 4);
	m_commonChat->SetTextLimit(70);
	m_commonChat->SetFrameColor(0xf0, 0xf0, 0xf0);
	AddChild(m_commonChat);
	m_lastFocusChat = m_commonChat;
	g_windowMgr.SetFocusEdit(m_commonChat);

	m_whisperChat = new UIWhisperEditCtrl;
	m_whisperChat->Create(90, 16);
	m_whisperChat->Move(6, m_h - m_chatEditHeight + 4);
	m_whisperChat->SetTextLimit(24);
	m_whisperChat->SetFrameColor(0xf0, 0xf0, 0xf0);
	AddChild(m_whisperChat);

	m_defPushId = 0x76;

	m_resizer = new UIResizer;
	m_resizer->SetBitmap("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\dialog_mid.bmp");
	m_resizer->Create(m_resizer->GetWidth(), m_resizer->GetHeight());
	m_resizer->SetType(RT_LEFT);
	m_resizer->Move(2, m_h - m_resizer->GetHeight());
	AddChild(m_resizer);

	std::string pathName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\");
	char* buttonName[3][4] = {
		{"dialog_btn0", "dialog_btn1", "dialog_btn2"},
		{"sys_base_off", "sys_base_on", "sys_base_on"},
		{"sys_base_off", "sys_base_on", "sys_base_on"},
	};
	int ids[3] = {ID_WHISPERCOMBO, ID_HIS, ID_TALKTOWHOCOMBO};
	int pos[3][2] = {
		{97, m_h - m_chatEditHeight + 3},
		{586, m_h - m_chatEditHeight + 7},
		{572, m_h - m_chatEditHeight + 7},
	};

	for (int i = 0; i < 3; ++i) {
		m_buttons[i] = new UIBitmapButton;
		m_buttons[i]->SetBitmapName((pathName + buttonName[i][0] + ".bmp").c_str(), BS_NORMAL);
		m_buttons[i]->SetBitmapName((pathName + buttonName[i][1] + ".bmp").c_str(), BS_MOUSEON);
		m_buttons[i]->SetBitmapName((pathName + buttonName[i][2] + ".bmp").c_str(), BS_PRESSED);
		m_buttons[i]->Create(m_buttons[i]->GetBitmapWidth(), m_buttons[i]->GetBitmapHeight());
		m_buttons[i]->Move(pos[i][0], pos[i][1]);
		m_buttons[i]->SetId(ids[i]);
		AddChild(m_buttons[i]);
	}

	m_listBox = new UIChatHisBox;
	m_listBox->Create2(0, 0, m_w - 8, 70, false);
	if (cy - 70 >= m_titleHeight + m_chatEditHeight)
		m_listBox->Move(5, m_titleHeight + 2);
	else
		m_listBox->Move(5, -70);
	m_listBox->SetId(ID_OK);
	AddChild(m_listBox);

	m_cmdListBox = new UIChatHisBox;
	m_cmdListBox->Create2(0, 0, m_w - 8, 70, false);
	m_cmdListBox->Move(-100 - m_w, -100);
	AddChild(m_cmdListBox);

	UIResizer* resizer = new UIResizer;
	resizer->Create(m_w - 26, 6);
	resizer->SetType(RT_UP);
	resizer->Move(3, m_titleHeight);
	resizer->SetColor(0xffff00ff, 0xffff00ff);
	AddChild(resizer);

	int gx = 0;
	int gy = 0;
	GetGlobalCoor(gx, gy);
	m_boxInfo.x = gx + 3;
	m_boxInfo.color = 0x66000000;
	m_boxInfo.y = gy + m_titleHeight;
	m_boxInfo.cx = m_w - 6;
	m_boxInfo.color2 = 0;
	m_boxInfo.cy = m_h - m_titleHeight - m_chatEditHeight;
	m_boxInfo.drawEdge = 0;

	g_windowMgr.ReadToReg();
}

void UINewChatWnd::OnMouseMove(int x, int y)
{
	if (g_windowMgr.GetCapture() == this) {
		UIFrameWnd::OnMouseMove(x, y);
	}
}

void UINewChatWnd::OnLBtnDown(int x, int y)
{
	if (y <= 0 || y >= 21) {
		UIFrameWnd::OnLBtnDown(x, y);
		return;
	}
	int oldTab = m_tab;
	if (x > 0 && x < m_titleWidth - 1 && oldTab != 0) {
		m_tab = 0;
	} else if (x > m_titleWidth + 1 && x < m_titleWidth * 2 - 1 && oldTab != 1) {
		m_tab = 1;
	}
	if (oldTab != m_tab)
		ChangeTab();
}

void UINewChatWnd::OnLBtnUp(int x, int y)
{
	UIFrameWnd::OnLBtnUp(x, y);
}

bool UINewChatWnd::GetTransBoxInfo2(BOXINFO* boxInfo)
{
	if (m_state) {
		*boxInfo = m_boxInfo;
		return true;
	}
	return false;
}

void UINewChatWnd::DrawBoxScreen2()
{
	int gx = 3;
	int gy = 0;
	GetGlobalCoor(gx, gy);
	DrawBoxScreen(gx + 1, gy + 1, m_titleWidth - 2, m_titleHeight, m_colorTab1);
	DrawBoxScreen(gx + m_titleWidth + 1, gy + 1, m_titleWidth - 2, m_titleHeight, m_colorTab2);
	DrawBoxScreen(m_boxInfo.x + 1, m_boxInfo.y + 1, m_boxInfo.cx - 2, m_boxInfo.cy - 1, m_boxInfo.color);
}
void UINewChatWnd::SetState(int state)
{
	if (g_windowMgr.GetComboBoxWnd())
		g_windowMgr.DeleteWindow((WINDOWID)0x1a);

	if (g_windowMgr.GetCapture() == this)
		return;

	if (state > 15)
		state = 0;
	else if (state < 0)
		state = 15;

	m_state = state;
	Invalidate();

	int height;
	switch (state) {
	case 0: {
		height = m_chatEditHeight;
		int oldHeight = m_h;
		Resize(m_w, height);
		Move(m_x, m_y + oldHeight - height);
		m_listBox->Move(-100 - m_w, -100);
		m_cmdListBox->Move(-100 - m_w, -100);

		m_whisperChat->Move(6, m_h - m_chatEditHeight + 4);
		m_commonChat->Move(110, m_h - m_chatEditHeight + 4);
		m_resizer->Move(3, m_h - m_resizer->GetHeight());
		m_btnStickOff->Move(m_titleWidth * 2 + 4, 4);
		m_btnOption->Move(m_titleWidth * 2 + 19, 4);

		int pos[3][2] = { { 97, m_h - m_chatEditHeight + 3 },
						  { 586, m_h - m_chatEditHeight + 7 },
						  { 572, m_h - m_chatEditHeight + 7 } };

		for (int i = 0; i < 3; ++i)
			m_buttons[i]->Move(pos[i][0], pos[i][1]);
		break;
	}
	default: {
		height = m_titleHeight + m_chatEditHeight + (state + 3) * 14;
		int oldHeight = m_h;
		Resize(m_w, height);
		Move(m_x, m_y + oldHeight - height);
		m_listBox->Move(-100 - m_w, -100);
		m_cmdListBox->Move(-100 - m_w, -100);

		if (m_tab) {
			m_cmdListBox->Resize(m_w - 8, height - m_titleHeight - m_chatEditHeight - 4);
			m_cmdListBox->Move(5, m_titleHeight + 2);
		} else {
			m_listBox->Resize(m_w - 8, height - m_titleHeight - m_chatEditHeight - 4);
			m_listBox->Move(5, m_titleHeight + 2);
		}

		m_whisperChat->Move(6, m_h - m_chatEditHeight + 4);
		m_commonChat->Move(110, m_h - m_chatEditHeight + 4);
		m_resizer->Move(3, m_h - m_resizer->GetHeight());
		m_btnStickOff->Move(m_titleWidth * 2 + 4, 4);
		m_btnOption->Move(m_titleWidth * 2 + 19, 4);

		int pos[3][2] = { { 97, m_h - m_chatEditHeight + 3 },
						  { 586, m_h - m_chatEditHeight + 7 },
						  { 572, m_h - m_chatEditHeight + 7 } };

		for (int i = 0; i < 3; ++i)
			m_buttons[i]->Move(pos[i][0], pos[i][1]);
	}
	}
}

void UINewChatWnd::Resize(int cx, int cy)
{
	UIWindow::Resize(cx, cy);
	m_boxInfo.cx = m_w - 6;
	m_boxInfo.cy = m_h - m_chatEditHeight - m_titleHeight;
}

void UINewChatWnd::Move(int x, int y)
{
	UIFrameWnd::Move(x, y);
	int gx = 0, gy = 0;
	GetGlobalCoor(gx, gy);
	m_boxInfo.x = gx + 3;
	m_boxInfo.y = gy + m_titleHeight;
	UISubChatMiniWnd* wnd = g_windowMgr.GetSubChatMiniWnd();
	if (wnd)
		wnd->SendMsg(this, UM_COMMAND, 0x83, m_w + x, m_h + y, 0);
}

void UINewChatWnd::OnDraw2()
{
	if (m_h > m_titleHeight + m_chatEditHeight) {
		tagRECT rect = { 5, m_titleHeight + 2, m_w - 6, m_h - m_chatEditHeight - 2 };
		ClearDCRect(0xffff00ff, rect);

		tagRECT rect2 = { 0, 0, m_titleWidth * 2 + 2, m_titleHeight + 1 };
		ClearDCRect(0xffff00ff, rect2);
	}

	if (!g_windowMgr.IsChatWndStickOn() || m_h <= m_titleHeight + m_chatEditHeight)
		return;

	unsigned long colorRef = 0xffffff;
	unsigned long colorTab1;
	unsigned long colorTab2;

	if (m_tab == 0) {
		colorTab1 = 0xffffff;
		colorTab2 = 0x8e8e8e;
		TextOutA(8, 4, MsgStr(MSI_ST_CHAT), 0, 0, 12, 0);
		DrawBox(m_titleWidth + 2, m_titleHeight, m_titleWidth + 1, 1, colorRef);
		DrawBox(3, m_titleHeight, 1, 1, colorRef);
	} else if (m_tab == 1) {
		colorTab1 = 0x8e8e8e;
		colorTab2 = 0xffffff;
		TextOutA(m_titleWidth + 9, 4, MsgStr(MSI_BT_CHAT), 0, 0, 12, 0);
		DrawBox(3, m_titleHeight, m_titleWidth + 1, 1, colorRef);
	}

	TextOutWithDecoration(7, 3, MsgStr(MSI_ST_CHAT), 0, &colorRef, 0, 12);
	TextOutWithDecoration(m_titleWidth + 8, 3, MsgStr(MSI_BT_CHAT), 0, &colorRef, 0, 12);

	DrawBox(5, 0, m_titleWidth - 5, 1, colorTab1);
	DrawBox(4, 1, 1, 1, colorTab1);
	DrawBox(3, 2, 1, m_titleHeight - 2, colorTab1);
	DrawBox(m_titleWidth, 0, 1, 1, colorTab1);
	DrawBox(m_titleWidth + 1, 1, 1, 1, colorTab1);
	DrawBox(m_titleWidth + 2, 2, 1, m_titleHeight - 2, colorTab1);

	DrawBox(m_titleWidth + 5, 0, m_titleWidth - 5, 1, colorTab2);
	DrawBox(m_titleWidth + 4, 1, 1, 1, colorTab2);
	DrawBox(m_titleWidth + 3, 2, 1, m_titleHeight - 2, colorTab2);
	DrawBox(m_titleWidth * 2, 0, 1, 1, colorTab2);
	DrawBox(m_titleWidth * 2 + 1, 1, 1, 1, colorTab2);
	DrawBox(m_titleWidth * 2 + 2, 2, 1, m_titleHeight - 2, colorTab2);
}

void UINewChatWnd::OnDraw()
{
	if (g_windowMgr.IsChatWndStickOn() && m_h > m_titleHeight + m_chatEditHeight) {
		DrawBox(3, m_titleHeight, 1, m_h - m_titleHeight - m_chatEditHeight, 0xffffff);
		DrawBox(m_titleWidth * 2 + 3, m_titleHeight, m_w - (m_titleWidth * 2 + 10), 1, 0xffffff);
		DrawBox(m_w - 6, m_titleHeight, 1, 1, 0x8e8e8e);
		DrawBox(m_w - 5, m_titleHeight + 1, 1, 1, 0x8e8e8e);
		DrawBox(m_w - 4, m_titleHeight + 2, 1, m_h - m_chatEditHeight - m_titleHeight - 2, 0x8e8e8e);
	}

	if (g_session.m_isBattle == 1 && g_session.m_isBmChat != 1) {
		CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\dialog_bg2.bmp"), false);
		DrawBitmap(0, m_h - bmp->m_height, bmp, false);
		return;
	}

	CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\dialog_bg.bmp"), false);
	DrawBitmap(0, m_h - bmp->m_height, bmp, false);
}

void UINewChatWnd::StoreInfo()
{
	g_windowMgr.m_chatWndStatus = m_state;
	g_windowMgr.m_chatWndHeight = m_h;
	g_windowMgr.m_chatWndX = m_x;
	g_windowMgr.m_chatWndY = m_y;
}

void UINewChatWnd::SetShow(int visible)
{
	m_show = visible;
	if (visible) {
		Invalidate();
		g_windowMgr.UpdateSnapInfo(this);
	} else {
		g_windowMgr.DeleteSnapInfo(this);
	}
	m_listBox->SetShow(m_show);
	m_cmdListBox->SetShow(m_show);
	StickOnOff(g_windowMgr.IsChatWndStickOn());
}

void UINewChatWnd::WriteChatToFile()
{
	char tmp[2048];
	int captureCnt = 0;

	sprintf(tmp, "%s\\Chat", g_baseDir3);
	if (access(tmp, 0)) {
		if (_mkdir(tmp))
			return;
	}

	sprintf(tmp, "%s\\Chat\\Chat.txt", g_baseDir3);
	while (access(tmp, 0) == 0)
		sprintf(tmp, "%s\\Chat\\Chat%03d.txt", g_baseDir3, ++captureCnt);

	if (m_listBox->WriteChatToFile(tmp)) {
		std::string chatStoreMsg = tmp;
		chatStoreMsg += " is Saved.";
		g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
		g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (int)chatStoreMsg.c_str(), 0xff00, 0, 0);
	}

	captureCnt = 0;
	sprintf(tmp, "%s\\Chat_BM", g_baseDir3);
	if (access(tmp, 0)) {
		if (_mkdir(tmp))
			return;
	}

	sprintf(tmp, "%s\\Chat_BM\\Chat_BM.txt", g_baseDir3);
	while (access(tmp, 0) == 0)
		sprintf(tmp, "%s\\Chat_BM\\Chat%03d_BM.txt", g_baseDir3, ++captureCnt);

	if (m_cmdListBox->WriteChatToFile(tmp)) {
		std::string chatStoreMsg = tmp;
		chatStoreMsg += " is Saved.";
		g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
		g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (int)chatStoreMsg.c_str(), 0xff00, 0, 0);
	}
}

int UINewChatWnd::SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_GET_EDIT_FOCUS:
		g_windowMgr.SetFocusEdit(m_commonChat);
		break;

	case UM_DELETE_WHISPER_NAME: {
		std::string name((const char*)val1);
		int size = m_whisperChat->m_hisChat.size();
		std::vector<std::string>::iterator it = m_whisperChat->m_hisChat.begin();
		int i = 0;
		while (it != m_whisperChat->m_hisChat.end()) {
			if (*it == name) {
				m_whisperChat->m_hisChat.erase(it);

				if (m_whisperChat->m_curHis == i)
					m_whisperChat->m_curHis = size - 1;
				else if (m_whisperChat->m_curHis > i)
					--m_whisperChat->m_curHis;
				break;
			}
			++it;
			++i;
		}
		break;
	}

	case UM_COMBOBOX_SELECTED:
		switch (m_comboMode) {
		case 0: {
			int whisperSize = m_whisperChat->m_hisChat.size();
			if (val1 >= 0 && val1 <= whisperSize) {
				if (val1 == whisperSize) {
					m_whisperChat->SetText("");
				} else {
					m_whisperChat->SetText(m_whisperChat->m_hisChat[val1].c_str());
				}
				m_whisperChat->m_curHis = val1;
			}
			g_windowMgr.SetFocusEdit(m_whisperChat);
			g_windowMgr.SetFocusEdit(m_commonChat);
			break;
		}

		case 1:
			g_session.m_talkType = val1;
			switch (val1) {
			case 0:
				m_commonChat->SetTextColor(0, 0, 0);
				break;
			case 1:
				m_commonChat->SetTextColor(0x80, 0, 0x80);
				break;
			case 2:
				m_commonChat->SetTextColor(0, 0x80, 0x80);
				break;
			}
			Invalidate();
			break;
		}
		break;

	case UM_ADDCHAT:
	case UM_ADDCHAT2: {
		UISubChatHisWnd* subChatWndST = g_windowMgr.GetSubChatWndST();
		UISubChatHisWnd* subChatWndBT = g_windowMgr.GetSubChatWndBT();
		std::vector<std::string> strings;
		char talkerName[25] = { 0 };
		if (val4)
			strcpy(talkerName, (const char*)val4);

		unsigned char allShow = 1;
		switch (val3) {
		case 1:
		case 3:
		case 4: {
			const char* msg = (const char*)val1;
			std::string wMsg(msg);
			int len = strlen(msg);
			int pos = wMsg.find(" :");
			if (pos > 0 && pos < len) {
				memcpy(talkerName, msg, pos);
				talkerName[pos] = 0;
			}
		}
		break;
		case 8:
		case 9:
			allShow = 0;
			break;
		}

		if (g_serviceType == ServiceGermany || g_serviceType == ServiceVietnam) {
			if (strlen(talkerName) > 24)
				break;
		}

		if (strcmp(talkerName, g_session.GetCharName()) == 0)
			talkerName[0] = 0;

		CutString((const char*)val1, strings, 94);
		for (int i = 0; i < strings.size(); ++i) {
			if (message == UM_ADDCHAT && allShow) {
				m_listBox->AddItem(strings[i].c_str(), val2, talkerName);
				if (subChatWndST)
					subChatWndST->SendMsg(this, UM_ADDCHAT, val1, val2, (int)talkerName, 0);
			}

			if (message == UM_ADDCHAT2 || (val3 && g_session.m_battleMsgOnOffSet[val3 - 1])) {
				m_cmdListBox->AddItem(strings[i].c_str(), val2, talkerName);
				if (subChatWndBT)
					subChatWndBT->SendMsg(this, UM_ADDCHAT, val1, val2, (int)talkerName, 0);
			}
		}
		break;
	}

	case UM_SETINFO: {
		bool outOfScreen = false;
		if (g_windowMgr.m_chatWndX < 0)
			outOfScreen = true;
		if (g_windowMgr.m_chatWndY < 0)
			outOfScreen = true;
		if (g_windowMgr.m_chatWndX > g_renderer->m_width - m_w)
			outOfScreen = true;
		if (g_windowMgr.m_chatWndY > g_renderer->m_height - m_h)
			outOfScreen = true;

		if (outOfScreen) {
			g_windowMgr.m_chatWndX = 0;
			g_windowMgr.m_chatWndY = g_renderer->m_height - m_h;
		}

		m_x = g_windowMgr.m_chatWndX;
		m_y = g_windowMgr.m_chatWndY;
		SetState(g_windowMgr.m_chatWndStatus);
		Move(m_x, m_y);
		break;
	}

	case UM_COMMAND:
		switch (val1) {
		case ID_OPTION:
			if (!g_windowMgr.DeleteWindow(WID_BATTLEMSG_OPTIONWND))
				g_windowMgr.MakeWindow(WID_BATTLEMSG_OPTIONWND);
			break;

		case ID_NAME:
			g_windowMgr.AddWhisperCharName2((const char*)val2);
			break;

		case ID_STICK_ON:
			StickOnOff(true);
			break;

		case ID_STICK_OFF:
			StickOnOff(false);
			break;

		case ID_TALKTOWHOCOMBO: {
			m_comboMode = 1;
			if (g_windowMgr.GetComboBoxWnd())
				g_windowMgr.DeleteWindow(WID_COMBOBOXWND);

			UIFrameWnd* comboBoxWnd = g_windowMgr.MakeWindow(WID_COMBOBOXWND);
			comboBoxWnd->SendMsg(NULL, UM_ADDITEM, (int)MsgStr(MSI_SEND_TO_ALL), 0, 0, 0);
			comboBoxWnd->SendMsg(NULL, UM_ADDITEM, (int)MsgStr(MSI_SEND_TO_PARTY), 0, 0, 0);
			comboBoxWnd->SendMsg(NULL, UM_ADDITEM, (int)MsgStr(MSI_SEND_TO_GUILD), 0, 0, 0);
			comboBoxWnd->SendMsg(NULL, UM_SETCURITEM, g_session.m_talkType, 0, 0, 0);
			comboBoxWnd->Resize(112, 48);
			comboBoxWnd->SendMsg(NULL, UM_SET_CALLER_WID, 1, 0, 0, 0);

			int gx = m_w - 115;
			int gy = m_h - m_chatEditHeight;
			GetGlobalCoor(gx, gy);
			comboBoxWnd->Move(gx + 1, gy - 47);
			break;
		}

		case ID_WHISPERCOMBO: {
			m_comboMode = 0;
			if (g_windowMgr.GetComboBoxWnd())
				g_windowMgr.DeleteWindow(WID_COMBOBOXWND);

			int whisperSize = m_whisperChat->m_hisChat.size();
			if (whisperSize) {
				UIFrameWnd* comboBoxWnd = g_windowMgr.MakeWindow(WID_COMBOBOXWND);
				for (int i = 0; i < whisperSize; ++i)
					comboBoxWnd->SendMsg(NULL, UM_ADDITEM, (int)m_whisperChat->m_hisChat[i].c_str(), 0, 0, 0);
				comboBoxWnd->SendMsg(NULL, UM_ADDITEM, (int)"", 0, 0, 0);

				int comboHeight = (whisperSize + 1) * 16 < 80 ? (whisperSize + 1) * 16 : 80;

				comboBoxWnd->SendMsg(NULL, UM_SETCURITEM, m_whisperChat->m_curHis, 0, 0, 0);
				comboBoxWnd->Resize(112, comboHeight);
				comboBoxWnd->SendMsg(NULL, UM_SET_CALLER_WID, 1, 0, 0, 0);

				int gx = 0;
				int gy = m_h - m_chatEditHeight;
				GetGlobalCoor(gx, gy);
				comboBoxWnd->Move(gx + 1, gy - comboHeight + 1);
			} else {
				g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
				g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (int)MsgStr(MSI_NOWHISPERLIST), 0xff, 0, 0);
			}
			break;
		}

		case ID_OK: {
			char chatBuf[256];
			strcpy(chatBuf, m_commonChat->GetText());
			m_commonChat->StoreInHistory();
			m_lastFocusChat = m_commonChat;
			g_windowMgr.SetFocusEdit(m_commonChat);

			if (strlen(chatBuf) == 0)
				break;

			if (g_serviceType == ServiceAmerica && !g_session.IsOnlyEnglish(chatBuf)) {
				g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (int)MsgStr(MSI_ENGLISHONLY), 0xff, 0, 0);
				break;
			}

			if (m_whisperChat->GetTextSize() != 0 && chatBuf[0] != '/') {
				g_windowMgr.SetChatMsg(chatBuf);
				g_modeMgr.GetCurMode()->SendMsg(MM_WHISPERMSG, (int)m_whisperChat->GetText(), 0, 0);
				break;
			}

			std::string param[3];
			TALKTYPE talkType;
			int chatStartOffset = g_session.GetTalkType(chatBuf, &talkType, param);
			if (chatStartOffset != -1)
				g_windowMgr.SetChatMsg(&chatBuf[chatStartOffset]);

			g_modeMgr.GetCurMode()->SendMsg(MM_PROCESS_TALK_TYPE, talkType, (int)param, 0);
			break;
		}

		case ID_HIS:
			SendMsg(NULL, UM_TOGGLEPUSH, 0, 0, 0, 0);
			break;
		}
		break;

	case UM_RESIZE:
		switch (val1) {
		case 2:
			if (g_windowMgr.IsChatWndStickOn()) {
				int top = m_titleHeight + 56;
				if (val3 >= top) {
					int state = (val3 - top) / 14 + 1;
					if (state > 15)
						state = 15;
					if (m_state != state)
						SetState(state);
				} else if (m_state) {
					SetState(0);
				}
			}
			break;
		}
		break;

	case UM_TOGGLEPUSH:
		if (g_windowMgr.IsChatWndStickOn()) {
			int dir = val1 ? -1 : 1;
			int state;
			if (m_state)
				state = m_state + dir * 3;
			else
				state = dir;
			if (state > 0)
				state += 1 - state % 3;
			else if (m_state)
				state = 0;
			SetState(state);
		}
		break;

	case UM_PAGEDOWNPUSH:
		if (m_tab && m_cmdListBox->IsVertScrEnabled())
			m_cmdListBox->SendMsg(NULL, UM_SCROLLVERTPAGEDOWN, 0, 0, 0, 0);
		else if (!m_tab && m_listBox->IsVertScrEnabled())
			m_listBox->SendMsg(NULL, UM_SCROLLVERTPAGEDOWN, 0, 0, 0, 0);
		break;

	case UM_PAGEUPPUSH:
		if (m_tab && m_cmdListBox->IsVertScrEnabled())
			m_cmdListBox->SendMsg(NULL, UM_SCROLLVERTPAGEUP, 0, 0, 0, 0);
		else if (!m_tab && m_listBox->IsVertScrEnabled())
			m_listBox->SendMsg(NULL, UM_SCROLLVERTPAGEUP, 0, 0, 0, 0);
		break;

	case UM_CHANGESTATUS:
		SetState(val1);
		break;

	case UM_OPENCHATWND:
		if (g_windowMgr.IsChatWndStickOn() && m_state == 0)
			SetState(1);
		break;

	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}

void UINewChatWnd::ChangeTab()
{
	if (m_tab == 0) {
		m_colorTab1 = 0x66000000;
		m_colorTab2 = 0x99000000;
		m_listBox->Resize(m_cmdListBox->GetWidth(), m_cmdListBox->GetHeight());
		m_listBox->Move(m_cmdListBox->GetX(), m_cmdListBox->GetY());
		m_cmdListBox->Move(-100 - m_w, -100);
	} else if (m_tab == 1) {
		m_colorTab1 = 0x99000000;
		m_colorTab2 = 0x66000000;
		m_cmdListBox->Resize(m_listBox->GetWidth(), m_listBox->GetHeight());
		m_cmdListBox->Move(m_listBox->GetX(), m_listBox->GetY());
		m_listBox->Move(-100 - m_w, -100);
	}
	Invalidate();
}

void UINewChatWnd::StickOnOff(bool stickOn)
{
	if (g_windowMgr.IsChatWndStickOn() != stickOn) {
		g_windowMgr.ChatWndStickOnOff(stickOn);
		if (stickOn) {
			m_tab = 0;
			if (!m_saveState)
				m_saveState = 1;
			SetState(m_saveState);
		} else {
			m_saveState = m_state;
			SetState(0);
		}
	}

	if (!m_show)
		stickOn = true;

	UIWindow* linkedWnd = g_windowMgr.GetSubChatMiniWnd();
	if (linkedWnd) {
		linkedWnd->SetShow(!stickOn);
		linkedWnd->SendMsg(this, UM_COMMAND, 0xcd, 0, 0, 0);
	}

	Invalidate();
}

UISubChatHisWnd::UISubChatHisWnd()
{
	m_info = NULL;
	m_resizer = NULL;
	m_listBox = NULL;
	m_titleHeight = 16;
	m_titleWidth = 71;
	m_barHeight = 15;

	switch (g_serviceType) {
	case ServiceGermany:
		m_titleWidth = 71;
		break;
	case ServiceFrance:
		m_titleWidth = 90;
		break;
	case ServiceAmerica:
	case ServicePhilippine:
	case ServiceIndia:
		m_titleWidth = 110;
		break;
	}

	m_chatHistory.reserve(240);
	m_colors.reserve(240);
	m_talkerNameList.reserve(240);
}

UISubChatHisWnd::~UISubChatHisWnd()
{}

void UISubChatHisWnd::OnCreate(int cx, int cy)
{
	{
		std::string pathName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\stickon");

		m_btnStickOn = new UIBitmapButton;
		m_btnStickOn->SetBitmapName((pathName + "_a.bmp").c_str(), BS_NORMAL);
		m_btnStickOn->SetBitmapName((pathName + "_b.bmp").c_str(), BS_MOUSEON);
		m_btnStickOn->SetBitmapName((pathName + "_b.bmp").c_str(), BS_PRESSED);
		m_btnStickOn->Create(m_btnStickOn->GetBitmapWidth(), m_btnStickOn->GetBitmapHeight());
		m_btnStickOn->Move(m_titleWidth + 1, 4);
		m_btnStickOn->SetId(ID_STICK_ON);
		AddChild(m_btnStickOn);
	}

	{
		std::string pathName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\battle_option");

		m_btnOption = new UIBitmapButton;
		m_btnOption->SetBitmapName((pathName + "_a.bmp").c_str(), BS_NORMAL);
		m_btnOption->SetBitmapName((pathName + "_b.bmp").c_str(), BS_MOUSEON);
		m_btnOption->SetBitmapName((pathName + "_b.bmp").c_str(), BS_PRESSED);
		m_btnOption->Create(m_btnOption->GetBitmapWidth(), m_btnOption->GetBitmapHeight());
		m_btnOption->Move(m_titleWidth + 15, 5);
		m_btnOption->SetId(ID_OPTION);
		AddChild(m_btnOption);
	}

	{
		std::string pathName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\wnd_mini");

		m_btnMini = new UIBitmapButton;
		m_btnMini->SetBitmapName((pathName + "_a.bmp").c_str(), BS_NORMAL);
		m_btnMini->SetBitmapName((pathName + "_b.bmp").c_str(), BS_MOUSEON);
		m_btnMini->SetBitmapName((pathName + "_b.bmp").c_str(), BS_PRESSED);
		m_btnMini->Create(m_btnMini->GetBitmapWidth(), m_btnMini->GetBitmapHeight());
		m_btnMini->SetId(ID_MINI);
		AddChild(m_btnMini);
	}

	m_listBox = new UIChatHisBox;
	m_listBox->Create2(0, 0, m_w - 8, m_h - m_barHeight - m_titleHeight - 4, false);
	m_listBox->Move(5, m_titleHeight + 4);
	AddChild(m_listBox);

	m_resizer = new UIResizer;
	m_resizer->Create(14, 14);
	m_resizer->SetType(RT_DOWNRIGHT);
	m_resizer->SetBitmap("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\btn_resize.bmp");
	m_resizer->Move(m_w - 15, m_h - 15);
	m_resizer->SetColor(0xffff00ff, 0xffff00ff);
	AddChild(m_resizer);

	{
	int gx = 0;
	int gy = 0;
	GetGlobalCoor(gx, gy);
	m_boxInfo.x = gx;
	m_boxInfo.y = gy + m_titleHeight;
	m_boxInfo.cx = m_w;
	m_boxInfo.cy = m_h - m_titleHeight;
	m_boxInfo.color = 0x66000000;
	m_boxInfo.color2 = 0;
	m_boxInfo.drawEdge = 0;
	}
}

void UISubChatHisWnd::OnMouseMove(int x, int y)
{
	if (g_windowMgr.GetCapture() == this) {
		UIFrameWnd::OnMouseMove(x, y);
	}
}

void UISubChatHisWnd::DrawBoxScreen2()
{
	int gx = 0, gy = 0;
	GetGlobalCoor(gx, gy);
	DrawBoxScreen(gx + 1, gy + 1, m_titleWidth - 2, m_titleHeight, 0x66000000);
	DrawBoxScreen(m_boxInfo.x + 1, m_boxInfo.y + 1, m_boxInfo.cx - 2, m_boxInfo.cy - 2, m_boxInfo.color);
}

void UISubChatHisWnd::OnLBtnDown(int x, int y)
{
	UIFrameWnd::OnLBtnDown(x, y);
}

void UISubChatHisWnd::OnLBtnUp(int x, int y)
{
	UIFrameWnd::OnLBtnUp(x, y);
}

bool UISubChatHisWnd::GetTransBoxInfo2(BOXINFO* boxInfo)
{
	*boxInfo = m_boxInfo;
	return true;
}

void UISubChatHisWnd::Resize(int cx, int cy)
{
	int oldWidth = m_w;

	UIWindow::Resize(cx, cy);
	m_listBox->Resize(m_w - 8, m_h - m_barHeight - m_titleHeight - 4);
	m_resizer->Move(m_w - 15, m_h - 15);

	if (m_id == 1)
		m_btnMini->Move(m_titleWidth + 26, 5);
	else
		m_btnMini->Move(m_titleWidth + 1, 5);

	if (oldWidth != cx)
		SetMsgListBox();

	m_boxInfo.cx = m_w;
	m_boxInfo.cy = m_h - m_titleHeight;
}

void UISubChatHisWnd::Move(int x, int y)
{
	UIFrameWnd::Move(x, y);
	int gx = 0, gy = 0;
	GetGlobalCoor(gx, gy);
	m_boxInfo.x = gx;
	m_boxInfo.y = m_titleHeight + gy;
}

void UISubChatHisWnd::OnDraw2()
{
	if (m_h > m_titleHeight) {
		tagRECT rect = { m_listBox->GetX(), m_listBox->GetY(), m_w - 3, m_h - m_barHeight };
		ClearDCRect(0xffff00ff, rect);
	}
}

void UISubChatHisWnd::OnDraw()
{
	unsigned long colorRef = 0xffffff;

	if (m_id == 0) {
		TextOutA(5, 4, MsgStr(MSI_ST_CHAT), 0, 0, 12, 0);
		TextOutWithDecoration(4, 3, MsgStr(MSI_ST_CHAT), 0, &colorRef, 0, 12);
	} else if (m_id == 1) {
		TextOutA(5, 4, MsgStr(MSI_BT_CHAT), 0, 0, 12, 0);
		TextOutWithDecoration(4, 3, MsgStr(MSI_BT_CHAT), 0, &colorRef, 0, 12);
	}

	DrawBox(2, 0, m_titleWidth - 5, 1, colorRef);
	DrawBox(1, 1, 1, 1, colorRef);
	DrawBox(0, 2, 1, m_h - 4, colorRef);
	DrawBox(m_titleWidth, m_titleHeight, m_w - m_titleWidth - 3, 1, colorRef);

	DrawBox(m_titleWidth - 3, 0, 1, 1, 0x8e8e8e);
	DrawBox(m_titleWidth - 2, 1, 1, 1, 0x8e8e8e);
	DrawBox(m_titleWidth - 1, 2, 1, m_titleHeight - 1, 0x8e8e8e);
	DrawBox(m_w - 3, m_titleHeight, 1, 1, 0x8e8e8e);
	DrawBox(m_w - 2, m_titleHeight + 1, 1, 1, 0x8e8e8e);
	DrawBox(m_w - 1, m_titleHeight + 2, 1, m_h - m_titleHeight - 4, 0x8e8e8e);
	DrawBox(1, m_h - 2, 1, 1, 0x8e8e8e);
	DrawBox(m_w - 2, m_h - 2, 1, 1, 0x8e8e8e);
	DrawBox(2, m_h - 1, m_w - 4, 1, 0x8e8e8e);
}

void UISubChatHisWnd::DeleteFirstHalfItem()
{
	int tHSize = m_chatHistory.size() / 2;
	for (int i = 0; i < tHSize; ++i)
		m_chatHistory[i] = m_chatHistory[i + tHSize];
	m_chatHistory.resize(tHSize);

	tHSize = m_colors.size() / 2;
	for (int j = 0; j < tHSize; ++j)
		m_colors[j] = m_colors[j + tHSize];
	m_colors.resize(tHSize);

	tHSize = m_talkerNameList.size() / 2;
	for (int k = 0; k < tHSize; ++k)
		m_talkerNameList[k] = m_talkerNameList[k + tHSize];
	m_talkerNameList.resize(tHSize);

	SetMsgListBox();
}

void UISubChatHisWnd::SetMsgListBox()
{
	m_listBox->ClearAllItems();
	for (unsigned int i = 0; i < m_chatHistory.size(); ++i) {
		m_listBox->AddItem2(m_chatHistory[i].c_str(), m_colors[i], m_talkerNameList[i].c_str());
	}
	SendMsg(0, UM_REFRESH, 0, 0, 0, 0);
}

int UISubChatHisWnd::SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_SETINFO:
		m_info = (SubChatWndInfo*)val1;
		if (!m_info->IsValidInfo())
			m_info->MakeDefault(m_id);

		Move(m_info->x, m_info->y);
		Resize(m_info->w, m_info->h);
		if (m_id == 0) {
			m_btnStickOn->Move(-100, -100);
			m_btnOption->Move(-100, -100);
		}
		break;

	case UM_ADDCHAT: {
		if (m_chatHistory.size() >= 240)
			DeleteFirstHalfItem();

		char talkerName[25] = { 0 };
		if (val3)
			strcpy(talkerName, (const char*)val3);

		m_chatHistory.push_back((const char*)val1);
		m_colors.push_back(val2);
		m_talkerNameList.push_back(talkerName);
		m_listBox->AddItem2((const char*)val1, val2, talkerName);
		break;
	}

	case UM_RESIZE:
		if (val1 == 7) {
			RAG_BUDGET_FILL(16)
			int cx = val2;
			int cy = val3;

			if (cx > 512)
				cx = 512;
			if (cy > 384)
				cy = 384;
			if (cx < 280)
				cx = 280;
			if (cy < 74)
				cy = 74;

			cx = ((cx - 280) / 32) * 32 + 280;
			cy = ((cy - 74) / 32) * 32 + 74;

			if (cx != m_w || cy != m_h) {
				Resize(cx, cy);
				m_listBox->Resize(m_w - 8, m_h - m_titleHeight - m_barHeight - 4);
				m_resizer->Move(m_w - 15, m_h - 15);
				SetMsgListBox();
			}
		}
		break;

	case UM_PAGEDOWNPUSH:
		if (m_listBox->IsVertScrEnabled())
			m_listBox->SendMsg(NULL, UM_SCROLLVERTPAGEDOWN, 0, 0, 0, 0);
		break;

	case UM_PAGEUPPUSH:
		if (m_listBox->IsVertScrEnabled())
			m_listBox->SendMsg(NULL, UM_SCROLLVERTPAGEUP, 0, 0, 0, 0);
		break;

	case UM_COMMAND:
		switch (val1) {
		case ID_NAME:
			g_windowMgr.AddWhisperCharName2((const char*)val2);
			break;
		case ID_OPTION:
			if (!g_windowMgr.DeleteWindow(WID_BATTLEMSG_OPTIONWND))
				g_windowMgr.MakeWindow(WID_BATTLEMSG_OPTIONWND);
			break;
		case ID_STICK_ON: {
			UIWindow* linkedWnd = g_windowMgr.GetChatWnd();
			if (linkedWnd)
				linkedWnd->SendMsg(this, message, val1, val2, val3, 0);
			break;
		}
		case ID_MINI: {
			SetShow(false);
			UIWindow* linkedWnd = g_windowMgr.GetSubChatMiniWnd();
			if (linkedWnd)
				linkedWnd->SendMsg(this, UM_COMMAND, 0x88, m_id, 0, 0);
			break;
		}
		}
		break;

	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}

void UISubChatHisWnd::StoreInfo()
{
	if (m_info) {
		m_info->w = m_w;
		m_info->h = m_h;
		m_info->x = m_x;
		m_info->y = m_y;
	}
}
UISubChatMiniWnd::UISubChatMiniWnd()
{
	m_miniWidth = 32;
	m_miniHeight = 32;
	m_num = 2;
}

UISubChatMiniWnd::~UISubChatMiniWnd() {}

void UISubChatMiniWnd::OnCreate(int cx, int cy)
{
	Resize(m_miniWidth * m_num, m_miniHeight);
}

void UISubChatMiniWnd::OnDraw()
{
	CBitmapRes* bmp1;
	if (g_windowMgr.IsStSubChatOn())
		bmp1 = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\st_a.tga"), false);
	else
		bmp1 = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\st_b.tga"), false);

	CBitmapRes* bmp2;
	if (g_windowMgr.IsBtSubChatOn())
		bmp2 = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\bt_a.tga"), false);
	else
		bmp2 = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\bt_b.tga"), false);

	DrawBitmap(0, 0, bmp1, false);
	DrawBitmap(m_miniWidth, 0, bmp2, false);
}

void UISubChatMiniWnd::OnLBtnDown(int x, int y)
{
	if (x > 0 && x <= m_miniWidth) {
		g_windowMgr.StSubChatOnOff();
	} else {
		g_windowMgr.BtSubChatOnOff();
	}
	SendMsg(this, UM_COMMAND, 0xcd, 0, 0, 0);
	Invalidate();
}

int UISubChatMiniWnd::SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_COMMAND:
		switch (val1) {
		case 0x83:
			Move(val2, val3 - m_miniHeight);
			break;
		case 0x88:
			OnLBtnDown(m_miniWidth * val2 + 1, 0);
			break;
		case 0xcd:
			{
				UISubChatHisWnd* subChatWndST = g_windowMgr.GetSubChatWndST();
				UISubChatHisWnd* subChatWndBT = g_windowMgr.GetSubChatWndBT();
				if (subChatWndST)
					subChatWndST->SetShow(g_windowMgr.IsStSubChatOn() && m_show);
				if (subChatWndBT)
					subChatWndBT->SetShow(g_windowMgr.IsBtSubChatOn() && m_show);
			}
			break;
		}
		break;
	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}

UIBattleMsgOptionWnd::UIBattleMsgOptionWnd()
{
	m_curItem = -1;
	m_battleMsgOptionTable.push_back((char*)MsgStr(MSI_VIEW_CHAT_MSG));
	m_battleMsgOptionTable.push_back((char*)MsgStr(MSI_VIEW_WHISPER_MSG));
	m_battleMsgOptionTable.push_back((char*)MsgStr(MSI_VIEW_PARTY_MSG));
	m_battleMsgOptionTable.push_back((char*)MsgStr(MSI_VIEW_GUILD_MSG));
	m_battleMsgOptionTable.push_back((char*)MsgStr(MSI_VIEW_GET_ITEM_MSG));
	m_battleMsgOptionTable.push_back((char*)MsgStr(MSI_VIEW_EQUIP_MSG));
	m_battleMsgOptionTable.push_back((char*)MsgStr(MSI_VIEW_CHANGE_STATUS_MSG));
	m_battleMsgOptionTable.push_back((char*)MsgStr(MSI_VIEW_GET_ITEM_PARTY_MSG));
	m_battleMsgOptionTable.push_back((char*)MsgStr(MSI_VIEW_CHANGE_STATUS_PARTY_MSG));
	m_battleMsgOptionTable.push_back((char*)MsgStr(MSI_VIEW_FAIL_SKILL_MSG));
	m_battleMsgOptionTable.push_back((char*)MsgStr(MSI_VIEW_PARTY_SETUP_MSG));
	m_battleMsgOptionTable.push_back((char*)MsgStr(MSI_VIEW_DAMAGED_EQUIP_MSG));
	m_battleMsgOptionTable.push_back((char*)MsgStr(MSI_VIEW_SIEGE_INFO_MSG));
}

UIBattleMsgOptionWnd::~UIBattleMsgOptionWnd()
{
}

void UIBattleMsgOptionWnd::OnCreate(int cx, int cy)
{
	m_scrollBar = new UIScrollBar;
	m_scrollBar->Create(m_w - 15, 14, 1, m_h - 38);
	AddChild(m_scrollBar);

	int posX[2] = { 3, m_w - 14 };
	int ids[2] = { ID_BASE, ID_CLOSE };
	char* buttonName[2] = { "base", "close" };
	char* tooltip[2] = { "", "ESC" };
	std::string preBmpName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sys_");

	for (int i = 0; i < 2; ++i) {
		UIBitmapButton* button = new UIBitmapButton;
		button->SetBitmapName((preBmpName + buttonName[i] + "_off.bmp").c_str(), BS_NORMAL);
		button->SetBitmapName((preBmpName + buttonName[i] + "_on.bmp").c_str(), BS_MOUSEON);
		button->SetBitmapName((preBmpName + buttonName[i] + "_on.bmp").c_str(), BS_PRESSED);
		button->Create(button->GetBitmapWidth(), button->GetBitmapHeight());
		button->Move(posX[i], 3);
		button->SetId(ids[i]);
		button->SetToolTip(tooltip[i]);
		AddChild(button);
	}

	m_itemVertNum = (cy - 38) / 20;
	RecalcScrbarPos();
}

void UIBattleMsgOptionWnd::OnDraw()
{
	CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_left.bmp"), false);
	DrawBitmap(0, 0, bmp, false);

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_right.bmp"), false);
	DrawBitmap(m_w - bmp->m_width, 0, bmp, false);

	int loopCnt = (m_w - 24) / 12 + (((m_w - 24) % 12) ? 1 : 0);
	for (int i = 0; i < loopCnt; ++i) {
		bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_mid.bmp"), false);
		DrawBitmap((i + 1) * 12, 0, bmp, false);
	}

	unsigned long rr;
	unsigned long gg;
	unsigned long bb;
	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(0, 17, m_w, m_h - 17, RGB(rr, gg, bb));

	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	DrawBox(1, 17, m_w - 2, m_h - 17, RGB(rr, gg, bb));

	loopCnt = (m_h - 42) / 8 + (((m_h - 42) % 8) ? 1 : 0);
	for (i = 0; i < loopCnt; ++i) {
		bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\itemwin_left.bmp"), false);
		DrawBitmap(0, i * 8 + 19, bmp, false);
	}

	loopCnt = (m_h - 38) / 8 + (((m_h - 38) % 8) ? 1 : 0);
	for (i = 0; i < loopCnt; ++i) {
		bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\itemwin_right.bmp"), false);
		DrawBitmap(m_w - bmp->m_width, i * 8 + 17, bmp, false);
	}

	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(18, 17, 1, m_h - 21, RGB(rr, gg, bb));

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_left.bmp"), false);
	DrawBitmap(0, m_h - bmp->m_height, bmp, false);

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_right.bmp"), false);
	DrawBitmap(m_w - bmp->m_width, m_h - 21, bmp, false);

	loopCnt = (m_w - 42) / 21 + (((m_w - 42) % 21) ? 1 : 0);
	for (i = 0; i < loopCnt; ++i) {
		bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_mid.bmp"), false);
		DrawBitmap((i + 1) * 21, m_h - bmp->m_height, bmp, false);
	}

	TextOutWithShadow(17, 2, MsgStr(MSI_BATTLE_CHAT_WND_OPTION), 0, 0, 0xffffff, 0, 12);

	std::vector<char*>::iterator it = m_battleMsgOptionTable.begin();
	int drawCount = 0;
	int itemCount = 0;
	for (; it != m_battleMsgOptionTable.end(); ) {
		if (itemCount < m_viewOffset) {
			++it;
			++itemCount;
			continue;
		}
		int viewCount = drawCount + 1;
		if (m_curItem == itemCount) {
			unsigned long rr;
			unsigned long gg;
			unsigned long bb;
			g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
			DrawBox(25, viewCount * 20 + 2 - 3, m_w - 40, 20, RGB(rr, gg, bb));
			DrawBox(26, viewCount * 20 + 2 - 2, m_w - 42, 18, 0xffffff);
		}

		TextOutA(50, viewCount * 20 + 2, *it, 0, 0, 12, 0);

		if (g_session.m_battleMsgOnOffSet[itemCount])
			bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\grp_online.bmp"), false);
		else
			bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\grp_offline.bmp"), false);
		DrawBitmap(27, viewCount * 20 + 2, bmp, true);

		if (drawCount + 1 == m_itemVertNum)
			break;

		++it;
		++drawCount;
		++itemCount;
	}
}

void UIBattleMsgOptionWnd::OnLBtnDown(int x, int y)
{
	if (y < 17) {
		UIFrameWnd::OnLBtnDown(x, y);
		return;
	}

	int oldCurItem = m_curItem;
	m_curItem = GetCurItem(x, y);
	if (oldCurItem != m_curItem)
		Invalidate();

	if (m_curItem != -1) {
		g_session.m_battleMsgOnOffSet[m_curItem] = !g_session.m_battleMsgOnOffSet[m_curItem];
		Invalidate();
	}
}

void UIBattleMsgOptionWnd::OnLBtnDblClk(int x, int y)
{
	if (y < 17) {
		UIFrameWnd::OnLBtnDown(x, y);
	}
}

void UIBattleMsgOptionWnd::OnMouseHover(int x, int y)
{
	int oldCurItem = m_curItem;
	m_curItem = GetCurItem(x, y);
	if (oldCurItem != m_curItem)
		Invalidate();
}

void UIBattleMsgOptionWnd::OnWheel(int wheel)
{
	if (m_vertScrEnabled) {
		int n = m_itemVertNum - 1;
		if (n >= 3) n = 3;
		if (wheel < 0)
			SendMsg(m_scrollBar, UM_SCROLLVERT, n, 0, 0, 0);
		else
			SendMsg(m_scrollBar, UM_SCROLLVERT, -n, 0, 0, 0);
	}
}

int UIBattleMsgOptionWnd::SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_COMMAND:
		if (val1 == 0x87) {
			WriteToReg();
			g_windowMgr.DeleteWindow((WINDOWID)0x8b);
		}
		break;

	case UM_SCROLLVERT:
	case UM_SCROLLVERTPAGEUP:
	case UM_SCROLLVERTPAGEDOWN: {
		int oldViewOffset = m_viewOffset;
		if (message == UM_SCROLLVERTPAGEUP)
			m_viewOffset = oldViewOffset - m_itemVertNum + 1;
		else if (message == UM_SCROLLVERTPAGEDOWN)
			m_viewOffset = m_itemVertNum + oldViewOffset - 1;
		else
			m_viewOffset = oldViewOffset + val1;

		if (oldViewOffset != m_viewOffset) {
			RecalcScrbarPos();
			Invalidate();
		}
		return m_viewOffset - oldViewOffset;
	}

	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

void UIBattleMsgOptionWnd::RecalcScrbarPos()
{
	if (m_scrollBar) {
		int numCanDisplayItem = m_itemVertNum;
		int numLine = m_battleMsgOptionTable.size() + (m_battleMsgOptionTable.size() != 0);
		int range = numLine - numCanDisplayItem;

		if (range < 0)
			range = 0;
		if (range != 0)
			m_vertScrEnabled = 1;
		else
			m_vertScrEnabled = 0;

		if (m_vertScrEnabled) {
			m_scrollBar->SetRange(range, numCanDisplayItem, numLine);
			if (m_viewOffset < 0)
				m_viewOffset = 0;
			if (m_viewOffset > numLine - m_itemVertNum)
				m_viewOffset = numLine - m_itemVertNum;
			m_scrollBar->SetPos(m_viewOffset);
		} else {
			m_viewOffset = 0;
			m_scrollBar->SetRange(0, numCanDisplayItem, numLine);
		}
		m_scrollBar->Invalidate();
	}
}
int UIBattleMsgOptionWnd::GetCurItem(int x, int y)
{
	if (x < 0 || y < 17 || y >= m_itemVertNum * 20 + 17)
		return -1;

	int xCell = x / m_w;
	int yCell = (y - 17) / 20;
	std::vector<char*>::iterator it = m_battleMsgOptionTable.begin();
	int k = 0;
	int idx = 0;
	while (it != m_battleMsgOptionTable.end()) {
		if (idx < m_viewOffset) {
			++it;
			++idx;
		} else {
			if (k == yCell + xCell)
				return idx;
			++it;
			++k;
			++idx;
		}
	}

	return -1;
}

void UIBattleMsgOptionWnd::WriteToReg()
{
	HKEY hKey;
	char OptionPath[512];
	DWORD dwDisposition = 0;
	char buf[10];

	strcpy(OptionPath, g_regPath);
	strcat(OptionPath, "\\ChatOption");

	RegCreateKeyExA(HKEY_LOCAL_MACHINE, OptionPath, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	for (int i = 0; i < 13; ++i) {
		itoa(i, buf, 10);
		RegSetValueExA(hKey, buf, 0, REG_DWORD, (LPBYTE)&g_session.m_battleMsgOnOffSet[i], 4);
	}

	RegCloseKey(hKey);
}

UIShortCutWnd::UIShortCutWnd()
{
	m_slot = -1;
	m_maxSlot = 27;
}


UIShortCutWnd::~UIShortCutWnd() {}


void UIShortCutWnd::OnCreate(int cx, int cy)
{
	int posX[3] = { m_w - 14 };
	int ids2[3] = { ID_CLOSE };
	char* buttonName2[8] = { "close" };
	char* toolTip[8] = { "F12" };
	std::string preBmpName2("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sys_");

	for (int i = 0; i < 1; ++i) {
		UIBitmapButton* button = new UIBitmapButton;
		button->SetBitmapName((preBmpName2 + buttonName2[i] + "_off.bmp").c_str(), BS_NORMAL);
		button->SetBitmapName((preBmpName2 + buttonName2[i] + "_on.bmp").c_str(), BS_MOUSEON);
		button->SetBitmapName((preBmpName2 + buttonName2[i] + "_on.bmp").c_str(), BS_PRESSED);
		button->Create(button->GetBitmapWidth(), button->GetBitmapHeight());
		button->Move(posX[i], 3);
		button->SetId(ids2[i]);
		button->SetToolTip(toolTip[i]);
		AddChild(button);
	}

	for (i = 0; i < m_maxSlot; ++i) {
		m_shortCutList[i].isSkill = 0;
		m_shortCutList[i].ID = 0;
		m_shortCutList[i].count = 0;
	}

	SendMsg(NULL, UM_REFRESHITEM, 0, 0, 0, 0);
}

void UIShortCutWnd::OnDraw()
{
	char text[1024];
	char msg[20];

	CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\shortitem_bg.bmp"), false);
	DrawBitmap(0, 0, bmp, false);

	if (m_slot != -1)
		DrawBox(m_slot * 29 + 5, 5, 24, 24, 0xb4ffb4);

	for (int i = 0; i < 9; ++i) {
		int x = i * 29 + 5;
		int slot = i + (g_session.m_shortcutSlotCnt & 0xff) * 9;
		if (m_shortCutList[slot].ID) {
			if (m_shortCutList[slot].isSkill) {
				SKILL_INFO skillInfo;
				if (m_shortCutList[slot].ID >= 0x1f40 && m_shortCutList[slot].ID < 0x1f51)
					skillInfo = g_session.GetHomunSkillItemInfoBySkillId(m_shortCutList[slot].ID);
				else if (m_shortCutList[slot].ID >= 0x2008 && m_shortCutList[slot].ID < 0x202e)
					skillInfo = g_session.GetMerSkillItemInfoBySkillId(m_shortCutList[slot].ID);
				else
					skillInfo = g_session.GetSkillItemInfoBySkillId(m_shortCutList[slot].ID);

				if (skillInfo.m_isValid) {
					g_session.GetSkillBitmapFileName(skillInfo.skillName, text);
					CBitmapRes* skillBmp = (CBitmapRes*)g_resMgr().Get(text, true);
					DrawBitmap(x, 4, skillBmp, true);

					if (m_shortCutList[slot].count) {
						sprintf(text, "%d", m_shortCutList[slot].count);
						TextOutA(x + 15, 19, text, 0, 0, 11, 0xffffff);
						TextOutA(x + 13, 19, text, 0, 0, 11, 0xffffff);
						TextOutA(x + 14, 20, text, 0, 0, 11, 0xffffff);
						TextOutA(x + 14, 18, text, 0, 0, 11, 0xffffff);
						TextOutA(x + 14, 19, text, 0, 0, 11, 0);
					}
				}
			}
			else {
				ITEM_INFO itemInfo = g_session.GetItemInfoById(m_shortCutList[slot].ID);
				if (itemInfo.IsValid())
					DrawItemWithCount(x - 1, 3, itemInfo, (bool&)itemInfo.m_isIdentified, true);
			}
		}

		sprintf(msg, "%d", (g_session.m_shortcutSlotCnt & 0xff) + 1);
		TextOutA(m_w - 13, m_h - 16, msg, 0, 0, 12, 0);
	}
}

void UIShortCutWnd::OnMouseHover(int x, int y)
{
	static const char* shortcutName[] = {
		"F1",
		"F2",
		"F3",
		"F4",
		"F5",
		"F6",
		"F7",
		"F8",
		"F9",
	};

	int slot = GetSlot(x, y);
	if (slot != m_slot) {
		m_slot = slot;
		Invalidate();
	}

	if (m_slot == -1)
		return;

	int gx = ((x - 4) / 29) * 29 + 4;
	int gy = -17;
	GetGlobalCoor(gx, gy);

	int index = m_slot + g_session.m_shortcutSlotCnt * 9;

	char itemName[128];
	char itemHelp[128];

	if (m_shortCutList[index].ID) {
		if (m_shortCutList[index].isSkill) {
			SKILL_INFO skillInfo;
			int skillId = m_shortCutList[index].ID;
			if (skillId >= 0x1f40 && skillId < 0x1f51)
				skillInfo = g_session.GetHomunSkillItemInfoBySkillId(skillId);
			else if (skillId >= 0x2008 && skillId < 0x202e)
				skillInfo = g_session.GetMerSkillItemInfoBySkillId(skillId);
			else
				skillInfo = g_session.GetSkillItemInfoBySkillId(skillId);

			int skillUseLevel = m_shortCutList[index].count;
			if (!skillUseLevel) {
				sprintf(itemHelp, "%s %s (Sp : %d)", shortcutName[m_slot], skillInfo.GetName(), skillInfo.spcost);
			} else {
				const std::vector<int>& spList = skillInfo.LevelUseSpAmountList();
				int sp = 0;

				if (spList.size() >= (unsigned)skillUseLevel)
					sp = spList[skillUseLevel - 1];

				sprintf(itemHelp, "%s %s Use Lv %d (Sp : %d)", shortcutName[m_slot], skillInfo.GetName(), skillUseLevel, sp);
			}
		}
		else {
			ITEM_INFO itemInfo = g_session.GetItemInfoById(m_shortCutList[index].ID);
			itemInfo.GetItemName(itemName);
			sprintf(itemHelp, MsgStr(MSI_EA3), shortcutName[m_slot], itemName, itemInfo.m_num);
		}
	} else {
		strcpy(itemHelp, shortcutName[m_slot]);
	}

	g_modeMgr.GetCurMode()->MakeHelpBalloon(itemHelp, gx, gy, -1);
}

void UIShortCutWnd::OnProcess()
{
	int x = g_mouse.GetXPos() - m_x;
	int y = g_mouse.GetYPos() - m_y;
	int slot = GetSlot(x, y);
	if (slot != m_slot) {
		m_slot = slot;
		Invalidate();
	}
}

void UIShortCutWnd::DragAndDrop(int x, int y, DRAG_INFO const* const dragInfo)
{
	int page = g_session.m_shortcutSlotCnt & 0xff;

	if (dragInfo->m_dragType == DT_FROM_SKILLLISTWND || dragInfo->m_dragType == DT_FROM_HOSKILLLISTWND ||
		dragInfo->m_dragType == DT_FROM_MERSKILLLISTWND) {
		std::string skillName = dragInfo->m_skillName;
		int skillIndex = dragInfo->m_dragItemIndex;
		int slot = GetSlot(x, y);
		if (slot != -1) {
			SKILL_INFO skillInfo(0);
			switch (dragInfo->m_dragType) {
			case DT_FROM_SKILLLISTWND:
				skillInfo = g_session.GetSkillItemInfoBySkillId(skillIndex);
				break;
			case DT_FROM_HOSKILLLISTWND:
				skillInfo = g_session.GetHomunSkillItemInfoBySkillId(skillIndex);
				break;
			case DT_FROM_MERSKILLLISTWND:
				skillInfo = g_session.GetMerSkillItemInfoBySkillId(skillIndex);
				break;
			}

			if (skillInfo.level && skillInfo.m_isValid && skillInfo.spcost) {
				g_session.SetShortCutItemInfo(1, skillInfo.SKID, dragInfo->m_skillUseLevel, slot + page * 9);
				SendMsg(NULL, UM_REFRESHITEM, 0, 0, 0, 0);
				Invalidate();
			}
		}
	} else if (dragInfo->m_dragType == DT_FROM_ITEMWND || dragInfo->m_dragType == DT_FROM_SHORTCUTWND) {
		ITEM_INFO itemInfo = g_session.GetItemInfoByIndex(dragInfo->m_dragItemIndex);
		if (itemInfo.m_itemIndex) {
			switch (itemInfo.m_itemType) {
			case 4:
			case 5:
			case 8:
			case 9:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
				if (!itemInfo.m_location)
					break;
			case 0:
			case 1:
			case 2:
			case 10:
			case 16:
			case 17: {
				int slot = GetSlot(x, y);
				if (slot != -1) {
					g_session.SetShortCutItemInfo(0, itemInfo.GetItemId(), 0, slot + page * 9);
					SendMsg(NULL, UM_REFRESHITEM, 0, 0, 0, 0);
					Invalidate();
				}
				break;
			}
			}
		}
	}

	g_modeMgr.GetCurMode()->SendMsg(MM_CANCEL_DRAG, 0, 0, 0);
}

void UIShortCutWnd::OnLBtnDown(int x, int y)
{
	bool outside = x - 4 >= 261 || x < 4 || y < 4;
	if (outside) {
		UIFrameWnd::OnLBtnDown(x, y);
	} else {
		int slot = GetSlot(x, y), page = g_session.m_shortcutSlotCnt & 0xff;
		if (slot != -1) {
			if (!m_shortCutList[slot + page * 9].isSkill) {
				if (m_shortCutList[slot + page * 9].ID) {
					ITEM_INFO itemInfo = g_session.GetItemInfoById(m_shortCutList[slot + page * 9].ID);
					if (itemInfo.m_itemIndex) {
						CMode* mode = g_modeMgr.GetCurMode();
						mode->SendMsg(MM_BEGIN_DRAG_FROM_SHORTCUTWND, itemInfo.m_itemIndex, 0, 0);
					}
				}
			} else {
				SKILL_INFO skillInfo;
				unsigned long skillId;
				if ((skillId = m_shortCutList[slot + page * 9].ID) >= 0x1f40 && skillId < 0x1f51)
					skillInfo = g_session.GetHomunSkillItemInfoBySkillId(skillId);
				else {
					if (skillId >= 0x2008 && skillId < 0x202e)
						skillInfo = g_session.GetMerSkillItemInfoBySkillId(skillId);
					else
						skillInfo = g_session.GetSkillItemInfoBySkillId(skillId);
				}

				if (skillInfo.m_isValid && skillInfo.level && skillInfo.spcost) {
					CMode* mode = g_modeMgr.GetCurMode();
					if (skillInfo.SKID >= 0x1f40 && skillInfo.SKID < 0x1f51) {
						mode->SendMsg(MM_BEGIN_DRAG_FROM_HOSKILLLISTWND, skillInfo.SKID, (int)skillInfo.skillName, m_shortCutList[slot + page * 9].count);
					} else if (skillInfo.SKID >= 0x2008 && skillInfo.SKID < 0x202e) {
						mode->SendMsg(MM_BEGIN_DRAG_FROM_MERSKILLLISTWND, skillInfo.SKID, (int)skillInfo.skillName, m_shortCutList[slot + page * 9].count);
					} else {
						mode->SendMsg(MM_BEGIN_DRAG_FROM_SKILLLISTWND, skillInfo.SKID, (int)skillInfo.skillName, m_shortCutList[slot + page * 9].count);
					}
				}
			}
		}
	}
}

void UIShortCutWnd::OnRBtnDown(int x, int y)
{
	if (x - 4 >= 261)
		return;

	int slot = GetSlot(x, y);
	if (slot == -1)
		return;

	int index = slot + (g_session.m_shortcutSlotCnt & 0xff) * 9;
	char isSkill = m_shortCutList[index].isSkill;
	unsigned long id = m_shortCutList[index].ID;
	if (isSkill) {
		UIWindow* wnd = g_windowMgr.MakeWindow(WID_SKILLDESCRIBEWND);
		wnd->SendMsg(NULL, UM_SKILLDESCRIBE, id, 0, 0, 0);
		return;
	}

	if (id) {
		ITEM_INFO itemInfo = g_session.GetItemInfoById(id);
		if (itemInfo.m_itemIndex) {
			UIWindow* wnd = g_windowMgr.MakeWindow(WID_ITEMCOLLECTIONWND);
			wnd->SendMsg(NULL, UM_ITEMEXPLANATION, (int)&itemInfo, 0, 0, 0);
		}
	}
}

void UIShortCutWnd::OnLBtnDblClk(int x, int y)
{
	int slot = GetSlot(x, y);
	if (slot != -1) {
		SendMsg(NULL, UM_USE_SHORTEN_ITEM, slot, 0, 0, 0);
	}
}

void UIShortCutWnd::StoreInfo()
{
	if (m_info) {
		m_info->x = m_x;
		m_info->y = m_y;
		m_info->w = m_w;
		m_info->h = m_h;
	}
}

int UIShortCutWnd::SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_USE_SHORTEN_ITEM: {
		int page = g_session.m_shortcutSlotCnt & 0xff;
		if (g_session.m_isBattle)
			page = val2;

		int slot = val1 + page * 9;
		if (!m_shortCutList[slot].isSkill) {
			if (!m_shortCutList[slot].ID)
				break;

			ITEM_INFO itemInfo = g_session.GetItemInfoById(m_shortCutList[slot].ID);
			if (itemInfo.m_itemIndex) {
				CGameMode* mode = g_modeMgr.GetGameMode();
				switch (itemInfo.m_itemType) {
				case 0:
				case 1:
				case 2:
					mode->SendMsg(MM_USEITEM, itemInfo.m_itemIndex, 0, 0);
					break;
				case 18:
					if (g_session.GetHonor() >= 0) {
						char msg[500];
						char itemName[128];
						itemInfo.GetItemName(itemName);
						sprintf(msg, MsgStr(MSI_CONFIRM_TO_OPEN_CASH_ITEM), itemName);
						if (g_windowMgr.ErrorMsg(msg, ME_YESNO, 0, 0, 0) == ID_YES)
							mode->SendMsg(MM_USEITEM, itemInfo.m_itemIndex, 0, 0);
					}
					break;
				case 10:
				case 16:
				case 17:
					if (itemInfo.m_wearLocation == LOCATION_ARROW) {
						mode->SendMsg(MM_TAKE_OFF_EQUIP, 0xf, 0, 0);
						mode->SendMsg(MM_WEAR_EQUIP_ARROW, itemInfo.m_itemIndex, itemInfo.m_location, 0);
					}
					break;
				default:
					if (itemInfo.m_location && !itemInfo.m_wearLocation && itemInfo.m_isIdentified)
						mode->SendMsg(MM_WEAR_EQUIP, itemInfo.m_itemIndex, itemInfo.m_location, 0);
					break;
				}
			}
		} else {
			SKILL_INFO skillInfo;
			if (m_shortCutList[slot].ID >= 0x1f40 && m_shortCutList[slot].ID < 0x1f51)
				skillInfo = g_session.GetHomunSkillItemInfoBySkillId(m_shortCutList[slot].ID);
			else if (m_shortCutList[slot].ID >= 0x2008 && m_shortCutList[slot].ID < 0x202e)
				skillInfo = g_session.GetMerSkillItemInfoBySkillId(m_shortCutList[slot].ID);
			else
				skillInfo = g_session.GetSkillItemInfoBySkillId(m_shortCutList[slot].ID);

			if (!skillInfo.level)
				break;
			if (!skillInfo.m_isValid)
				break;
			g_modeMgr.GetCurMode()->SendMsg(MM_USING_SKILL, (int)&skillInfo, m_shortCutList[slot].count, 0);
		}
		break;
	}

	case UM_REFRESHITEM: {
		for (int slot = 0; slot < m_maxSlot; ++slot) {
			unsigned long id = g_session.m_shortCutList[slot].ID;
			int isSkill = g_session.m_shortCutList[slot].isSkill;
			int level = g_session.m_shortCutList[slot].count;

			m_shortCutList[slot].ID = 0;
			m_shortCutList[slot].isSkill = 0;
			m_shortCutList[slot].count = 0;

			if (!id)
				continue;

			if (isSkill) {
				SKILL_INFO skillInfo = g_session.GetSkillItemInfoBySkillId(id);
				if (!skillInfo.m_isValid) {
					skillInfo = g_session.GetHomunSkillItemInfoBySkillId(id);
					if (!skillInfo.m_isValid)
						skillInfo = g_session.GetMerSkillItemInfoBySkillId(id);
				}

				if (skillInfo.m_isValid && skillInfo.level && skillInfo.spcost) {
					m_shortCutList[slot].ID = skillInfo.SKID;
					m_shortCutList[slot].isSkill = 1;
					m_shortCutList[slot].count = level;
				}
			}
			else {
				ITEM_INFO itemInfo = g_session.GetItemInfoById(id);
				if (itemInfo.m_itemIndex && itemInfo.m_isIdentified)
					m_shortCutList[slot].ID = id;
			}
		}
		Invalidate();
		break;
	}

	case UM_SETINFO:
		m_info = (ShortCutWndInfo*)val1;
		if (!m_info->IsValidInfo())
			m_info->MakeDefault();
		SendMsg(NULL, UM_REFRESHITEM, 0, 0, 0, 0);
		Move(m_info->x, m_info->y);
		break;

	case UM_COMMAND:
		if (val1 == 0x87)
			g_windowMgr.ShowWindow(WID_SHORTCUTWND, 0);
		break;

	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}

int UIShortCutWnd::GetSlot(int x, int y)
{
	if (y < 0 || y >= 24 || x <= 3)
		return -1;

	if (x - 4 < 29)
		return 0;
	if (x - 4 < 58)
		return 1;
	if (x - 4 < 87)
		return 2;
	if (x - 4 < 116)
		return 3;
	if (x - 4 < 145)
		return 4;
	if (x - 4 < 174)
		return 5;
	if (x - 4 < 203)
		return 6;
	if (x - 4 < 232)
		return 7;
	if (x - 4 < 261)
		return 8;

	return -1;
}
void UIShortCutWnd::SetShow(int visible)
{
	m_show = visible;
	if (visible) {
		SendMsg(0, UM_REFRESHITEM, 0, 0, 0, 0);
		Invalidate();
		g_windowMgr.UpdateSnapInfo(this);
	} else {
		g_windowMgr.DeleteSnapInfo(this);
	}
}

UIMemorialDunWnd::UIMemorialDunWnd() {}

UIMemorialDunWnd::~UIMemorialDunWnd() {}

void UIMemorialDunWnd::OnCreate(int cx, int cy)
{
	{
		int posX[2] = { 3, m_w - 15 };
		int ids[2] = { ID_BASE, ID_CLOSE };
		char* buttonName[2] = { "base", "close" };
		char* tooltip[8] = { "", "Alt+B" };
		std::string preBmpName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sys_");

		for (int i = 0; i < 2; ++i) {
			UIBitmapButton* button = new UIBitmapButton;
			button->SetBitmapName((preBmpName + buttonName[i] + "_off.bmp").c_str(), BS_NORMAL);
			button->SetBitmapName((preBmpName + buttonName[i] + "_on.bmp").c_str(), BS_MOUSEON);
			button->SetBitmapName((preBmpName + buttonName[i] + "_on.bmp").c_str(), BS_PRESSED);
			button->Create(button->GetBitmapWidth(), button->GetBitmapHeight());
			button->Move(posX[i], 3);
			button->SetId(ids[i]);
			button->SetToolTip(tooltip[i]);
			AddChild(button);
		}
	}

	{
		std::string pathName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\");
		char* buttonName[2] = { "btn_ok", "btn_cancel" };
		int ids[2] = { ID_OK, ID_CANCEL };

		for (int j = 0; j < 2; ++j) {
			m_buttons[j] = new UIBitmapButton;
			m_buttons[j]->SetBitmapName((pathName + buttonName[j] + ".bmp").c_str(), BS_NORMAL);
			m_buttons[j]->SetBitmapName((pathName + buttonName[j] + "_a.bmp").c_str(), BS_MOUSEON);
			m_buttons[j]->SetBitmapName((pathName + buttonName[j] + "_b.bmp").c_str(), BS_PRESSED);
			m_buttons[j]->Create(m_buttons[j]->GetBitmapWidth(), m_buttons[j]->GetBitmapHeight());
			m_buttons[j]->Move(-100, -100);
			m_buttons[j]->SetId(ids[j]);
			AddChild(m_buttons[j]);
		}
	}

	RefreshInfo();
}

void UIMemorialDunWnd::Move(int x, int y)
{
	UIFrameWnd::Move(x, y);

	if (g_session.dunInfoWndX != x)
		g_session.dunInfoWndX = x;
	if (g_session.dunInfoWndY != y)
		g_session.dunInfoWndY = y;
}

void UIMemorialDunWnd::OnDraw() {}

int UIMemorialDunWnd::SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_DEFPUSH:
		SendMsg(NULL, UM_COMMAND, m_defPushId, 0, 0, 0);
		break;
	case UM_COMMAND:
		switch (val1) {
		case ID_OK:
			g_windowMgr.DeleteWindow(WID_MEMORIAL_DUN_WND);
			break;
		case ID_CLOSE:
			SetShow(FALSE);
			if (g_session.m_dunState == 2)
				g_session.m_onDunInfoWnd = FALSE;
			break;
		case ID_CANCEL: {
			PACKET_CZ_MEMORIALDUNGEON_COMMAND p;
			p.PacketType = 0x2cf;
			if (g_session.m_dunState == 0)
				p.Command = PACKET_CZ_MEMORIALDUNGEON_COMMAND::COMMAND_SUBSCRIPTION_CANCEL;
			else if (g_session.m_dunState == 1)
				p.Command = PACKET_CZ_MEMORIALDUNGEON_COMMAND::COMMAND_MEMORIALDUNGEON_DESTROY;

			CRagConnection::instanceR().SendPacket(CRagConnection::instanceR().GetPacketSize(p.PacketType), (const char *)&p);
			break;
		}
		}
		break;
	case UM_REFRESH:
		RefreshInfo();
		break;
	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}

void UIMemorialDunWnd::InitInfo()
{
	g_session.m_dunName = "";
	g_session.m_priority = 0;
	g_session.m_destroyTime = 0;
	g_session.m_enterTimeOut = 0;
	g_session.m_dunState = 0;
}

void UIMemorialDunWnd::RefreshInfo()
{
	if (g_session.m_dunState == 0 || g_session.m_dunState == 1) {
		m_buttons[0]->Move(-100, -100);
		if (g_session.m_amIPartyMaster) {
			m_buttons[1]->Move((m_w - m_buttons[1]->GetWidth()) / 2, m_h - 30);
			m_defPushId = 0x77;
		}
	} else if (g_session.m_dunState == 3) {
		m_buttons[0]->Move((m_w - m_buttons[0]->GetWidth()) / 2, m_h - 30);
		m_buttons[1]->Move(-100, -100);
		m_defPushId = 0x76;
	} else if (g_session.m_dunState == 2) {
		m_buttons[0]->Move(-100, -100);
		m_buttons[1]->Move(-100, -100);
		m_defPushId = 0x75;
	}

	Invalidate();
}
UIRoMapWnd::UIRoMapWnd()
{
	InitInfo();
	m_titleBarTexture = 0;
	m_roMapTexture = 0;
}

UIRoMapWnd::~UIRoMapWnd()
{
	m_titleBarTexture = 0;
	m_roMapTexture = 0;
	m_miniMapTexture = 0;
}


void UIRoMapWnd::OnCreate(int cx, int cy)
{
	m_titleBarTexture = g_texMgr.GetTexture("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\titlebarmap.bmp", false);
	m_roMapTexture = g_texMgr.GetTexture("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\worldmap.bmp", false);

	{
		int posX[2] = { m_w - 30, m_w - 15 };
		int ids[2] = { ID_MAP, ID_CLOSE };
		char* buttonName[2] = { "view", "close" };
		char* tooltip[8] = { "", "ctrl+`" };

		std::string preBmpName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\world_");

		for (int i = 0; i < 2; ++i) {
			UIBitmapButton* button = new UIBitmapButton;
			button->SetBitmapName((preBmpName + buttonName[i] + "_off.bmp").c_str(), BS_NORMAL);
			button->SetBitmapName((preBmpName + buttonName[i] + "_on.bmp").c_str(), BS_MOUSEON);
			button->SetBitmapName((preBmpName + buttonName[i] + "_on.bmp").c_str(), BS_PRESSED);
			button->Create(button->GetBitmapWidth(), button->GetBitmapHeight());
			button->Move(posX[i], 0);
			button->SetId(ids[i]);
			button->SetToolTip(tooltip[i]);
			AddChild(button);
		}
	}

	GetPartyFriendMapInfo();
}

void UIRoMapWnd::OnMouseMove(int x, int y)
{
	if (g_windowMgr.GetCapture() != this) {
		ShowThisMapPartyInfo(x, y);
		ShowThisMapName(x, y);
	}
}

int UIRoMapWnd::SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_COMMAND:
		switch (val1) {
		case ID_MAP:
			m_showAllMapName = !m_showAllMapName;
			Invalidate();
			break;
		case ID_CLOSE:
			g_windowMgr.DeleteWindow(WID_ROMAPWND);
			break;
		}
		break;
	case UM_REFRESH:
		InitInfo();
		GetPartyFriendMapInfo();
		Invalidate();
		break;
	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}

void UIRoMapWnd::OnDraw()
{
	ClearDC(0xffff00ff);
	if (m_showAllMapName) {
		ShowAllMapName();
	} else {
		DrawPartyInfos();
		DrawThisMapInfo();
	}
	if (m_showMiniMap)
		DrawMiniMap();
}

void UIRoMapWnd::DrawRoMap()
{
	static unsigned long m_boxColor[12] = { 0x66dc143c, 0x66ffa500, 0x66ffff00, 0x66adff2f, 0x6600bfff, 0x66483d8b, 0x669400d3, 0x66ff1493, 0x66fff5ee, 0x66f5fffa, 0x66f0f8ff, 0x66778899 };
	if (m_titleBarTexture)
		m_titleBarTexture->DrawSurfaceStretch(0, 0, m_w, 17);

	if (m_roMapTexture)
		m_roMapTexture->DrawSurfaceStretch(0, 17, m_w, m_h - 17);

	if (!m_showAllMapName) {
		int colorIndex = 0;
		std::list<PartyMapInfo>::iterator it = m_partyPosList.begin();
		while (it != m_partyPosList.end()) {
			if (colorIndex == 12)
				colorIndex = 0;
			DrawBoxScreen((*it).mapPosInfo.x_LT,
						  (*it).mapPosInfo.y_LT,
						  (*it).mapPosInfo.x_RB - (*it).mapPosInfo.x_LT,
						  (*it).mapPosInfo.y_RB - (*it).mapPosInfo.y_LT,
						  m_boxColor[colorIndex]);
			++it;
			++colorIndex;
		}
	}

	if (m_showMiniMap && m_miniMapInfo) {
		MapPosInfo tmpMapInfo = m_showMiniMap->mapPosInfo;
		DrawBoxScreen(m_miniMapInfo->x_LT, m_miniMapInfo->y_LT, 130, 130, 0x88000000);
		if (m_miniMapTexture)
			m_miniMapTexture->DrawSurfaceStretch(m_miniMapInfo->x_LT, m_miniMapInfo->y_LT, 130, 130);
		DrawBoxScreen(tmpMapInfo.x_LT,
					  tmpMapInfo.y_LT,
					  tmpMapInfo.x_RB - tmpMapInfo.x_LT,
					  tmpMapInfo.y_RB - tmpMapInfo.y_LT,
					  0x66000000);
	}

	if (m_thisPosInfo)
		DrawBoxScreen(m_thisPosInfo->x_LT,
					  m_thisPosInfo->y_LT,
					  m_thisPosInfo->x_RB,
					  m_thisPosInfo->y_RB,
					  0x66000000);
}


void UIRoMapWnd::DrawMiniMap()
{
	int w = m_showMiniMap->mapPosInfo.x_RB - m_showMiniMap->mapPosInfo.x_LT;
	int h = m_showMiniMap->mapPosInfo.y_RB - m_showMiniMap->mapPosInfo.y_LT;
	DrawBorderLine(m_showMiniMap->mapPosInfo.x_LT, m_showMiniMap->mapPosInfo.y_LT, w, h, 0xffffff);

	int x = m_showMiniMap->mapPosInfo.x_RB + 10;
	int y = m_showMiniMap->mapPosInfo.y_LT + 10;
	if (x + 130 > g_renderer->m_width)
		x = m_showMiniMap->mapPosInfo.x_LT - 140;
	if (y + 130 > g_renderer->m_height)
		y = g_renderer->m_height - 130;

	m_miniMapInfo = new MapPosInfo(x, y, x + 130, y + 130);

	tagRECT rect = { x, y, x + 130, y + 130 };
	ClearDCRect(0xffff00ff, rect);

	std::string mapName = m_showMiniMap->mapName;
	if (mapName.size() > 4)
		mapName.resize(mapName.size() - 4);

	TextOutWithOutline(x + 3, y + 115, mapName.c_str(), 0, 0xffffff, 0x5c5c5c, 0, 12, true);

	{
		std::string minimapBmpName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\map\\");
		minimapBmpName += mapName;
		minimapBmpName += ".bmp";
		m_miniMapTexture = g_texMgr.GetTexture(minimapBmpName.c_str(), false);
	}

	mapName = g_session.GetMapName(m_showMiniMap->mapName.c_str());
	TextOutWithOutline(x + 3, y + 3, mapName.c_str(), 0, 0xd2ff, 0, 0, 12, true);
}


void UIRoMapWnd::DrawPartyInfos()
{
	std::list<PartyMapInfo>::iterator it = m_partyPosList.begin();

	while (it != m_partyPosList.end()) {
		{
			int w = it->mapPosInfo.x_RB - it->mapPosInfo.x_LT;
			int h = it->mapPosInfo.y_RB - it->mapPosInfo.y_LT;
			DrawBorderLine(it->mapPosInfo.x_LT, it->mapPosInfo.y_LT, w, h, 0xffffff);

			std::string mapName = it->mapName;
			if (mapName.size() > 4)
				mapName.resize(mapName.size() - 4);

			int x = GetTextWidth(mapName.c_str(), 0, 0, 12, true);
			x = (x - it->mapPosInfo.x_LT - it->mapPosInfo.x_RB) / 2;
			if (x < 0)
				x = -x;
			TextOutWithOutline(x, it->mapPosInfo.y_LT + 2, mapName.c_str(), 0, 0xffffff, 0, 0, 12, true);

			CGameMode* gameMode = g_modeMgr.GetGameMode();
			std::string rswName = (char*)gameMode->SendMsg(MM_QUERYRSWNAME, 0, 0, 0);
			if (rswName == it->mapName) {
				int starX = GetTextWidth("\xa1\xda", 0, 0, 12, true);
				starX = (starX - it->mapPosInfo.x_LT - it->mapPosInfo.x_RB) / 2;
				if (starX < 0)
					starX = -starX;
				TextOutWithOutline(starX, it->mapPosInfo.y_LT + 16, "\xa1\xda", 0, 0x8080f0, 0xffffff, 0, 12, true);
			}
		}

		++it;
	}
}


void UIRoMapWnd::DrawThisMapInfo()
{
	int x = 0;
	int w;
	m_thisPosInfo = 0;
	if (!m_showThisPosInfo || !m_miniMapInfo)
		return;

	w = 0;
	bool drawLeftMap = false;
	int th = GetTextHeight(" ", 0, 0, 12, true) + 3;
	MapPosInfo tmpPosInfo = m_showThisPosInfo->mapPosInfo;

	if (m_miniMapInfo->x_RB < tmpPosInfo.x_RB) {
		drawLeftMap = true;
		x = m_miniMapInfo->x_LT - 10;
	} else {
		x = m_miniMapInfo->x_RB + 10;
	}
	int y = m_miniMapInfo->y_LT;

	int i = 0;
	std::list<std::string>::iterator it = m_showThisPosInfo->cNameList.begin();
	while (it != m_showThisPosInfo->cNameList.end()) {
		int tw = GetTextWidth((*it).c_str(), 0, 0, 12, true);
		if (w < tw)
			w = tw;
		++it;
		++i;
	}

	w += 10;
	int h = i * th + 7;
	if (drawLeftMap)
		x -= w;

	if (x + w > g_renderer->m_width) {
		x = tmpPosInfo.x_LT - w - 10;
	} else {
		if (!drawLeftMap)
			y += 17;
	}

	if (y + h > g_renderer->m_height)
		y = g_renderer->m_height - h + 10;

	it = m_showThisPosInfo->cNameList.begin();
	i = 0;
	while (it != m_showThisPosInfo->cNameList.end()) {
		unsigned long nameColor = 0xffffff;
		if (strcmp((*it).c_str(), g_session.GetCharName()) == 0)
			nameColor = 0x8080f0;
		TextOutWithOutline(x + 5, y + 5 + i * th, (*it).c_str(), 0, nameColor, 0, 0, 12, true);
		++it;
		++i;
	}

	m_thisPosInfo = new MapPosInfo(x, y, w, h);
	DrawBorderLine(x, y, w, h, 0xffffff);
}


void UIRoMapWnd::ShowAllMapName()
{
	int maxNum = g_session.m_divisionMapPosIndexTable.size();

	for (int i = 0; i < maxNum; ++i) {
		std::map<std::string, MapPosInfo>::iterator it = g_session.m_divisionMapPosTable[i].begin();
		int j = 0;
		while (it != g_session.m_divisionMapPosTable[i].end()) {
			MapPosInfo mapPosInfo = (*it).second;
			int w = mapPosInfo.x_RB - mapPosInfo.x_LT;
			int h = mapPosInfo.y_RB - mapPosInfo.y_LT;
			DrawBorderLine(mapPosInfo.x_LT, mapPosInfo.y_LT, w, h, 0x8f8fbc);

			{
				std::string mapName = (*it).first;
				if (mapName.size() > 4)
					mapName.resize(mapName.size() - 4);

				int x = GetTextWidth(mapName.c_str(), 0, 0, 10, false);
				x = (x - mapPosInfo.x_LT - mapPosInfo.x_RB) / 2;
				if (x < 0)
					x = -x;

				int y = mapPosInfo.y_LT + 2;
				if (j % 2)
					y = mapPosInfo.y_RB - 12;

				TextOutA(x, y, mapName.c_str(), 0, 0, 10, 0xffffff);
			}

			++it;
			++j;
		}
	}
}

void UIRoMapWnd::ShowThisMapName(int mx, int my)
{
	m_showMiniMap = 0;

	int division = GetThisDivisionInMap(mx, my);
	if (division == -1)
		return;

	std::map<std::string, MapPosInfo>::iterator it = g_session.m_divisionMapPosTable[division].begin();
	while (it != g_session.m_divisionMapPosTable[division].end()) {
		if ((*it).second.x_LT < mx &&
			(*it).second.x_RB > mx &&
			(*it).second.y_LT < my &&
			(*it).second.y_RB > my) {
			m_showMiniMap = new PartyMapInfo;
			m_showMiniMap->mapName = (*it).first;
			m_showMiniMap->mapPosInfo = (*it).second;
			Invalidate();
			return;
		}
		++it;
	}
}

int UIRoMapWnd::GetThisDivisionInMap(int mx, int my)
{
	if (m_showMiniMap &&
		m_showMiniMap->mapPosInfo.x_LT < mx &&
		m_showMiniMap->mapPosInfo.x_RB > mx &&
		m_showMiniMap->mapPosInfo.y_LT < my &&
		m_showMiniMap->mapPosInfo.y_RB > my)
		return -1;

	std::map<int, MapPosInfo>::iterator it = g_session.m_divisionMapPosIndexTable.begin();
	while (it != g_session.m_divisionMapPosIndexTable.end()) {
		if ((*it).second.x_LT < mx &&
			(*it).second.x_RB > mx &&
			(*it).second.y_LT < my &&
			(*it).second.y_RB > my)
			return (*it).first;
		++it;
	}

	return -1;
}
void UIRoMapWnd::ShowThisMapPartyInfo(int mx, int my)
{
	m_showThisPosInfo = 0;
	std::list<PartyMapInfo>::iterator it = m_partyPosList.begin();
	m_showThisPosInfo = 0;
	while (it != m_partyPosList.end()) {
		if ((*it).mapPosInfo.x_LT < mx &&
			(*it).mapPosInfo.x_RB > mx &&
			(*it).mapPosInfo.y_LT < my &&
			(*it).mapPosInfo.y_RB > my) {
			m_showThisPosInfo = &(*it);
			Invalidate();
			return;
		}
		++it;
	}
	Invalidate();
}

bool UIRoMapWnd::IsInSameMap(std::string mapName, std::string cName)
{
	std::list<PartyMapInfo>::iterator it = m_partyPosList.begin();
	while (it != m_partyPosList.end()) {
		if ((*it).mapName == mapName) {
			(*it).cNameList.push_back(cName);
			return true;
		}
		++it;
	}

	return false;
}

void UIRoMapWnd::GetPartyFriendMapInfo()
{
	int num = g_session.GetNumParty();

	if (num > 0) {
		for (int i = 0; i < num; ++i) {
			FRIEND_INFO friendInfo = g_session.GetPartyInfoBySequence(i);

			if (friendInfo.isValid && friendInfo.state == GROUPMEMBER_STATE_CONNECTED) {
				if (!IsInSameMap(friendInfo.mapName, friendInfo.characterName)) {
					PartyMapInfo partyMapInfo;
					partyMapInfo.mapName = friendInfo.mapName;
					partyMapInfo.cNameList.push_back(friendInfo.characterName);

					if (GetPartyFriendPos(&partyMapInfo))
						m_partyPosList.push_back(partyMapInfo);
				}
			}
		}
	} else {
		CGameMode* gameMode = g_modeMgr.GetGameMode();
		std::string rswName = (const char*)gameMode->SendMsg(MM_QUERYRSWNAME, 0, 0, 0);

		PartyMapInfo partyMapInfo;
		partyMapInfo.mapName = rswName;
		partyMapInfo.cNameList.push_back(std::string(g_session.GetCharName()));

		if (GetPartyFriendPos(&partyMapInfo))
			m_partyPosList.push_back(partyMapInfo);
	}
}


bool UIRoMapWnd::GetPartyFriendPos(PartyMapInfo* partyMapInfo)
{
	std::map<std::string, MapPosInfo>::iterator it = g_session.m_mapPosTable.find(partyMapInfo->mapName);
	if (it != g_session.m_mapPosTable.end()) {
		partyMapInfo->mapPosInfo = (*it).second;
		g_session.CalculateMapPos(&partyMapInfo->mapPosInfo);
		return true;
	}
	return false;
}

void UIRoMapWnd::InitInfo()
{
	m_partyPosList.clear();
	m_miniMapTexture = 0;
	m_thisPosInfo = 0;
	m_miniMapInfo = 0;
	m_showMiniMap = 0;
	m_showThisPosInfo = 0;
	m_showAllMapName = 0;
}


UIMiniBtnWnd::UIMiniBtnWnd()
{
	m_wndID = (WINDOWID)-1;
}

UIMiniBtnWnd::~UIMiniBtnWnd()
{
}

void UIMiniBtnWnd::OnDraw()
{
	if (m_id < 0)
		return;
	UIWindow* w = g_windowMgr.QueryWindow(m_wndID);
	if (w && w->IsShow()) {
		DrawBitmap(0, 0, m_OnImgRes, false);
	} else {
		DrawBitmap(0, 0, m_OffImgRes, false);
	}
}

void UIMiniBtnWnd::OnLBtnDown(int x, int y)
{
	SendMsg(this, UM_COMMAND, 0xcd, 0, 0, 0);
}

int UIMiniBtnWnd::SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_COMMAND:
	case 0xcd: {
		UIWindow* w = g_windowMgr.QueryWindow(m_wndID);
		if (w) {
			w->SetShow(w->IsShow() == 0);
			w->SendMsg(this, UM_COMMAND, 0xcd, 0, 0, 0);
		}
		Invalidate();
		break;
	}
	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}
