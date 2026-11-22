#include "common.h"
#include "device/Sound.h"
#include "HP_Tetris.h"
#include "window/UIFrameWnd.h"
#include "GameMode.h"
#include "Session.h"
#include "Mode.h"
#include "window/UISys.h"
#include "resource/Action.h"
#include <stdio.h>

extern Tetris*	TT;

Tetris* TT;
extern CSession g_session;
extern short g_BS_Job;
extern short game_TetrisPlayed;
extern short g_menuSelect;
extern bool g_WordDisplay;
extern int g_curStance;
extern int g_Frame;
extern CModeMgr g_modeMgr;

Tetris::Tetris()
{
	int i, j;

	for (i = 0; i < 18; i++) {
		for (j = 0; j < 19; j++) {
			Area[i][j] = 0;
			if (i == 3 || i == 14) {
				if (j <= 15)
					Area[i][j] = 3;
			} else if (j == 15) {
				if (i > 3 && i < 14)
					Area[i][j] = 3;
			}
		}
	}

	for (i = 0; i < 4; i++)
		Del_Line[i] = -1;

	char blockType = (char)(rand() % 2 + 1);
	char start = (char)(rand() % 10 + 4);
	for (i = 4; i < 14; i++) {
		if (i != start) {
			Area[i][14] = blockType;
			Area[i][13] = blockType;
			Area[i][12] = blockType;
		} else {
			Area[i][13] = 4;
			Area[i][14] = 4;
		}
	}

	Block_type = 0;
	Block_DelTime = 0;
	AfterFrame = 0;
	DownFrame = 0;
	Percent = 0;
	Word_displaytime = 0;

	T_spr[0] = (CSprRes*)g_resMgr().Get("\xb8\xf3\xbd\xba\xc5\xcd\\freezer.spr", false);
	T_act[0] = (CActRes*)g_resMgr().Get("\xb8\xf3\xbd\xba\xc5\xcd\\freezer.act", false);
	T_spr[1] = (CSprRes*)g_resMgr().Get("\xb8\xf3\xbd\xba\xc5\xcd\\garden_keeper.spr", false);
	T_act[1] = (CActRes*)g_resMgr().Get("\xb8\xf3\xbd\xba\xc5\xcd\\garden_keeper.act", false);
	T_spr[2] = (CSprRes*)g_resMgr().Get("\xb8\xf3\xbd\xba\xc5\xcd\\geographer.spr", false);
	T_act[2] = (CActRes*)g_resMgr().Get("\xb8\xf3\xbd\xba\xc5\xcd\\geographer.act", false);
}

Tetris::~Tetris() {}

extern void PlayWave(const char* waveFileName, float x, float y, float z,
					 int volumeMaxDist, int volumeMinDist, float vFactor);

