






#ifndef HIGHPRIEST_MERCLUAFUNC_H
#define HIGHPRIEST_MERCLUAFUNC_H

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "common.h"
#include "GameActor.h"
#include "GameMode.h"
#include "GameWorld.h"
#include "View.h"
#include "RagEffect.h"
#include "device/Control.h"
#include "3dlib/Renderer.h"
#include "resource/Attr.h"
#include "base/Util.h"
#include "Session.h"
#include "packet.h"
#include "Picker.h"
#include "resource/Wave.h"
#include "resource/PaletteRes.h"
#include "resource/Emblem.h"
#include "device/Connection.h"




enum {
	V_OWNER,
	V_POSITION,
	V_TYPE,
	V_MOTION,
	V_ATTACKRANGE,
	V_TARGET,
	V_SKILLATTACKRANGE,
	V_HOMUNTYPE,
	V_HP,
	V_SP,
	V_MAXHP,
	V_MAXSP,
	V_MERTYPE,
};


#if defined (_HOMUN)

static void		setfield (lua_State *L,int index, int value)
{
	lua_pushnumber (L,(double)index);
	lua_pushnumber (L,(double)value);
	lua_settable (L,-3);
}


static int l_GetActors (lua_State* L)
{
	CWorld* world = g_modeMgr.GetGameMode()->GetWorld()  ;
	sAssert (world);
	lua_newtable(L);
	int index = 1;
	list <CGameActor *> :: iterator i = world->m_actorList.begin();
	while (i != world->m_actorList.end()) {
		setfield(L,index++,(*i)->GetGID());
		++i;
	}
	setfield (L,index,g_session.GetAid());

	return 1;
}

static int	l_Trace (lua_State* L)
{
	const char* str = (const char*) luaL_checkstring (L,1);
	if (NULL != str) {
		Trace("AI.lua trace: %s\n",str);
	}
	return 0;
}

static int	l_GetTick (lua_State* L)
{
	lua_pushnumber (L,(double)GetTickCount());
	return 1;
}

static int	l_SkillGround (lua_State* L)
{
	DWORD  myID	= (DWORD)	luaL_checknumber (L,1);

	int		values[4];
	values[0] = (int) luaL_checknumber (L,3);
	values[1] = (int) luaL_checknumber (L,4);
	values[2] = (int) luaL_checknumber (L,5);
	values[3] = (int) luaL_checknumber (L,2);


	bool	rst = ( myID == g_session.m_homunGID ) ;

#if defined (_MER)
	rst = ( rst || ( myID == g_session.m_merGID ) ) ;
#endif

	if ( !rst )
		return 0;




	rst = ( SKID_HOMUN_BEGIN < values[0] && values[0] < SKID_HOMUN_LAST ) ;

#if defined (_MER)
	rst = ( rst || ( SKID_MERCENARY_BEGIN < values[0] && values[0] < SKID_MERCENARY_LAST ) ) ;
#endif

	if ( !rst )
		return 0;


	CGameMode* gameMode = g_modeMgr.GetGameMode () ;
	CGameActor*	myself = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (myID);

	if ( myself->m_stateId==ST_MOVE || myself->m_stateId==ST_STAND || myself->m_stateId==ST_DAMAGE )
		gameMode->SendMsg(MM_USE_SKILL_TOGROUND, int(values));

	gameMode->SendMsg(MM_SETLOCKONMOUSE, 0) ;

	return 0 ;
}

