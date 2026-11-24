#ifndef HIGHPRIEST_MERCENARYAI_H
#define HIGHPRIEST_MERCENARYAI_H


extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

enum {
	NONE_CMD,
	MOVE_CMD,
	STOP_CMD,
	ATTACK_OBJECT_CMD,
	ATTACK_AREA_CMD,
	PATROL_CMD,
	HOLD_CMD,
	SKILL_OBJECT_CMD,
	SKILL_AREA_CMD,
	FOLLOW_CMD,
};

struct lua_State;
class CMercenaryAI;

typedef struct _MSG2AI
{
	int command;
	int x;
	int y;
	int skillLevel;
	int skillID;

	DWORD targetID;

	_MSG2AI() { Init(); }
	void Init() { memset(this, 0, sizeof(_MSG2AI)); command = NONE_CMD; }

} MSG2AI;

class CMercenaryAI {
public:
	CMercenaryAI(int merType);
	virtual ~CMercenaryAI();
	int Open(const char* filename);
	int Proc(unsigned int Gid);
	unsigned char IsOpened() { return m_luaVM != 0; }
	void Close();
	void SetEnemyID(int merType);
protected:
	void RegisterFunc();
protected:
	lua_State* m_luaVM;
	int m_mt;
	int m_delay;
	int m_merType;
	int m_tick;
};

#endif