void Tetris::BlockSetting(short B_Type, short B_Rot, bool BCheck)
{
	char CurBType[4][4];
	const char BTypeA[4][4] = {
		{0, 0, 0, 0},
		{0, 1, 1, 0},
		{0, 1, 1, 0},
		{0, 0, 0, 0},
	};
	const char BTypeB1[4][4] = {
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{1, 1, 1, 1},
		{0, 0, 0, 0},
	};
	const char BTypeB2[4][4] = {
		{0, 0, 1, 0},
		{0, 0, 1, 0},
		{0, 0, 1, 0},
		{0, 0, 1, 0},
	};
	const char BTypeC1[4][4] = {
		{0, 1, 1, 0},
		{0, 0, 1, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 0},
	};
	const char BTypeC2[4][4] = {
		{0, 0, 0, 0},
		{1, 1, 1, 0},
		{1, 0, 0, 0},
		{0, 0, 0, 0},
	};
	const char BTypeC3[4][4] = {
		{0, 1, 0, 0},
		{0, 1, 0, 0},
		{0, 1, 1, 0},
		{0, 0, 0, 0},
	};
	const char BTypeC4[4][4] = {
		{0, 0, 0, 0},
		{0, 0, 1, 0},
		{1, 1, 1, 0},
		{0, 0, 0, 0},
	};
	const char BTypeD1[4][4] = {
		{0, 0, 1, 0},
		{0, 0, 1, 0},
		{0, 1, 1, 0},
		{0, 0, 0, 0},
	};
	const char BTypeD2[4][4] = {
		{0, 0, 0, 0},
		{1, 1, 1, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 0},
	};
	const char BTypeD3[4][4] = {
		{0, 1, 1, 0},
		{0, 1, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 0, 0},
	};
	const char BTypeD4[4][4] = {
		{0, 0, 0, 0},
		{1, 0, 0, 0},
		{1, 1, 1, 0},
		{0, 0, 0, 0},
	};
	const char BTypeE1[4][4] = {
		{0, 1, 0, 0},
		{1, 1, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 0, 0},
	};
	const char BTypeE2[4][4] = {
		{0, 0, 0, 0},
		{1, 1, 1, 0},
		{0, 1, 0, 0},
		{0, 0, 0, 0},
	};
	const char BTypeE3[4][4] = {
		{0, 1, 0, 0},
		{0, 1, 1, 0},
		{0, 1, 0, 0},
		{0, 0, 0, 0},
	};
	const char BTypeE4[4][4] = {
		{0, 1, 0, 0},
		{1, 1, 1, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
	};
	const char BTypeF1[4][4] = {
		{0, 0, 1, 0},
		{0, 1, 1, 0},
		{0, 1, 0, 0},
		{0, 0, 0, 0},
	};
	const char BTypeF2[4][4] = {
		{0, 0, 0, 0},
		{1, 1, 0, 0},
		{0, 1, 1, 0},
		{0, 0, 0, 0},
	};
	const char BTypeG1[4][4] = {
		{0, 1, 0, 0},
		{0, 1, 1, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 0},
	};
	const char BTypeG2[4][4] = {
		{0, 0, 0, 0},
		{0, 1, 1, 0},
		{1, 1, 0, 0},
		{0, 0, 0, 0},
	};
	char PASS;
	int bjob;
	int j;
	int i;

	if (B_Type > 10) {
		bjob = 2;
		B_Type -= 10;
	} else {
		bjob = 1;
	}

	PASS = 0;
	switch (B_Type) {
	case 1:
		memcpy(CurBType, BTypeA, sizeof(CurBType));
		break;
	case 2:
		if (B_Rot % 2 == 0)
			memcpy(CurBType, BTypeB1, sizeof(CurBType));
		else if (B_Rot % 2 == 1)
			memcpy(CurBType, BTypeB2, sizeof(CurBType));
		break;
	case 3:
		if (B_Rot % 4 == 0)
			memcpy(CurBType, BTypeC1, sizeof(CurBType));
		else if (B_Rot % 4 == 1)
			memcpy(CurBType, BTypeC2, sizeof(CurBType));
		else if (B_Rot % 4 == 2)
			memcpy(CurBType, BTypeC3, sizeof(CurBType));
		else if (B_Rot % 4 == 3)
			memcpy(CurBType, BTypeC4, sizeof(CurBType));
		break;
	case 4:
		if (B_Rot % 4 == 0)
			memcpy(CurBType, BTypeD1, sizeof(CurBType));
		else if (B_Rot % 4 == 1)
			memcpy(CurBType, BTypeD2, sizeof(CurBType));
		else if (B_Rot % 4 == 2)
			memcpy(CurBType, BTypeD3, sizeof(CurBType));
		else if (B_Rot % 4 == 3)
			memcpy(CurBType, BTypeD4, sizeof(CurBType));
		break;
	case 5:
		if (B_Rot % 4 == 0)
			memcpy(CurBType, BTypeE1, sizeof(CurBType));
		else if (B_Rot % 4 == 1)
			memcpy(CurBType, BTypeE2, sizeof(CurBType));
		else if (B_Rot % 4 == 2)
			memcpy(CurBType, BTypeE3, sizeof(CurBType));
		else if (B_Rot % 4 == 3)
			memcpy(CurBType, BTypeE4, sizeof(CurBType));
		break;
	case 6:
		if (B_Rot % 2 == 0)
			memcpy(CurBType, BTypeF1, sizeof(CurBType));
		else if (B_Rot % 2 == 1)
			memcpy(CurBType, BTypeF2, sizeof(CurBType));
		break;
	case 7:
		if (B_Rot % 2 == 0)
			memcpy(CurBType, BTypeG1, sizeof(CurBType));
		else if (B_Rot % 2 == 1)
			memcpy(CurBType, BTypeG2, sizeof(CurBType));
		break;
	}

	if (BCheck == true) {
		for (i = 0; i < 4; ++i) {
			for (j = 0; j < 4; ++j) {
				if (CurBType[i][j] > 0 && Area[Block_X + i][Block_Y + j] != 0) {
					PASS = 1;
					break;
				}
			}
		}
		if (PASS == 1) {
			for (i = 0; i < 4; ++i) {
				for (j = 0; j < 4; ++j) {
					if (CurBType[i][j] > 0 && Area[Block_X + i + 1][Block_Y + j] != 0) {
						PASS = 2;
						break;
					}
				}
			}
		}
		if (PASS == 2) {
			for (i = 0; i < 4; ++i) {
				for (j = 0; j < 4; ++j) {
					if (CurBType[i][j] > 0 && Area[Block_X + i - 1][Block_Y + j] != 0) {
						PASS = 3;
						break;
					}
				}
			}
		}
		if (PASS == 3) {
			for (i = 0; i < 4; ++i) {
				for (j = 0; j < 4; ++j) {
					if (CurBType[i][j] > 0 && Area[Block_X + i + 2][Block_Y + j] != 0) {
						PASS = 4;
						break;
					}
				}
			}
		}

		if (PASS == 4) {
			--Block_rot;
			return;
		} else if (PASS == 3) {
			Block_X += 2;
			Block_fX += 60;
		} else if (PASS == 2) {
			--Block_X;
			Block_fX -= 30;
		} else if (PASS == 1) {
			++Block_X;
			Block_fX += 30;
		}
	}

	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 4; ++j)
			Block[i][j] = (short)(CurBType[i][j] * bjob);
	}

}