static int	l_IsMonster (lua_State* L)
{
	DWORD  id	= (DWORD)	luaL_checknumber (L,1);
	CGameMode* gameMode = g_modeMgr.GetGameMode ();
	sAssert (gameMode);
	CGameActor*	actor = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (id);

	double result = 0;

	if (actor)
	{
		int job = actor->GetJob ( ) ;
		if (job > JT_MON_BEGIN  && job < JT_MONSTER_LAST && actor->GetHeadType() != HEADTYPE_LAST)
		{
#if defined (_MER_R2)
			if ( actor->m_objectType == NPC_MERSOL_TYPE )
				result = 0 ;
			else
#endif
				result = 1 ;
		}
		else if ( g_session.IsPc(job) )
		{
			if (gameMode->GetWorld()->IsPKZone() || gameMode->GetWorld()->IsEventPVPMode())
			{
				if (!g_session.IsMyParty(actor->GetGID()))
					result = 1 ;
			}
			else if (gameMode->GetWorld()->IsSiegeMode())
			{
				if (!g_session.IsMyGuild(actor->GetGdid()))
					result = 1 ;
			}
		}


		if ( result && ( IsEffectStateSpecialHiding(actor->GetEffectState()) || IsEffectStateBurrow(actor->GetEffectState()) || IsEffectStateHiding(actor->GetEffectState())) )
			result = 0 ;
	}
	lua_pushnumber (L, (double)result);
	return 1;
}

static	int	l_TraceAI ( lua_State* L )
{
#if defined (_HOMUN)
	if ( g_session.m_onTraceAI )
	{
		const char * szBuffer = luaL_checkstring (L,1) ;
		if (NULL == szBuffer)
			return 0;

		static string szTemp ;
		if ( szTemp != szBuffer )
			szTemp = szBuffer ;
		else
			return 0 ;

		string s = szBuffer;
		s += "\n";
		OutputDebugString (s.c_str()) ;

		SYSTEMTIME sTime;
		GetLocalTime(&sTime);
		DWORD logDate = (sTime.wYear << 16) | ((short)sTime.wMonth << 8) | (short)sTime.wDay;
		DWORD logTime = (sTime.wHour << 16) | ((short)sTime.wMinute << 8) | (short)sTime.wSecond;

		char *buf;
		CFile fp;
		char curPath[256];
		GetCurrentDirectory(256, curPath);

		int Pos = (int)strstr(curPath, "\\data"), logCnt = 0, size = 0;
		if (Pos) {
			Pos = Pos - (int)curPath;
			memcpy(curPath, curPath, Pos);
		}
		strcat(curPath, "\\TraceAI.txt");

		if (!fp.Open(curPath, CFile::modeRead)) {
			Trace("Read Failed");
		} else {
			size = fp.GetLength() ;
			buf = new char[size];
			fp.Read(buf, size);
			fp.Close();
		}

		fp.Open(curPath, CFile::modeWrite|CFile::modeCreate);
		if (size)
			fp.Write(buf, size);

		int year,month,day,hour,min,sec;
		char msg[256];
		year = (logDate >> 16) & 0xffff;
		month = (logDate >> 8) & 0xff;
		day = logDate & 0xff;
		hour = (logTime >> 16) & 0xff;
		min = (logTime >> 8) & 0xff;
		sec = logTime & 0xff;
		sprintf(msg, "%d year %d month %d day %d hour %d min %d sec",
		year, month, day, hour, min, sec);
		fp.Write(msg,strlen(msg));
		fp.Write ( szBuffer, strlen ( szBuffer ) ) ;

		char e;
		e=13;
		fp.Write(&e, 1);
		e=10;
		fp.Write(&e, 1);

		fp.Close();

		if (size) {
			delete [] buf;
		}
	}
#endif
	return 0 ;
}




static int	l_MoveToOwner (lua_State* L)
{
	DWORD id = (DWORD) luaL_checknumber (L,1);


	bool	rst = ( id == g_session.m_homunGID ) ;

#if defined (_MER)
	rst = ( rst || ( id == g_session.m_merGID ) ) ;
#endif

	if ( !rst )
		return 0;


	CGameMode* gameMode = g_modeMgr.GetGameMode ();
	sAssert (gameMode);

	CGameActor*	actor = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (id);
	if (NULL == actor)
		return 0;

	PACKET_CZ_REQUEST_MOVETOOWNER p;
	p.PacketType = HEADER_CZ_REQUEST_MOVETOOWNER;
	p.GID		 = id;
	g_connection.SendPacket (g_connection.GetPacketSize(p.PacketType), (const char *)&p);

	return 0;
}

