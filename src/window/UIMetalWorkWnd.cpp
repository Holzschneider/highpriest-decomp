#include "common.h"
#include "../MsgStrings.h"
#include "../GameMode.h"
#include "../Mode.h"
#define ITEMREPAIREV2
#include "../Session.h"
#include "../ItemInfo.h"
#include "UIMetalWorkWnd.h"
#include "../base/Res.h"
#include "../pdb_enums.h"
#include "../resource/Bitmap.h"
#include "../skin.h"
#include "UISys.h"

extern UIWindowMgr g_windowMgr;
extern CModeMgr g_modeMgr;
extern CSession g_session;
extern const char* UIBmp(const char* uiBmpName);
extern const char* MsgStr(MSGSTRINGID id);

typedef std::vector<char const *> PCharList;

UIItemRepairWnd :: UIItemRepairWnd()
{
}

UIItemRepairWnd :: ~UIItemRepairWnd()
{
}

void	UIItemRepairWnd :: OnCreate(int cx, int cy)
{
	int toolW = cx - 24;
	int toolH = cy - 60+5;

	m_metalList = new UIListBox;
	m_metalList->Create2(12, 22, toolW, toolH, false);
	m_metalList->m_bR = 240;
	m_metalList->m_bG = 240;
	m_metalList->m_bB = 240;
	m_metalList->SetId(ID_OK);
	AddChild(m_metalList);

	std::string pathName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\";
	char *buttonName[][3] = 	{
									{ "btn_ok","btn_ok_a", "btn_ok_b",	},
									{ "btn_cancel","btn_cancel_a", "btn_cancel_b",	},
								};

	int ids[] = { ID_OK, ID_CANCEL };
	int pos[][2] =	{
						{ cx-91, cy-(120-96) },
						{ cx-46, cy-(120-96) },
					};

	for (int index = 0; index<2; index++) {
		UIBitmapButton * button = new UIBitmapButton;
		button->SetBitmapName((pathName+buttonName[index][0]+".bmp").c_str(), BS_NORMAL);
		button->SetBitmapName((pathName+buttonName[index][1]+".bmp").c_str(), BS_MOUSEON);
		button->SetBitmapName((pathName+buttonName[index][2]+".bmp").c_str(), BS_PRESSED);
		button->Create(button->GetBitmapWidth(), button->GetBitmapHeight());
		button->Move(pos[index][0], pos[index][1]);
		button->SetId(ids[index]);
		AddChild(button);
	}
	m_defPushId = ID_OK;
}

INT_PTR		UIItemRepairWnd :: SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	char itemDisName[64];
	ITEM_INFO dummyItemInfo;

	switch(message)
	{
		case	UM_ADDITEM:
			{
#ifdef ITEMREPAIREV2
				REPAIRITEM_INFO * repairItemList = ((REPAIRITEM_INFO *) val1) ;
				m_repairItemList.push_back ( *repairItemList ) ;
				m_metalListId.push_back(repairItemList->ITID);
				m_metalListIndex.push_back(repairItemList->index);
				dummyItemInfo.SetItemId(repairItemList->ITID);
#else
				m_metalListId.push_back(val1);
				m_metalListIndex.push_back(val2);
				dummyItemInfo.SetItemId(val1);
#endif

				dummyItemInfo.m_refiningLevel = 0;
				dummyItemInfo.m_isIdentified = TRUE;
				dummyItemInfo.GetItemName(itemDisName);
				m_metalList->AddItem(itemDisName);
			}
			break;

		case	UM_UPPUSH:
			m_metalList->SendMsg(NULL, UM_UPPUSH, 0, 0, 0, 0);
			break;

		case	UM_DOWNPUSH:
			m_metalList->SendMsg(NULL, UM_DOWNPUSH, 0, 0, 0, 0);
			break;

		case	UM_COMMAND:
			switch(val1)
			{
				case	ID_OK:
					{
						if(m_metalList->GetNumItem() == 0)
						{
							g_modeMgr.GetCurMode()->SendMsg(MM_REPAIRITEM_SELECT_FROM_LIST, -1, 0, 0);
							g_windowMgr.DeleteWindow(WID_ITEMREPAIRWND);
						}
						else
						{
							if(m_metalList->GetSelected() < 0) return 0;
#ifdef ITEMREPAIREV2
							std::list<REPAIRITEM_INFO> :: iterator it = m_repairItemList.begin();
							while (it != m_repairItemList.end())
							{
								if ( (*it).index == m_metalListIndex[m_metalList->GetSelected()] )
								{
									g_modeMgr.GetCurMode()->SendMsg(MM_REPAIRITEM_SELECT_FROM_LIST, int(&(*it)), 0, 0);
									break;
								}
								++it;
							}
#else
							g_modeMgr.GetCurMode()->SendMsg(MM_REPAIRITEM_SELECT_FROM_LIST, m_metalListIndex[m_metalList->GetSelected()], 0, 0);
#endif
						}
					}
					g_windowMgr.DeleteWindow(WID_ITEMREPAIRWND);
					break;

				case	ID_CANCEL:
					{
						g_modeMgr.GetCurMode()->SendMsg(MM_REPAIRITEM_SELECT_FROM_LIST, -1, 0, 0);
						g_windowMgr.DeleteWindow(WID_ITEMREPAIRWND);
					}
					break;
			}
			break;
		default:
			return	UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
			break;
	}

	return 0;
}