void Tetris::AreaCheck()
{
	int j;
	char PASS;

	PASS = 0;
	if (Block_DelTime != 0)
		return;

	for (j = 0; j < 15; j++) {
		int oldCount = 0;
		int newCount = 0;
		short* p = &Area[4][j];
		int r;
		for (r = 0; r < 10; r++) {
			if (*p == 1)
				oldCount++;
			if (*p == 2)
				newCount++;
			p += 19;
		}
		if (oldCount == 10 || newCount == 10) {
			for (int k = 0; k < 4; k++) {
				if (Del_Line[k] == -1) {
					Del_Line[k] = (short)j;
					Block_DelTime = 1;
					AfterFrame = 0;
					break;
				}
			}
			if (!PASS) {
				PlayWave("effect\\\xb9\xf6\xbc\xad\xc5\xa9.wav", 0, 0, 0, 250, 40, 1.0f);
				PASS = 1;
			}
		}
	}
}

void Tetris::AreaDown()
{
	int i;

	for (i = 0; i < 19; i++)
		LineDownSpeed[i] = 0;

	for (i = 0; i < 4; i++) {
		if (Del_Line[i] >= 0) {
			for (int r = 4; r < 14; r++)
				Area[r][Del_Line[i]] = 0;
			for (int k = Del_Line[i]; k >= 0; k--)
				LineDownSpeed[k]++;
		}
	}

	DownFrame = 0;
}

void Tetris::AreaInfluence()
{
	for (int i = 0; i < 4; i++) {
		if (Del_Line[i] >= 0) {
			for (int r = 4; r < 14; r++) {
				if (Del_Line[i] - 1 >= 0 && Area[r][Del_Line[i] - 1] > 0)
					Area[r][Del_Line[i] - 1] = Area[r][Del_Line[i]];
				if (Del_Line[i] + 1 <= 14 && Area[r][Del_Line[i] + 1] > 0)
					Area[r][Del_Line[i] + 1] = Area[r][Del_Line[i]];
			}
		}
	}
}