static int	l_Move (lua_State* L)
{
	DWORD id   = (DWORD) luaL_checknumber (L,1);
	int x	   = (int) luaL_checknumber (L,2);
	int y	   = (int) luaL_checknumber (L,3);


	bool	rst = ( id == g_session.m_homunGID ) ;

#if defined (_MER)
	rst = ( rst || ( id == g_session.m_merGID ) ) ;
#endif

	if ( !rst )
		return 0;


	CGameMode* gameMode = g_modeMgr.GetGameMode ();
	sAssert (gameMode);

	CGameActor*	actor = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (id);
	if (NULL == actor)
		return 0;

	int sx, sy;
	gameMode->GetServerCoor (actor->m_pos.x,actor->m_pos.z, sx, sy);

	if ( abs(x-sx) + abs(y-sy) > 24)
		return 0;

	PACKET_CZ_REQUEST_MOVENPC p;
	p.PacketType = HEADER_CZ_REQUEST_MOVENPC;
	p.GID		 = id;
	EncodePosDir(x, y, 0, p.dest);
	g_connection.SendPacket (g_connection.GetPacketSize(p.PacketType), (const char *)&p);

	return 0;
}

static int	l_GetMsg (lua_State* L)
{
	DWORD myID		= (DWORD) luaL_checknumber (L,1);


	bool	rst = 0 ;
	int		merType = 0 ;

	if ( rst = ( myID == g_session.m_homunGID ) )
		merType = MT_HOMUN ;
#if defined (_MER)
	else if ( myID == g_session.m_merGID )
		merType = MT_MER ;
#endif

#if defined (_MER)
	rst = ( rst || ( myID == g_session.m_merGID ) ) ;
#endif

	if ( !rst )
		return 0;


	CGameMode* gameMode = g_modeMgr.GetGameMode ();
	sAssert (gameMode);
	CGameActor*	myself = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (myID);
	if (NULL == myself)
		return 0;

	MSG2AI msg = myself->GetMsg2AI (merType);

	lua_newtable(L);

	int index = 1 ;
	setfield ( L, index, msg.command ) ;

	switch ( msg.command )
	{
		case MOVE_CMD :
			setfield ( L, ++index, msg.x ) ;
			setfield ( L, ++index, msg.y ) ;
			break ;

		case ATTACK_OBJECT_CMD :
			setfield ( L, ++index, msg.targetID ) ;
			break ;

		case ATTACK_AREA_CMD :
			setfield ( L, ++index, msg.x ) ;
			setfield ( L, ++index, msg.y ) ;
			break ;

		case PATROL_CMD :
			setfield ( L, ++index, msg.x ) ;
			setfield ( L, ++index, msg.y ) ;
			break ;

		case SKILL_OBJECT_CMD :
			setfield ( L, ++index, msg.skillLevel ) ;
			setfield ( L, ++index, msg.skillID ) ;
			setfield ( L, ++index, msg.targetID ) ;
			break ;

		case SKILL_AREA_CMD :
			setfield ( L, ++index, msg.skillLevel ) ;
			setfield ( L, ++index, msg.skillID ) ;
			setfield ( L, ++index, msg.x ) ;
			setfield ( L, ++index, msg.y ) ;
			break ;
	}

	msg.command = NONE_CMD ;
	myself->SetMsg2AI (msg, merType);

	return 1;
}

