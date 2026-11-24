#include "common.h"
#include "device/Sound.h"
#include "MercenaryAI.h"
#include "MercLuaFunc.h"

CMercenaryAI::CMercenaryAI(int merType)
{
	m_luaVM = NULL;
	m_mt = 0;
	m_delay = 0;
	m_merType = merType;
	m_tick = 0;
}



CMercenaryAI::~CMercenaryAI()
{
}

int CMercenaryAI::Open(const char* filename)
{
	return 0;
}

void CMercenaryAI::Close()
{
}

void CMercenaryAI::SetEnemyID(int EnemyID)
{
}

int CMercenaryAI::Proc(unsigned int Gid)
{
	return 0;
}







void CMercenaryAI::RegisterFunc()
{
}