void Tetris::AreaDelete()
{
	int i, j, k;

	for (i = 0; i < 4; i++) {
		if (Del_Line[i] >= 0) {
			for (j = 4; j < 14; j++)
				Area[j][Del_Line[i]] = 0;

			for (k = Del_Line[i]; k > 0; k--)
				for (j = 4; j < 14; j++)
					Area[j][k] = Area[j][k - 1];

			for (j = 4; j < 14; j++)
				Area[j][0] = 0;

			if (g_BS_Job == 0x0fab)
				Percent += 15;
			else
				Percent += 10;
		}
	}

	if (Percent >= 100) {
		Percent = 100;
		g_session.HP_MakingItem();
		Block_DelTime = 4;
		AfterFrame = 0;
		game_TetrisPlayed++;
		g_menuSelect = 1;
		PlayWave("effect\\complete.wav", 0, 0, 0, 250, 40, 1.0f);
	}

	for (i = 0; i < 4; i++)
		Del_Line[i] = -1;
	g_WordDisplay = true;
	Word_displaytime = timeGetTime() + 3000;
}
void Tetris::CreateFiller()
{
	short i;
	short StartY;
	short j;

	for (i = 4; i < 14; i++) {
		StartY = 0;
		for (j = 0; j < 15; j++) {
			if (Area[i][j] > 0 && StartY == 0)
				StartY = j;
			else if (Area[i][j] == 0 && StartY > 0 && j > 0) {
				if (Area[i + 1][j] != 0 && Area[i - 1][j] != 0) {
					Area[i][j] = 4;
					return;
				}
			}
		}
	}
}

