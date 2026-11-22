#ifndef HIGHPRIEST_SKILLINFO_H
#define HIGHPRIEST_SKILLINFO_H

#include "ItemInfo.h"

struct SKILL_DESC;
struct SKILL_INFO;

struct SKILL_DESC {
public:
	SKILL_DESC() : skillIdName(NULL), displayName(NULL) {}
	const char* skillIdName;
	const char* displayName;
	std::vector<char const *> descriptionName;
	std::vector<int> spUsage;
};

struct SKILL_INFO {
public:
	SKILL_INFO(int dummy);
	SKILL_INFO() : m_isValid(1), skillName(NULL) {}
	const char* GetName();
	std::vector<char const *>& GetDescription();
	const char* GetSkillIdName();
	const std::vector<int>& LevelUseSpAmountList();
protected:
	static CommonObject s_obj;
	static int __cdecl GetIdBySkillIdName(const char* skillIdName, int findStart);
	static void __cdecl InitTable();
	static void __cdecl InitGuildTable();
	static void __cdecl InitHomunTable();
	static void __cdecl InitMerTable();
public:
	static void __cdecl InitSkillInfoTables();
	static bool __cdecl IsGoodSkillForActor(int skillId);
	static bool __cdecl IsSkillNotDelayed(int skillId);
	static const char* __cdecl GetSkillIdNameBySkillId(int skillId);
	static const char* __cdecl GetSkillNameBySkillId(int skillId);
	static int __cdecl IsLevelUseSkill(int skillId);
	static std::vector<char const *> s_skillIdNameList;
	static std::vector<char const *> s_guildSkillIdNameList;
	static std::vector<char const *> s_homunSkillIdNameList;
	static std::vector<char const *> s_merSkillIdNameList;
	static std::vector<int> s_isLevelUseSkillList;
	static std::map<int, SKILL_DESC> s_skillDescMap;
	int m_isValid;
	int SKID;
	int type;
	int level;
	int spcost;
	int upgradable;
	int attackRange;
	const char* skillName;
	int skillPos;
	int skillMaxLv;
	std::vector<int> needSkillList;
};

struct QUEST_INFO {
public:
	DWORD questID;
	unsigned char show;

	short m_mobTime_Year;
	short m_mobTime_Month;
	short m_mobTime_Day;
	short m_mobTime_Hour;
	short m_mobTime_Minute;

	short m_Server_Hour;
	short m_Server_Minute;

	char mobCount;
	DWORD mobID[3];
	short mobHunt[3];
	char mobName[3][51];
};

#endif
