#include "common.h"


#include "GuildInfo.h"
#include "base/Util.h"


#include "3dlib/DrawUtil.h"
#include "window/UISys.h"

extern UIWindowMgr g_windowMgr;

GuildInfo g_guildInfo;

GuildInfo :: GuildInfo()
{
	Init();
}

GuildInfo :: ~GuildInfo()
{
}

void	GuildInfo :: Init2()
{
	int a = m_gdid;
	int b = m_emblemVersion;
	int c = m_rightJoin;
	int d = m_rightPunish;
	int e = m_amIMaster;

	Init();

	m_gdid = a;
	m_emblemVersion = b;
	m_rightJoin = c;
	m_rightPunish = d;
	m_amIMaster = e;
}

void	GuildInfo :: Init()
{
	m_gdid = 0;
	m_emblemVersion = 0;
	m_rightJoin = 0;
	m_rightPunish = 0;
	m_amIMaster = 0;

	m_guildTabBitmask = -1;

	for (int r=0; r<5; ++r)
	{
		m_isRequestInfo[r] = FALSE;
	}

	m_guildName = "";
	m_masterName = "";
	m_subjectName = "";
	m_noticeName = "";
	m_guildLevel = 0;
	m_numMember = 0;
	m_numMaxMember = 0;
	m_memberAvgLevel = 0;
	m_manageLand = "";
	m_exp = 0;
	m_nextExp = 0;
	m_emblem = 0;
	m_point = 0;
	m_honor = 0;
	m_virtue = 0;
	m_zeny = 0;


	m_posInfoList.clear();
	m_memberInfoList.clear();
	m_relatedGuildInfoList.clear();
	m_banishList.clear();
	m_guildList.clear();
	m_positionIdNameMap.clear();
	m_memberPositionInfoListForChange.clear();
	m_changedPositionInfoList.clear();
}

void	GuildInfo :: RecalculateGuildExp()
{
	double long totalMemberExp = 0;
	std::list<GUILD_MEMBER_INFORMATION>::iterator it = m_memberInfoList.begin();
	while (it != m_memberInfoList.end()) {
		totalMemberExp += it->MemberExp;
		++it;
	}

	it = m_memberInfoList.begin();
	while (it != m_memberInfoList.end()) {

		if (totalMemberExp) {
			it->Service = float(it->MemberExp) * 100.0f / float(totalMemberExp);
		} else {
			it->Service = 0;
		}
		it->GPositionName = m_positionIdNameMap[it->GPositionID];
		++it;
	}
}

void	GuildInfo :: AddMemberInfo(GUILD_MEMBER_INFORMATION * guildMemberInfo)
{
	m_memberInfoList.push_back(*guildMemberInfo);
}

void	GuildInfo :: ClearMemberInfo()
{
	m_memberInfoList.clear();
}

void	GuildInfo :: SetMemberInfoState(DWORD AID, int status)
{
	std::list<GUILD_MEMBER_INFORMATION>::iterator it = m_memberInfoList.begin();
	while (it != m_memberInfoList.end()) {
		if (AID == it->AID) {
			it->CurrentState = status;
			g_windowMgr.InvalidateGuildWnd();
			break;
		}
		++it;
	}
}

void	GuildInfo ::	SetMemberInfoState2(DWORD AID, int status, DWORD GID, short sex, short head, short headPalette)
{
	std::list<GUILD_MEMBER_INFORMATION>::iterator it = m_memberInfoList.begin();
	while (it != m_memberInfoList.end()) {
		if (GID == it->GID)
		{
			it->Sex = sex;
			it->HeadType = head;
			it->HeadPalette = headPalette;

			it->CurrentState = status;
			g_windowMgr.InvalidateGuildWnd();
			break;
		}
		++it;
	}
}

void	GuildInfo :: ChangeMemberPosition(int AID, int GID, int positionID)
{
	std::list<GUILD_MEMBER_INFORMATION>::iterator it = m_memberInfoList.begin();
	while (it != m_memberInfoList.end()) {
		if (it->AID == AID && it->GID == GID) {
			it->GPositionID = positionID;
			it->GPositionName = m_positionIdNameMap[it->GPositionID];
			g_windowMgr.InvalidateGuildWnd();
			break;
		}
		++it;
	}
}

BOOL	GuildInfo :: RemoveMemberInfo(const char *charName)
{
	std::list<GUILD_MEMBER_INFORMATION>::iterator it = m_memberInfoList.begin();
	while (it != m_memberInfoList.end()) {
		if (it->CharName == charName) {
			m_memberInfoList.erase(it);
			g_windowMgr.InvalidateGuildWnd();
			return TRUE;
		}
		++it;
	}
	return FALSE;
}

void	GuildInfo :: AddToBanishList(SGUILD_BANISH_INFO * banishInfo)
{
	m_banishList.push_back(*banishInfo);
	g_windowMgr.InvalidateGuildWnd();
}

void	GuildInfo :: UpdatePosInfo(GUILD_POSITION_INFORMATION newInfo)
{
	std::list<GUILD_POSITION_INFORMATION>::iterator it = m_posInfoList.begin();
	while (it != m_posInfoList.end()) {
		if (it->positionID == newInfo.positionID) {
			*(it) = newInfo;
			break;
		}
		++it;
	}
}

void	GuildInfo :: DeleteRelatedGuild(int gdid, int relation)
{
	std::list<RELATED_GUILD_INFORMATION>::iterator it = m_relatedGuildInfoList.begin();
	while (it != m_relatedGuildInfoList.end()) {
		if (it->GDID == gdid && it->relation == relation) {
			m_relatedGuildInfoList.erase(it);
			g_windowMgr.InvalidateGuildWnd();
			return;
		}
		++it;
	}
}

int		GuildInfo :: GetNumRelatedGuild(int relation)
{
	int count = 0;
	std::list<RELATED_GUILD_INFORMATION>::iterator it = m_relatedGuildInfoList.begin();
	while (it != m_relatedGuildInfoList.end()) {
		if (it->relation == relation) {
			++count;
		}
		++it;
	}
	return count;
}

GUILD_MEMBER_INFORMATION	GuildInfo :: GetMemberInfoByAID(DWORD AID) const
{
	std::list<GUILD_MEMBER_INFORMATION>::const_iterator it = m_memberInfoList.begin();
	while (it != m_memberInfoList.end()) {
		if (AID == it->AID)
			return *it;
		++it;
	}
	GUILD_MEMBER_INFORMATION ret;
	ZeroMemory(&ret, sizeof(ret));
	return ret;
}

int GuildInfo :: GetAIDbyName(const char* name) const
{
	std::list<GUILD_MEMBER_INFORMATION>::const_iterator it = m_memberInfoList.begin();
	while (it != m_memberInfoList.end()) {
		if (it->CharName == name)
			return it->AID;
		char tszName[100];
		strcpy(tszName, it->CharName.c_str());
		++it;
	}
	return -1;
}