void Tetris::DrawAreaBlocks(int Back_X, int Back_Y)
{
	int mc;
	char temp[256];
	int iy;
	RECT rect;

	rect.left = 0; rect.top = 0; rect.right = 255; rect.bottom = 255;
	sprintf(temp, "effect\\white02.bmp");

	for (int ix = 0; ix < 18; ix++) {
		for (iy = 0; iy < 19; iy++) {
			char PASS = 0;
			short down = 0;
			if (Block_DelTime == 1) {
				for (int i = 0; i < 4; i++) {
					if (Del_Line[i] == iy)
						PASS = 1;
				}
			} else if (Block_DelTime == 2) {
				down = DownFrame * LineDownSpeed[iy] * 5;
			} else if (Block_DelTime == 3) {
				PASS = 1;
			}

			short area = Area[ix][iy];
			if (area == 1) {
				if (Block_DelTime != 3) {
					rect.left = ix * 30 + Back_X + 3; rect.top = iy * 30 + down + Back_Y + 3; rect.right = rect.left + 26; rect.bottom = rect.top + 26;
					g_modeMgr.GetGameMode()->DrawBackGround2D(&rect, 0, 0.01f, 0x22ddffdd, g_texMgr.GetTexture(temp, false), UVRECT(0, 0, 1, 1));
					rect.left = ix * 30 + Back_X + 5; rect.top = iy * 30 + down + Back_Y + 5; rect.right = rect.left + 22; rect.bottom = rect.top + 22;
					g_modeMgr.GetGameMode()->DrawBackGround2D(&rect, 0, 0.01f, 0x2255ff55, g_texMgr.GetTexture(temp, false), UVRECT(0, 0, 1, 1));
				}

				if (PASS == 1) {
					mc = T_act[1]->GetMotionCount(0);
					int alpha = 200 - AfterFrame * 6;
					if (alpha < 0)
						alpha = 0;
					g_modeMgr.GetGameMode()->DrawSprite(ix * 30 + Back_X + 14, AfterFrame + iy * 30 + Back_Y + 24, T_act[1], T_spr[1], 0, (g_Frame / 5) % mc, 0.5f, 0.0f, (alpha << 24) + 0xffffff);
					g_modeMgr.GetGameMode()->DrawSprite(ix * 30 + Back_X + 14, iy * 30 - AfterFrame + Back_Y + 24, T_act[1], T_spr[1], 0, (g_Frame / 5) % mc, 0.5f, 0.0f, (alpha << 24) + 0xffffff);
					if (Block_DelTime == 3) {
						g_modeMgr.GetGameMode()->DrawSprite(ix * 30 + AfterFrame + Back_X + 14, iy * 30 + Back_Y + 24, T_act[1], T_spr[1], 0, (g_Frame / 5) % mc, 0.5f, 0.0f, (alpha << 24) + 0xffffff);
						g_modeMgr.GetGameMode()->DrawSprite(ix * 30 - AfterFrame + Back_X + 14, iy * 30 + Back_Y + 24, T_act[1], T_spr[1], 0, (g_Frame / 5) % mc, 0.5f, 0.0f, (alpha << 24) + 0xffffff);
					} else {
						g_modeMgr.GetGameMode()->DrawSprite(ix * 30 + Back_X + 14, iy * 30 + Back_Y + 24, T_act[1], T_spr[1], 0, (g_Frame / 5) % mc, 0.5f, 0.0f, (alpha << 24) + 0xffffff);
					}
				} else {
					mc = T_act[1]->GetMotionCount(0);
					g_modeMgr.GetGameMode()->DrawSprite(ix * 30 + Back_X + 14, iy * 30 + down + Back_Y + 24, T_act[1], T_spr[1], 0, (g_Frame / 5) % mc, 0.5f, 0.0f, 0xccffffff);
				}
			} else if (area == 2) {
				if (Block_DelTime != 3) {
					rect.left = ix * 30 + Back_X + 3; rect.top = iy * 30 + down + Back_Y + 3; rect.right = rect.left + 26; rect.bottom = rect.top + 26;
					g_modeMgr.GetGameMode()->DrawBackGround2D(&rect, 0, 0.01f, 0x22ddffdd, g_texMgr.GetTexture(temp, false), UVRECT(0, 0, 1, 1));
					rect.left = ix * 30 + Back_X + 5; rect.top = iy * 30 + down + Back_Y + 5; rect.right = rect.left + 22; rect.bottom = rect.top + 22;
					g_modeMgr.GetGameMode()->DrawBackGround2D(&rect, 0, 0.01f, 0x2255ff55, g_texMgr.GetTexture(temp, false), UVRECT(0, 0, 1, 1));
				}

				if (PASS == 1) {
					mc = T_act[2]->GetMotionCount(0);
					int alpha = 200 - AfterFrame * 6;
					if (alpha < 0)
						alpha = 0;
					g_modeMgr.GetGameMode()->DrawSprite(ix * 30 + Back_X + 11, AfterFrame + iy * 30 + Back_Y + 24, T_act[2], T_spr[2], 0, (g_Frame / 5) % mc, 0.5f, 0.0f, (alpha << 24) + 0xffffff);
					g_modeMgr.GetGameMode()->DrawSprite(ix * 30 + Back_X + 11, iy * 30 - AfterFrame + Back_Y + 24, T_act[2], T_spr[2], 0, (g_Frame / 5) % mc, 0.5f, 0.0f, (alpha << 24) + 0xffffff);
					if (Block_DelTime == 3) {
						g_modeMgr.GetGameMode()->DrawSprite(ix * 30 + AfterFrame + Back_X + 11, iy * 30 + Back_Y + 24, T_act[2], T_spr[2], 0, (g_Frame / 5) % mc, 0.5f, 0.0f, (alpha << 24) + 0xffffff);
						g_modeMgr.GetGameMode()->DrawSprite(ix * 30 - AfterFrame + Back_X + 11, iy * 30 + Back_Y + 24, T_act[2], T_spr[2], 0, (g_Frame / 5) % mc, 0.5f, 0.0f, (alpha << 24) + 0xffffff);
					} else {
						g_modeMgr.GetGameMode()->DrawSprite(ix * 30 + Back_X + 11, iy * 30 + Back_Y + 24, T_act[2], T_spr[2], 0, (g_Frame / 5) % mc, 0.5f, 0.0f, (alpha << 24) + 0xffffff);
					}
				} else {
					mc = T_act[2]->GetMotionCount(0);
					g_modeMgr.GetGameMode()->DrawSprite(ix * 30 + Back_X + 11, iy * 30 + down + Back_Y + 24, T_act[2], T_spr[2], 0, (g_Frame / 5) % mc, 0.5f, 0.0f, 0xccffffff);
				}
			} else if (area == 4) {
				rect.left = ix * 30 + Back_X + 3; rect.top = iy * 30 + down + Back_Y + 3; rect.right = rect.left + 26; rect.bottom = rect.top + 26;
				g_modeMgr.GetGameMode()->DrawBackGround2D(&rect, 0, 0.01f, 0x22ddffdd, g_texMgr.GetTexture(temp, false), UVRECT(0, 0, 1, 1));
				rect.left = ix * 30 + Back_X + 5; rect.top = iy * 30 + down + Back_Y + 5; rect.right = rect.left + 22; rect.bottom = rect.top + 22;
				g_modeMgr.GetGameMode()->DrawBackGround2D(&rect, 0, 0.01f, 0x2255ff55, g_texMgr.GetTexture(temp, false), UVRECT(0, 0, 1, 1));

				mc = T_act[0]->GetMotionCount(0);
				g_modeMgr.GetGameMode()->DrawSprite(ix * 30 + Back_X + 15, iy * 30 + down + Back_Y + 24, T_act[0], T_spr[0], 0, (g_Frame / 5) % mc, 0.4f, 0.0f, 0xccffffff);
			} else if (area == 3) {
				rect.left = ix * 30 + Back_X + 3; rect.top = iy * 30 + Back_Y + 3; rect.right = rect.left + 26; rect.bottom = rect.top + 26;
				g_modeMgr.GetGameMode()->DrawBackGround2D(&rect, 0, 0.01f, 0x66ddddff, g_texMgr.GetTexture(temp, false), UVRECT(0, 0, 1, 1));
				rect.left = ix * 30 + Back_X + 5; rect.top = iy * 30 + Back_Y + 5; rect.right = rect.left + 22; rect.bottom = rect.top + 22;
				g_modeMgr.GetGameMode()->DrawBackGround2D(&rect, 0, 0.01f, 0xbb5555ff, g_texMgr.GetTexture(temp, false), UVRECT(0, 0, 1, 1));

				mc = T_act[0]->GetMotionCount(0);
				g_modeMgr.GetGameMode()->DrawSprite(ix * 30 + Back_X + 15, iy * 30 + Back_Y + 21, T_act[0], T_spr[0], 0, (g_Frame / 5) % mc, 0.4f, 0.0f, 0x99ffffff);
			}
		}
	}
}