void	UIItemRepairWnd :: OnDraw()
{

	DWORD rr, gg, bb;
	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(0, 17, m_w, m_h-17, RGB(rr,gg,bb));
	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	DrawBox(1, 17, m_w-2, m_h-17, RGB(rr,gg,bb));

	int loopCnt = (m_w-24)/12 + (((m_w-24)%12)?1:0);
	int i ;
	for (i=0; i<loopCnt; i++) {
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_mid.bmp"), false);
		DrawBitmap((i+1)*12, 0, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_left.bmp"), false);
		DrawBitmap(0, 0, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_right.bmp"), false);
		DrawBitmap(m_w-bitmap->m_width, 0, bitmap, false);
	}

	g_skinMgr.GetColorChipColor(20, 6, rr, gg, bb);
	DrawBox(0, 17, 1, m_h-17-21, RGB(rr, gg, bb));
	DrawBox(m_w-1, 17, 1, m_h-17-21, RGB(rr, gg, bb));

	loopCnt = (m_w-21-21)/21 + (((m_w-21-21)%21)?1:0);
	for (i=0; i<loopCnt; i++) {
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_mid2.bmp"), false);
		DrawBitmap((i+1)*21, m_h-bitmap->m_height, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_left2.bmp"), false);
		DrawBitmap(0, m_h-bitmap->m_height, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_right2.bmp"), false);
		DrawBitmap(m_w-bitmap->m_width, m_h-bitmap->m_height, bitmap, false);
	}

	TextOutWithShadow(16, 3-1, MsgStr(MSI_REPAIRITEMLIST), 0);
}

UIWeaponRefineWnd :: UIWeaponRefineWnd()
{
}

UIWeaponRefineWnd :: ~UIWeaponRefineWnd()
{
}

void	UIWeaponRefineWnd :: OnCreate(int cx, int cy)
{
	int toolW = cx - 24;
	int toolH = cy - 60+5;

	m_metalList = new UIListBox;
	m_metalList->Create2(12, 22, toolW, toolH, false);
	m_metalList->m_bR = 240;
	m_metalList->m_bG = 240;
	m_metalList->m_bB = 240;
	m_metalList->SetId(ID_OK);
	AddChild(m_metalList);

	std::string pathName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\";
	char *buttonName[][3] = 	{
									{ "btn_ok","btn_ok_a", "btn_ok_b",	},
									{ "btn_cancel","btn_cancel_a", "btn_cancel_b",	},
								};

	int ids[] = { ID_OK, ID_CANCEL };
	int pos[][2] =	{
						{ cx-91, cy-(120-96) },
						{ cx-46, cy-(120-96) },
					};

	for (int index = 0; index<2; index++) {
		UIBitmapButton * button = new UIBitmapButton;
		button->SetBitmapName((pathName+buttonName[index][0]+".bmp").c_str(), BS_NORMAL);
		button->SetBitmapName((pathName+buttonName[index][1]+".bmp").c_str(), BS_MOUSEON);
		button->SetBitmapName((pathName+buttonName[index][2]+".bmp").c_str(), BS_PRESSED);
		button->Create(button->GetBitmapWidth(), button->GetBitmapHeight());
		button->Move(pos[index][0], pos[index][1]);
		button->SetId(ids[index]);
		AddChild(button);
	}
	m_defPushId = ID_OK;
}

INT_PTR		UIWeaponRefineWnd :: SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	char itemDisName[64];
	ITEM_INFO dummyItemInfo;

	switch(message)
	{
		case	UM_ADDITEM:
			m_metalListId.push_back(val1);
			m_metalListIndex.push_back(val2);
			dummyItemInfo.SetItemId(val1);

			dummyItemInfo.m_refiningLevel = 0;
			dummyItemInfo.m_isIdentified = TRUE;
			dummyItemInfo.GetItemName(itemDisName);
			m_metalList->AddItem(itemDisName);
			break;

		case	UM_UPPUSH:
			m_metalList->SendMsg(NULL, UM_UPPUSH, 0, 0, 0, 0);
			break;

		case	UM_DOWNPUSH:
			m_metalList->SendMsg(NULL, UM_DOWNPUSH, 0, 0, 0, 0);
			break;

		case	UM_COMMAND:
			switch(val1)
			{
				case	ID_OK:
					{
						if(m_metalList->GetNumItem() == 0)
						{
							g_modeMgr.GetCurMode()->SendMsg(MM_REFINEITEM_SELECT_FROM_LIST, -1, 0, 0);
							g_windowMgr.DeleteWindow(WID_WEAPONREFINEWND);
						}
						else
						{
							if(m_metalList->GetSelected() < 0) return 0;
							g_modeMgr.GetCurMode()->SendMsg(MM_REFINEITEM_SELECT_FROM_LIST, m_metalListIndex[m_metalList->GetSelected()], 0, 0);
						}
					}
					g_windowMgr.DeleteWindow(WID_WEAPONREFINEWND);
					break;

				case	ID_CANCEL:
					{
						g_modeMgr.GetCurMode()->SendMsg(MM_REFINEITEM_SELECT_FROM_LIST, -1, 0, 0);
						g_windowMgr.DeleteWindow(WID_WEAPONREFINEWND);
					}
					break;
			}
			break;
		default:
			return	UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
			break;
	}

	return 0;
}

void	UIWeaponRefineWnd :: OnDraw()
{

	DWORD rr, gg, bb;
	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(0, 17, m_w, m_h-17, RGB(rr,gg,bb));
	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	DrawBox(1, 17, m_w-2, m_h-17, RGB(rr,gg,bb));

	int loopCnt = (m_w-24)/12 + (((m_w-24)%12)?1:0);
	int i ;
	for (i=0; i<loopCnt; i++) {
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_mid.bmp"), false);
		DrawBitmap((i+1)*12, 0, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_left.bmp"), false);
		DrawBitmap(0, 0, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_right.bmp"), false);
		DrawBitmap(m_w-bitmap->m_width, 0, bitmap, false);
	}

	g_skinMgr.GetColorChipColor(20, 6, rr, gg, bb);
	DrawBox(0, 17, 1, m_h-17-21, RGB(rr, gg, bb));
	DrawBox(m_w-1, 17, 1, m_h-17-21, RGB(rr, gg, bb));

	loopCnt = (m_w-21-21)/21 + (((m_w-21-21)%21)?1:0);
	for (i=0; i<loopCnt; i++) {
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_mid2.bmp"), false);
		DrawBitmap((i+1)*21, m_h-bitmap->m_height, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_left2.bmp"), false);
		DrawBitmap(0, m_h-bitmap->m_height, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_right2.bmp"), false);
		DrawBitmap(m_w-bitmap->m_width, m_h-bitmap->m_height, bitmap, false);
	}

	TextOutWithShadow(16, 3-1, MsgStr(MSI_REFINEITEMLIST), 0);
}

UIMakeTargetListWnd :: UIMakeTargetListWnd()
{
}

UIMakeTargetListWnd :: ~UIMakeTargetListWnd()
{
}

void	UIMakeTargetListWnd :: OnCreate(int cx, int cy)
{
	int toolW = cx - 24;
	int toolH = cy - 60+5;

	m_metalList = new UIListBox;
	m_metalList->Create2(12, 22, toolW, toolH, false);
	m_metalList->m_bR = 240;
	m_metalList->m_bG = 240;
	m_metalList->m_bB = 240;
	m_metalList->SetId(ID_OK);
	AddChild(m_metalList);

	std::string pathName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\";
	char *buttonName[][3] = 	{
									{ "btn_ok","btn_ok_a", "btn_ok_b",	},
									{ "btn_cancel","btn_cancel_a", "btn_cancel_b",	},
								};

	int ids[] = { ID_OK, ID_CANCEL };
	int pos[][2] =	{
						{ cx-91, cy-(120-96) },
						{ cx-46, cy-(120-96) },
					};

	for (int index = 0; index<2; index++) {
		UIBitmapButton * button = new UIBitmapButton;
		button->SetBitmapName((pathName+buttonName[index][0]+".bmp").c_str(), BS_NORMAL);
		button->SetBitmapName((pathName+buttonName[index][1]+".bmp").c_str(), BS_MOUSEON);
		button->SetBitmapName((pathName+buttonName[index][2]+".bmp").c_str(), BS_PRESSED);
		button->Create(button->GetBitmapWidth(), button->GetBitmapHeight());
		button->Move(pos[index][0], pos[index][1]);
		button->SetId(ids[index]);
		AddChild(button);
	}
	m_defPushId = ID_OK;
}

INT_PTR		UIMakeTargetListWnd :: SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	char itemDisName[64];
	ITEM_INFO dummyItemInfo;

	switch(message)
	{
		case	UM_ADDITEM:
			m_metalListId.push_back(val1);
			dummyItemInfo.SetItemId(val1);
			dummyItemInfo.m_refiningLevel = 0;
			dummyItemInfo.m_isIdentified = TRUE;
			dummyItemInfo.GetItemName(itemDisName);
			m_metalList->AddItem(itemDisName);
			break;

		case	UM_UPPUSH:
			m_metalList->SendMsg(NULL, UM_UPPUSH, 0, 0, 0, 0);
			break;

		case	UM_DOWNPUSH:
			m_metalList->SendMsg(NULL, UM_DOWNPUSH, 0, 0, 0, 0);
			break;

		case	UM_COMMAND:
			switch(val1)
			{
				case	ID_OK:
					{
						if(m_metalList->GetNumItem() == 0) {
							g_windowMgr.DeleteWindow(WID_MAKETARGETLISTWND);
						} else {
							if(m_metalList->GetSelected() < 0) return 0;

							UIMakeTargetProcessWnd * newWindow = (UIMakeTargetProcessWnd *)g_windowMgr.MakeWindow(WID_MAKETARGETPROCESSWND);
							newWindow->SendMsg(this, UM_SETCURITEM, m_metalListId[m_metalList->GetSelected()], 0, 0, 0);
							newWindow->Move(m_x, m_y);
						}
					}
					g_windowMgr.DeleteWindow(WID_MAKETARGETLISTWND);
					break;

				case	ID_CANCEL:
					{
						CGameMode * gameMode = g_modeMgr.GetGameMode();
						gameMode->SendMsg(MM_REQ_MAKING_ITEM, 0, (int)&dummyItemInfo, 0);
						g_windowMgr.DeleteWindow(WID_MAKETARGETLISTWND);
					}
					break;
			}
			break;
		default:
			return	UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
			break;
	}

	return 0;
}

void	UIMakeTargetListWnd :: OnDraw()
{

	DWORD rr, gg, bb;
	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(0, 17, m_w, m_h-17, RGB(rr,gg,bb));
	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	DrawBox(1, 17, m_w-2, m_h-17, RGB(rr,gg,bb));

	int loopCnt = (m_w-24)/12 + (((m_w-24)%12)?1:0);
	int i ;
	for (i=0; i<loopCnt; i++) {
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_mid.bmp"), false);
		DrawBitmap((i+1)*12, 0, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_left.bmp"), false);
		DrawBitmap(0, 0, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_right.bmp"), false);
		DrawBitmap(m_w-bitmap->m_width, 0, bitmap, false);
	}

	g_skinMgr.GetColorChipColor(20, 6, rr, gg, bb);
	DrawBox(0, 17, 1, m_h-17-21, RGB(rr, gg, bb));
	DrawBox(m_w-1, 17, 1, m_h-17-21, RGB(rr, gg, bb));

	loopCnt = (m_w-21-21)/21 + (((m_w-21-21)%21)?1:0);
	for (i=0; i<loopCnt; i++) {
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_mid2.bmp"), false);
		DrawBitmap((i+1)*21, m_h-bitmap->m_height, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_left2.bmp"), false);
		DrawBitmap(0, m_h-bitmap->m_height, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_right2.bmp"), false);
		DrawBitmap(m_w-bitmap->m_width, m_h-bitmap->m_height, bitmap, false);
	}

	TextOutWithShadow(16, 3-1, MsgStr(MSI_MAKE_LIST), 0);
}

UIMakeTargetProcessWnd :: UIMakeTargetProcessWnd()
{
}

UIMakeTargetProcessWnd :: ~UIMakeTargetProcessWnd()
{
}

void	UIMakeTargetProcessWnd :: OnCreate(int cx, int cy)
{
	std::string pathName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\";
	char *buttonName[][3] = 	{
									{ "btn_ok","btn_ok_a", "btn_ok_b",	},
									{ "btn_cancel","btn_cancel_a", "btn_cancel_b",	},
								};

	int ids[] = { ID_OK, ID_CANCEL };
	int pos[][2] =	{
						{ cx-91, cy-(120-96) },
						{ cx-46, cy-(120-96) },
					};

	for (int index = 0; index<2; index++) {
		UIBitmapButton * button = new UIBitmapButton;
		button->SetBitmapName((pathName+buttonName[index][0]+".bmp").c_str(), BS_NORMAL);
		button->SetBitmapName((pathName+buttonName[index][1]+".bmp").c_str(), BS_MOUSEON);
		button->SetBitmapName((pathName+buttonName[index][2]+".bmp").c_str(), BS_PRESSED);
		button->Create(button->GetBitmapWidth(), button->GetBitmapHeight());
		button->Move(pos[index][0], pos[index][1]);
		button->SetId(ids[index]);
		AddChild(button);
	}
	m_defPushId = ID_OK;
}

void	UIMakeTargetProcessWnd :: DragAndDrop(int x, int y, const DRAG_INFO * const dragInfo)
{
	if(!IsWeaponTargetItem()) return;

	if(dragInfo->m_dragType == DT_FROM_ITEMWND)
	{
		int emptySlot = 3;
		bool hasPropertyItem = false;

		for(int i=0; i<3; ++i)
		{
			if(m_propertyiteminfo[i].IsValid()) {
				hasPropertyItem |= m_propertyiteminfo[i].IsPropertyItem();
			} else if(emptySlot == 3) {
				emptySlot = i;
			}
		}

		if(emptySlot < 3) {
			ITEM_INFO iteminfo = g_session.GetItemInfoByIndex(dragInfo->m_dragItemIndex);

			if(iteminfo.IsValid() && iteminfo.IsMaterialItem())
			{
				if(!(hasPropertyItem && iteminfo.IsPropertyItem()))
				{
					m_propertyiteminfo[emptySlot] = iteminfo;
					m_propertyiteminfo[emptySlot].m_num = 1;
					g_session.DecItem(iteminfo.m_itemIndex, 1);
					if(g_windowMgr.GetItemWnd())
						g_windowMgr.GetItemWnd()->SendMsg(NULL, UM_REFRESHITEM, 0, 0, 0, 0);
					Invalidate();
				}
			}
		}
	}
	g_modeMgr.GetCurMode()->SendMsg(MM_CANCEL_DRAG, 0, 0, 0);
}

void	UIMakeTargetProcessWnd :: OnLBtnDown(int x, int y)
{
	ITEM_INFO itemInfo = GetItemInfo(x, y);
	if (itemInfo.m_itemIndex) {
		CMode * curMode = g_modeMgr.GetCurMode();
		curMode->SendMsg(MM_BEGIN_DRAG_FROM_METALPROCESSWND, (int)itemInfo.m_itemName.c_str(), 0, 0);
	} else {
		UIFrameWnd::OnLBtnDown(x, y);
	}
}

INT_PTR		UIMakeTargetProcessWnd :: SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch(message)
	{
		case	UM_SETCURITEM:
			m_targetItem = val1;
			Invalidate();
			break;
		case	UM_CLEAR_ITEM:
			{
				for(int i=2; i>=0; --i)
				{
					if(m_propertyiteminfo[i].IsValid())
					{
						if(m_propertyiteminfo[i].m_itemName == (char*)val1)
						{
							g_session.AddItem(&m_propertyiteminfo[i]);
							m_propertyiteminfo[i].Reset();
							Invalidate();
							break;
						}
					}
				}
			}
			break;
		case	UM_COMMAND:
			switch(val1)
			{
				case	ID_OK:
					{
						CGameMode * gameMode = g_modeMgr.GetGameMode();
						gameMode->SendMsg(MM_REQ_MAKING_ITEM, m_targetItem, (int)m_propertyiteminfo, 0);
						g_windowMgr.DeleteWindow(WID_MAKETARGETPROCESSWND);
					}
					break;
				case	ID_CANCEL:
					{
						CGameMode * gameMode = g_modeMgr.GetGameMode();
						gameMode->SendMsg(MM_REQ_MAKING_ITEM, 0, (int)m_propertyiteminfo, 0);
						for(int i=0; i<3; ++i)
							if(m_propertyiteminfo[i].IsValid()) g_session.AddItem(m_propertyiteminfo + i);
						if(g_windowMgr.GetItemWnd()) g_windowMgr.GetItemWnd()->SendMsg(NULL, UM_REFRESHITEM, 0, 0, 0, 0);
						g_windowMgr.DeleteWindow(WID_MAKETARGETPROCESSWND);
					}
					break;
			}
			break;
		default:
			return	UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
			break;
	}

	return 0;
}

void	UIMakeTargetProcessWnd :: OnDraw()
{

	DWORD rr, gg, bb;
	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(0, 17, m_w, m_h-17, RGB(rr,gg,bb));
	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	DrawBox(1, 17, m_w-2, m_h-17, RGB(rr,gg,bb));

	int loopCnt = (m_w-24)/12 + (((m_w-24)%12)?1:0);
	int i ;
	for (i=0; i<loopCnt; i++) {
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_mid.bmp"), false);
		DrawBitmap((i+1)*12, 0, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_left.bmp"), false);
		DrawBitmap(0, 0, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_right.bmp"), false);
		DrawBitmap(m_w-bitmap->m_width, 0, bitmap, false);
	}

	g_skinMgr.GetColorChipColor(20, 6, rr, gg, bb);
	DrawBox(0, 17, 1, m_h-17-21, RGB(rr, gg, bb));
	DrawBox(m_w-1, 17, 1, m_h-17-21, RGB(rr, gg, bb));

	loopCnt = (m_w-21-21)/21 + (((m_w-21-21)%21)?1:0);
	for (i=0; i<loopCnt; i++) {
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_mid2.bmp"), false);
		DrawBitmap((i+1)*21, m_h-bitmap->m_height, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_left2.bmp"), false);
		DrawBitmap(0, m_h-bitmap->m_height, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_right2.bmp"), false);
		DrawBitmap(m_w-bitmap->m_width, m_h-bitmap->m_height, bitmap, false);
	}

	if(IsWeaponTargetItem())
		for(i=0; i<3; ++i)
		{
			CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_right2.bmp"), false);
			DrawItemFrame(i*32+4, m_h-bitmap->m_height-32);
			if(m_propertyiteminfo[i].IsValid()) DrawItem(i*32+4+4, m_h-bitmap->m_height-32+4, m_propertyiteminfo[i], FALSE);
		}

	int textLen;
	ITEM_INFO dummyItemInfo;

	dummyItemInfo.SetItemId(m_targetItem);
	dummyItemInfo.m_refiningLevel = 0;
	dummyItemInfo.m_isIdentified = TRUE;

	textLen = DrawItemText(16, 3-1, dummyItemInfo, 0, 0, 12);
	TextOutWithShadow(16+textLen, 3-1, MsgStr(MSI_MAKE_TARGET), 0);

	textLen = DrawItemText(16, 3-1+20, dummyItemInfo, 0, 0, 12);
	TextOutWithShadow(16+textLen, 3-1+20, MsgStr(MSI_REQUIRE_FOR_MAKE_TARGET), 0);

	const PCharList& makeTargetDesc = ITEM_INFO::GetMakeTargetDesc(m_targetItem);

	for(i=0; i<makeTargetDesc.size(); ++i) {
		TextOutA(16, 3-1+20+20+i*20, makeTargetDesc[i], 0, 0, 12, 0);
	}
}


bool	UIMakeTargetProcessWnd :: IsWeaponTargetItem()
{
	switch(m_targetItem)
	{
		case  998:
		case  999:
		case  1000:
		case  994:
		case  995:
		case  997:
		case  996:
			return false;
	}

	return true;
}

ITEM_INFO	UIMakeTargetProcessWnd :: GetItemInfo(int x, int y)
{
	if(!IsWeaponTargetItem()) return ITEM_INFO(0);

	if(y > m_h-24-32 && y < m_h-32)
	{
		if(x>3 && x < 32+3) {
			return m_propertyiteminfo[0];
		} else if(x>32+3 && x < 64+3) {
			return m_propertyiteminfo[1];
		} else if(x>64+3 && x < 96+3) {
			return m_propertyiteminfo[2];
		}
	}
	return ITEM_INFO(0);
}

UIMakeTargetResultWnd :: UIMakeTargetResultWnd()
:	m_succeed(false)
{
	m_targetItem = 0;
}

UIMakeTargetResultWnd :: ~UIMakeTargetResultWnd()
{
}

	void	UIMakeTargetResultWnd :: OnCreate(int cx, int cy)
{
	std::string pathName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\";
	char *buttonName[3] = 	{ "btn_ok","btn_ok_a", "btn_ok_b", };

	int pos[2] =	{ cx-91, cy-(120-96) };

	UIBitmapButton * button = new UIBitmapButton;
	button->SetBitmapName((pathName+buttonName[0]+".bmp").c_str(), BS_NORMAL);
	button->SetBitmapName((pathName+buttonName[1]+".bmp").c_str(), BS_MOUSEON);
	button->SetBitmapName((pathName+buttonName[2]+".bmp").c_str(), BS_PRESSED);
	button->Create(button->GetBitmapWidth(), button->GetBitmapHeight());
	button->Move(pos[0], pos[1]);
	button->SetId(ID_OK);
	AddChild(button);

	m_defPushId = ID_OK;
}

INT_PTR		UIMakeTargetResultWnd :: SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch(message)
	{
		case	UM_SETCURITEM:
			m_targetItem = val1;
			if(val2) m_succeed = true;
			Invalidate();
			break;
		case	UM_COMMAND:
			g_windowMgr.DeleteWindow(WID_MAKETARGETRESULTWND);
			break;
		default:
			return	UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

void	UIMakeTargetResultWnd :: OnDraw()
{

	DWORD rr, gg, bb;
	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(0, 17, m_w, m_h-17, RGB(rr,gg,bb));
	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	DrawBox(1, 17, m_w-2, m_h-17, RGB(rr,gg,bb));

	int loopCnt = (m_w-24)/12 + (((m_w-24)%12)?1:0);
	int i;
	for (i=0; i<loopCnt; i++) {
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_mid.bmp"), false);
		DrawBitmap((i+1)*12, 0, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_left.bmp"), false);
		DrawBitmap(0, 0, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_right.bmp"), false);
		DrawBitmap(m_w-bitmap->m_width, 0, bitmap, false);
	}

	g_skinMgr.GetColorChipColor(20, 6, rr, gg, bb);
	DrawBox(0, 17, 1, m_h-17-21, RGB(rr, gg, bb));
	DrawBox(m_w-1, 17, 1, m_h-17-21, RGB(rr, gg, bb));

	loopCnt = (m_w-21-21)/21 + (((m_w-21-21)%21)?1:0);
	for (i=0; i<loopCnt; i++) {
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_mid2.bmp"), false);
		DrawBitmap((i+1)*21, m_h-bitmap->m_height, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_left2.bmp"), false);
		DrawBitmap(0, m_h-bitmap->m_height, bitmap, false);
	}
	{
		CBitmapRes *bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_right2.bmp"), false);
		DrawBitmap(m_w-bitmap->m_width, m_h-bitmap->m_height, bitmap, false);
	}

	int textLen;
	ITEM_INFO dummyItemInfo;

	dummyItemInfo.SetItemId(m_targetItem);
	dummyItemInfo.m_refiningLevel = 0;
	dummyItemInfo.m_isIdentified = TRUE;

	textLen = DrawItemText(16, 3-1, dummyItemInfo, 0, 0, 12);
	if(m_succeed == true)
		TextOutWithShadow(16+textLen, 3-1, MsgStr(MSI_MAKE_TARGET_SUCCEESS), 0);
	else
		TextOutWithShadow(16+textLen, 3-1, MsgStr(MSI_MAKE_TARGET_FAIL), 0);

	textLen = DrawItemText(16, 3-1+40, dummyItemInfo, 0, 0, 12);
	if(m_succeed == true)
		TextOutWithShadow(16+textLen, 3-1+40, MsgStr(MSI_MAKE_TARGET_SUCCEESS_MSG), 0);
	else
		TextOutWithShadow(16+textLen, 3-1+40, MsgStr(MSI_MAKE_TARGET_FAIL_MSG), 0);
}