static int	l_GetResMsg (lua_State* L)
{
	DWORD myID		= (DWORD) luaL_checknumber (L,1);


	bool	rst = 0 ;
	int		merType = 0 ;

	if ( rst = ( myID == g_session.m_homunGID ) )
		merType = MT_HOMUN ;
#if defined (_MER)
	else if ( myID == g_session.m_merGID )
		merType = MT_MER ;
#endif

#if defined (_MER)
	rst = ( rst || ( myID == g_session.m_merGID ) ) ;
#endif

	if ( !rst )
		return 0;


	CGameMode* gameMode = g_modeMgr.GetGameMode ();
	sAssert (gameMode);
	CGameActor*	myself = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (myID);
	if (NULL == myself)
		return 0;

	MSG2AI msg = myself->GetResMsg2AI (merType);

	lua_newtable(L);

	int index = 1 ;
	setfield ( L, index, msg.command ) ;

	switch ( msg.command )
	{
		case MOVE_CMD :
			setfield ( L, ++index, msg.x ) ;
			setfield ( L, ++index, msg.y ) ;
			break ;

		case ATTACK_OBJECT_CMD :
			setfield ( L, ++index, msg.targetID ) ;
			break ;

		case ATTACK_AREA_CMD :
			setfield ( L, ++index, msg.x ) ;
			setfield ( L, ++index, msg.y ) ;
			break ;

		case PATROL_CMD :
			setfield ( L, ++index, msg.x ) ;
			setfield ( L, ++index, msg.y ) ;
			break ;

		case SKILL_OBJECT_CMD :
			setfield ( L, ++index, msg.skillLevel ) ;
			setfield ( L, ++index, msg.skillID ) ;
			setfield ( L, ++index, msg.targetID ) ;
			break ;

		case SKILL_AREA_CMD :
			setfield ( L, ++index, msg.skillLevel ) ;
			setfield ( L, ++index, msg.skillID ) ;
			setfield ( L, ++index, msg.x ) ;
			setfield ( L, ++index, msg.y ) ;
			break ;
	}

	msg.command = NONE_CMD ;
	myself->SetResMsg2AI (msg, merType);

	return 1;
}

static int	l_SkillObject (lua_State* L)
{
	DWORD  myID	= (DWORD)	luaL_checknumber (L,1);
	DWORD  skillLevel	= (DWORD)	luaL_checknumber (L,2);
	DWORD  skillID	= (DWORD)	luaL_checknumber (L,3);
	DWORD  targetID	= (DWORD)	luaL_checknumber (L,4);


	bool	rst = ( myID == g_session.m_homunGID ) ;

#if defined (_MER)
	rst = ( rst || ( myID == g_session.m_merGID ) ) ;
#endif

	if ( !rst )
		return 0;




	rst = ( SKID_HOMUN_BEGIN < skillID && skillID < SKID_HOMUN_LAST ) ;

#if defined (_MER)
	rst = ( rst || ( SKID_MERCENARY_BEGIN < skillID && skillID < SKID_MERCENARY_LAST ) ) ;
#endif

	if ( !rst )
		return 0;


	CGameMode* gameMode = g_modeMgr.GetGameMode () ;

	CGameActor*	tActor = NULL;
	if (targetID == g_session.GetAid())
		tActor = (CGameActor*) gameMode->GetWorld()->m_player;
	else
		tActor = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (targetID) ;

	CGameActor*	myself = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (myID);

	if ( tActor && (myself->m_stateId==ST_MOVE || myself->m_stateId==ST_STAND || myself->m_stateId==ST_DAMAGE ))
		gameMode->SendMsg(MM_USE_SKILL, skillID, targetID, skillLevel) ;

	gameMode->SendMsg(MM_SETLOCKONMOUSE, 0) ;

	if (SKILL_INFO::IsGoodSkillForActor(skillID))
	{
		lua_pushnumber (L,1);
		return 1;
	}

	return 0;
}

#endif


#if defined (_HOMUN)