void UIWindowMgr::BS_Tetris()
{
	char temp[256];
	int ix;
	RECT rect;

	rect.left = 0; rect.top = 0; rect.right = 255; rect.bottom = 255;

	if (TT->Block_DelTime >= 1) {
		++TT->AfterFrame;
		if (TT->Block_DelTime == 2) {
			++TT->DownFrame;
			if (TT->DownFrame > 6)
				TT->DownFrame = 6;
		}

		if (TT->Block_DelTime == 3) {
			if (TT->AfterFrame == 60) {
				SetcurStance_B(0);
				return;
			}
		} else if (TT->Block_DelTime == 4) {
			if (TT->AfterFrame >= 120) {
				SetcurStance_B(0);
				return;
			}
		} else {
			if (TT->AfterFrame == 28)
				TT->AreaInfluence();
			if (TT->AfterFrame == 40) {
				TT->AreaDown();
				TT->Block_DelTime = 2;
			}
			if (TT->AfterFrame == 45) {
				TT->Block_DelTime = 0;
				TT->AreaDelete();
			}
		}
	} else if (TT->Block_type == 0) {
		if (TT->Area[8][2] > 0 || TT->Area[9][2] > 0) {
			TT->Block_DelTime = 3;
			TT->AfterFrame = 0;
			PlayWave("effect\\bs_refinefailed.wav", 0, 0, 0, 250, 40, 1.0f);
		}

		TT->Block_type = (short)(rand() % 5 + 1);
		TT->Block_rot = 0;
		if (rand() % 2 == 1)
			TT->Block_type += 10;
		TT->BlockSetting(TT->Block_type, TT->Block_rot, false);
		TT->Block_X = 7;
		TT->Block_Y = 0;
		TT->Block_fX = (short)(TT->Block_X * 30 + 3);
		TT->Block_fY = (short)(TT->Block_Y * 30 + 113);
		TT->CreateFiller();
		if (g_curStance >= 3 && g_curStance <= 4)
			TT->Block_movetime = timeGetTime() + 500;
		else
			TT->Block_movetime = timeGetTime() + 10000;
	} else {
		char PASS = 0;
		if (TT->Block_movetime < timeGetTime()) {
			for (ix = 0; ix < 4; ++ix) {
				for (int iy = 0; iy < 4; ++iy) {
					if (TT->Block[ix][iy] > 0
						&& TT->Area[TT->Block_X + ix][TT->Block_Y + iy + 1] > 0) {
						PASS = 1;
					}
				}
			}

			if (PASS == 1) {
				for (ix = 0; ix < 4; ++ix) {
					for (int iy = 0; iy < 4; ++iy) {
						if (TT->Block[ix][iy] > 0) {
							TT->Area[TT->Block_X + ix][TT->Block_Y + iy] = TT->Block[ix][iy];
						}
					}
				}
				TT->Block_type = 0;
				TT->AreaCheck();
			} else if (PASS == 0) {
				++TT->Block_Y;
				if (g_curStance >= 3 && g_curStance <= 4)
					TT->Block_movetime = timeGetTime() + 500;
				else
					TT->Block_movetime = timeGetTime() + 10000;
			}
		}

		int x = TT->Block_X * 15;
		if (TT->Block_fX > x * 2 + 33)
			TT->Block_fX -= 10;
		else if (TT->Block_fX < x * 2 - 27)
			TT->Block_fX += 10;

		int y = TT->Block_Y * 15;
		if (TT->Block_fY > y * 2 + 143)
			TT->Block_fY -= 10;
		else if (TT->Block_fY < y * 2 + 83)
			TT->Block_fY += 10;

		if (TT->Block_fX > TT->Block_X * 30 + 3)
			TT->Block_fX -= 5;
		else if (TT->Block_fX < TT->Block_X * 30 + 3)
			TT->Block_fX += 5;

		if (TT->Block_fY > TT->Block_Y * 30 + 113)
			TT->Block_fY -= 5;
		else if (TT->Block_fY < TT->Block_Y * 30 + 113)
			TT->Block_fY += 5;
	}

	sprintf(temp, "effect\\white02.bmp");
	rect.left = 79; rect.top = 89; rect.right = 441; rect.bottom = 571;
	g_modeMgr.GetGameMode()->DrawBackGround2D(&rect, 0, 0.01f, 0x335555ff, g_texMgr.GetTexture(temp, false), UVRECT(0, 0, 1, 1));

	TT->DrawAreaBlocks(-11, 89);

	if (TT->Block_type > 0 && TT->Block_DelTime != 3) {
		for (ix = 0; ix < 4; ++ix) {
			for (int iy = 0; iy < 4; ++iy) {
				if (TT->Block[ix][iy] == 1) {
					int motionCount = TT->T_act[1]->GetMotionCount(0);
					g_modeMgr.GetGameMode()->DrawSprite(TT->Block_fX + ix * 30, TT->Block_fY + iy * 30,
						TT->T_act[1], TT->T_spr[1], 0, (g_Frame / 5) % motionCount, 0.5f, 0.0f, 0xccffffff);
				} else if (TT->Block[ix][iy] == 2) {
					int motionCount = TT->T_act[2]->GetMotionCount(0);
					g_modeMgr.GetGameMode()->DrawSprite(TT->Block_fX + ix * 30 - 3, TT->Block_fY + iy * 30,
						TT->T_act[2], TT->T_spr[2], 0, (g_Frame / 5) % motionCount, 0.5f, 0.0f, 0xccffffff);
				}
			}
		}
	}
}


void	UIWindowMgr :: BS_StartFasterTetris()
{
	if (TT) {
		TT->Block_movetime = timeGetTime() + 500;
	}
}
