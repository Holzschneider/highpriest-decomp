#ifndef HIGHPRIEST_GUILDINFO_H
#define HIGHPRIEST_GUILDINFO_H


struct SGUILD_BANISH_INFO;
class GuildInfo;
struct RELATED_GUILD_INFORMATION;



struct GUILD_MEMBER_INFORMATION {
public:
	unsigned int AID;
	unsigned int GID;
	std::string CharName;
	int HeadType;
	int HeadPalette;
	int Job;
	int Sex;
	int GPositionID;
	std::string GPositionName;
	int Level;
	std::string Memo;
	float Service;
	int MemberExp;
	int CurrentState;
};

struct MEMBER_POSITION_INFORMATION {
	int AID;
	int GID;
	int positionID;
};

struct GUILD_POSITION_INFORMATION {
	int ranking;
	int positionID;
	std::string posName;
	int rightOfJoin;
	int rightOfPunish;
	int payRate;
};

struct SGUILD_BANISH_INFO {
public:
	std::string MemberName;
	std::string MemberAccount;
	std::string Reason;
};

struct SGUILD_INFO {
public:
	int GDID;
	std::string GuildName;
	int Level;
	int UserNum;
	int MaxUserNum;
	int Ranking;
};

struct RELATED_GUILD_INFORMATION {
	int GDID;
	int relation;
	std::string GuildName;
};

class GuildInfo {
public:
	GuildInfo();
	~GuildInfo();
	void Init();
	void Init2();
	void RecalculateGuildExp();
	void AddMemberInfo(GUILD_MEMBER_INFORMATION* guildMemberInfo);
	void ClearMemberInfo();
	void SetMemberInfoState(unsigned int AID, int status);
	void SetMemberInfoState2(unsigned int AID, int status, unsigned int GID, short sex, short head, short headPalette);
	void ChangeMemberPosition(int AID, int GID, int positionID);
	int RemoveMemberInfo(const char* charName);
	std::list<GUILD_MEMBER_INFORMATION>& GetMemberInfoList() { return m_memberInfoList; }
	GUILD_MEMBER_INFORMATION GetMemberInfoByAID(unsigned int __$ReturnUdt) const;
	int GetAIDbyName(const char* name) const;
	void AddToBanishList(SGUILD_BANISH_INFO* banishInfo);
	void UpdatePosInfo(GUILD_POSITION_INFORMATION newInfo);
	std::list<GUILD_POSITION_INFORMATION>& GetPosInfoList() { return m_posInfoList; }
	void DeleteRelatedGuild(int gdid, int relation);
	int GetNumRelatedGuild(int relation);
	std::string m_guildName;
	std::string m_masterName;
	std::string m_subjectName;
	std::string m_noticeName;
	int m_guildLevel;
	int m_numMember;
	int m_numOnMember;
	int m_numMaxMember;
	int m_memberAvgLevel;
	std::string m_manageLand;
	int m_exp;
	int m_nextExp;
	int m_emblem;
	int m_point;
	int m_honor;
	int m_virtue;
	unsigned int m_zeny;
	int m_guildTabBitmask;
	int m_gdid;
	int m_emblemVersion;
	int m_rightJoin;
	int m_rightPunish;
	int m_amIMaster;
	int m_isRequestInfo[5];
protected:
	std::list<GUILD_POSITION_INFORMATION> m_posInfoList;
	std::list<GUILD_MEMBER_INFORMATION> m_memberInfoList;
public:
	std::list<RELATED_GUILD_INFORMATION> m_relatedGuildInfoList;
	std::list<SGUILD_BANISH_INFO> m_banishList;
	std::list<SGUILD_INFO> m_guildList;
	std::map<int, std::string> m_positionIdNameMap;
	std::map<int, MEMBER_POSITION_INFORMATION> m_memberPositionInfoListForChange;
	std::map<int, GUILD_POSITION_INFORMATION> m_changedPositionInfoList;
};
extern GuildInfo g_guildInfo;

#endif