static	int l_GetV (lua_State* L)
{
	int		v	= (int)		luaL_checknumber (L,1);
	DWORD  id	= (DWORD)	luaL_checknumber (L,2);
	CGameMode* gameMode = g_modeMgr.GetGameMode ();
	sAssert (gameMode);
	CGameActor*	actor = NULL;

	if (id == g_session.GetAid())
		actor = (CGameActor*) gameMode->GetWorld()->m_player;
	else
		actor = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (id);

	switch (v)
	{
		case V_OWNER:
		{
			lua_pushnumber (L,(double)g_session.GetAid());
			return 1;
		}
		case V_POSITION:
		{
			if (NULL == actor) {
				lua_pushnumber (L,(double)-1);
				lua_pushnumber (L,(double)-1);
				return 2;
			}

			if (id != g_session.GetAid())


			if (id != g_session.GetAid())
			if ( IsEffectStateSpecialHiding(actor->GetEffectState()) || IsEffectStateBurrow(actor->GetEffectState()) || IsEffectStateHiding(actor->GetEffectState()))
			{
				lua_pushnumber (L,(double)-1);
				lua_pushnumber (L,(double)-1);
				return 2;
			}

			int sx, sy;
			gameMode->GetServerCoor (actor->m_pos.x,actor->m_pos.z, sx, sy);
			lua_pushnumber (L,(double)sx);
			lua_pushnumber (L,(double)sy);
			return 2;
		}
		case V_TYPE:
		{

			return 1;
		}
		case V_MOTION:
		{
			if (NULL == actor)
				lua_pushnumber (L,-1);
			else
				lua_pushnumber (L,(double)actor->m_stateId);

			return 1;
		}
		case V_ATTACKRANGE:
		{
			if (NULL == actor)
				lua_pushnumber (L,-1);
			else
			{
				if ( g_session.m_homunATKRange )
					lua_pushnumber (L,(double)g_session.m_homunATKRange);
				else
					lua_pushnumber (L,-1);
			}
			return 1;
		}
		case V_TARGET:
		{
			if (NULL == actor)
				lua_pushnumber (L,-1);
			else
				lua_pushnumber (L, (double)actor->GetTargetGid());

			return 1;
		}
		case V_SKILLATTACKRANGE:
		{
			DWORD  s_id	= (DWORD)	luaL_checknumber (L,3);
			SKILL_INFO skillInfo = g_session.GetHomunSkillItemInfoBySkillId ( s_id ) ;

			if ( skillInfo.m_isValid )
				lua_pushnumber (L,(double)skillInfo.attackRange);
			else
				lua_pushnumber (L,(double)1);
			return 1;
		}
		case V_HOMUNTYPE:
		{
			if (NULL == actor)
				lua_pushnumber (L,-1);
			else
				lua_pushnumber (L, (double)(actor->GetJob()%JT_MER_BEGIN));
			return 1;
		}
		case V_HP:
		{
			if (id == g_session.GetAid()) {
				lua_pushnumber (L, (double)g_session.GetHp());
			} else if (id == g_session.m_homunGID) {
				lua_pushnumber (L, (double)g_session.GetHomunHP());
			} else
				lua_pushnumber (L,-1);
			return 1;
		}
		case V_SP:
		{
			if (id == g_session.GetAid()) {
				lua_pushnumber (L, (double)g_session.GetSp());
			} else if (id == g_session.m_homunGID) {
				lua_pushnumber (L, (double)g_session.GetHomunSP());
			} else
				lua_pushnumber (L,-1);
			return 1;
		}
		case V_MAXHP:
		{
			if (id == g_session.GetAid()) {
				lua_pushnumber (L, (double)g_session.GetMaxhp());
			} else if (id == g_session.m_homunGID) {
				lua_pushnumber (L, (double)g_session.GetHomunMaxHP());
			} else
				lua_pushnumber (L,-1);
			return 1;
		}
		case V_MAXSP:
		{
			if (id == g_session.GetAid()) {
				lua_pushnumber (L, (double)g_session.GetMaxsp());
			} else if (id == g_session.m_homunGID) {
				lua_pushnumber (L, (double)g_session.GetHomunMaxSP());
			} else
				lua_pushnumber (L,-1);
			return 1;
		}

		default:
			sAssert(0);
	}
	return 0;
}

