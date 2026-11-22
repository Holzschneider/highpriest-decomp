#ifndef HIGHPRIEST_HP_TETRIS_H
#define HIGHPRIEST_HP_TETRIS_H


class CActRes;
class CSprRes;
class Tetris;

class Tetris {
public:
	Tetris();
	~Tetris();
	void BlockSetting(short B_Type, short PASS, bool B_Rot);
	void AreaCheck();
	void AreaDelete();
	void AreaInfluence();
	void AreaDown();
	void DrawAreaBlocks(int Back_X, int Back_Y);
	void CreateFiller();
	short Area[18][19];
	short Block[4][4];
	short LineDownSpeed[19];
	short Block_type;
	short Block_rot;
	short Block_X;
	short Block_Y;
	short Block_fX;
	short Block_fY;
	short Del_Line[4];
	short Percent;
	unsigned long Block_movetime;
	unsigned long Word_displaytime;
	int AfterFrame;
	int DownFrame;
	unsigned long Block_DelTime;
	CSprRes* T_spr[3];
	CActRes* T_act[3];
};

#endif