static int	l_Attack (lua_State* L)
{
	DWORD myID		= (DWORD) luaL_checknumber (L,1);
	DWORD enemyID	= (DWORD) luaL_checknumber (L,2);

	if ( myID != g_session.m_homunGID )
		return 0 ;

#if defined (_MER)
	if ( g_session.m_merGID && enemyID == g_session.m_merGID )
		return 0 ;
#endif

	CGameMode* gameMode = g_modeMgr.GetGameMode ();
	sAssert (gameMode);

	CGameActor*	myself = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (myID);
	CGameActor* enemy  = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (enemyID);

	if (NULL == myself || NULL == enemy)
		return 0;


	if ( IsEffectStateSpecialHiding(enemy->GetEffectState()) || IsEffectStateBurrow(enemy->GetEffectState()) || IsEffectStateHiding(enemy->GetEffectState()))
		return 0 ;

	static	int homunAttackLastTime = 0 ;

	if ( ( homunAttackLastTime + g_session.m_homunAspd ) < timeGetTime( ) )
	{
		PACKET_CZ_REQUEST_ACTNPC p;
		p.PacketType = HEADER_CZ_REQUEST_ACTNPC;
		p.GID		 = myID;
		p.targetGID  = enemyID;
		p.action	 = ACTION_ATTACK;
		g_connection.SendPacket (g_connection.GetPacketSize(p.PacketType), (const char *)&p);

		homunAttackLastTime = timeGetTime( ) ;
	}

	return 0;
}

#endif


#if defined (_MER)

static	int l_GetV_M (lua_State* L)
{
	int		v	= (int)		luaL_checknumber (L,1);
	DWORD  id	= (DWORD)	luaL_checknumber (L,2);

	CGameMode* gameMode = g_modeMgr.GetGameMode ();
	sAssert (gameMode);
	CGameActor*	actor = NULL;

	if (id == g_session.GetAid())
		actor = (CGameActor*) gameMode->GetWorld()->m_player;
	else
		actor = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (id);

	switch (v)
	{
		case V_OWNER:
		{
			lua_pushnumber (L,(double)g_session.GetAid());
			return 1;
		}
		case V_POSITION:
		{
			if (NULL == actor) {
				lua_pushnumber (L,(double)-1);
				lua_pushnumber (L,(double)-1);
				return 2;
			}


			if (id != g_session.GetAid())
			if ( IsEffectStateSpecialHiding(actor->GetEffectState()) || IsEffectStateBurrow(actor->GetEffectState()) || IsEffectStateHiding(actor->GetEffectState()))
			{
				lua_pushnumber (L,(double)-1);
				lua_pushnumber (L,(double)-1);
				return 2;
			}

			int sx, sy;
			gameMode->GetServerCoor (actor->m_pos.x,actor->m_pos.z, sx, sy);
			lua_pushnumber (L,(double)sx);
			lua_pushnumber (L,(double)sy);
			return 2;
		}
		case V_TYPE:
		{

			return 1;
		}
		case V_MOTION:
		{
			if (NULL == actor)
				lua_pushnumber (L,-1);
			else
				lua_pushnumber (L,(double)actor->m_stateId);
			return 1;
		}
		case V_ATTACKRANGE:
		{
			if (NULL == actor)
				lua_pushnumber (L,-1);
			else
			{
				if ( g_session.m_merATKRange )
					lua_pushnumber (L,(double)g_session.m_merATKRange);
				else
					lua_pushnumber (L,-1);
			}
			return 1;
		}
		case V_TARGET:
		{
			if (NULL == actor || actor->m_stateId == ST_DEAD )
				lua_pushnumber (L,-1);
			else
				lua_pushnumber (L, (double)actor->GetTargetGid());
			return 1;
		}
		case V_SKILLATTACKRANGE:
		{
			DWORD  s_id	= (DWORD)	luaL_checknumber (L,3);
			SKILL_INFO skillInfo = g_session.GetMerSkillItemInfoBySkillId ( s_id ) ;

			if ( skillInfo.m_isValid )
				lua_pushnumber (L,(double)skillInfo.attackRange);
			else
				lua_pushnumber (L,(double)1);
			return 1;
		}
		case V_MERTYPE:
		{
			if (NULL == actor)
				lua_pushnumber (L,-1);
			else
			{

				if ( g_session.IsMer (actor->GetJob()) )
					lua_pushnumber (L, (double)(actor->GetJob()%JT_MER_ARCHER01)+1);
				else
					lua_pushnumber (L, (double)1);
			}

			return 1;
		}
		case V_HP:
		{
			if (id == g_session.GetAid()) {
				lua_pushnumber (L, (double)g_session.GetHp());
			} else if (id == g_session.m_merGID) {
				lua_pushnumber (L, (double)g_session.GetMerHP());
			} else
				lua_pushnumber (L,-1);
			return 1;
		}
		case V_SP:
		{
			if (id == g_session.GetAid()) {
				lua_pushnumber (L, (double)g_session.GetSp());
			} else if (id == g_session.m_merGID) {
				lua_pushnumber (L, (double)g_session.GetMerSP());
			} else
				lua_pushnumber (L,-1);
			return 1;
		}
		case V_MAXHP:
		{
			if (id == g_session.GetAid()) {
				lua_pushnumber (L, (double)g_session.GetMaxhp());
			} else if (id == g_session.m_merGID) {
				lua_pushnumber (L, (double)g_session.GetMerMaxHP());
			} else
				lua_pushnumber (L,-1);
			return 1;
		}
		case V_MAXSP:
		{
			if (id == g_session.GetAid()) {
				lua_pushnumber (L, (double)g_session.GetMaxsp());
			} else if (id == g_session.m_merGID) {
				lua_pushnumber (L, (double)g_session.GetMerMaxSP());
			} else
				lua_pushnumber (L,-1);
			return 1;
		}

		default:
			sAssert(0);
	}
	return 0;
}

static int	l_Attack_M (lua_State* L)
{
	DWORD myID		= (DWORD) luaL_checknumber (L,1);
	DWORD enemyID	= (DWORD) luaL_checknumber (L,2);

	if ( myID != g_session.m_merGID )
		return 0 ;

#if defined (_HOMUN)
	if ( g_session.m_homunGID && enemyID == g_session.m_homunGID )
		return 0 ;
#endif

	CGameMode* gameMode = g_modeMgr.GetGameMode ();
	sAssert (gameMode);

	CGameActor*	myself = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (myID);
	CGameActor* enemy  = (CGameActor*) gameMode->GetWorld()->GetGameActorByAID (enemyID);

	if (NULL == myself || NULL == enemy)
		return 0;

	if ( IsEffectStateSpecialHiding(enemy->GetEffectState())
		|| IsEffectStateBurrow(enemy->GetEffectState())
		|| IsEffectStateHiding(enemy->GetEffectState()))
	{
		return 0 ;
	}

	static	int merAttackLastTime = 0 ;

	if ( ( merAttackLastTime + g_session.m_merAspd ) < timeGetTime( ) )
	{
		PACKET_CZ_REQUEST_ACTNPC p;
		p.PacketType = HEADER_CZ_REQUEST_ACTNPC;
		p.GID		 = myID;
		p.targetGID  = enemyID;
		p.action	 = ACTION_ATTACK;
		g_connection.SendPacket (g_connection.GetPacketSize(p.PacketType), (const char *)&p);

		merAttackLastTime = timeGetTime( ) ;
	}

	return 0;
}

#endif

#endif
