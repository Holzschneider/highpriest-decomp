#include "common.h"
#include "Session.h"

typedef std::map<int, SKILL_DESC> skilldescmap;

skilldescmap SKILL_INFO::s_skillDescMap;
PCharList SKILL_INFO::s_skillIdNameList;
PCharList SKILL_INFO::s_guildSkillIdNameList;
PCharList SKILL_INFO::s_homunSkillIdNameList;
PCharList SKILL_INFO::s_merSkillIdNameList;
IntegerList SKILL_INFO::s_isLevelUseSkillList;
CommonObject SKILL_INFO::s_obj;


const int SKID_GD_START = 10000;
const int SKID_LAST_HP = 1039;
const int SKID_ALL_DREAM_SUMMERNIGHT = 695;
static const char * unknownSkill = "Unknown-Skill";

extern CSession g_session;

using std::pair;


template<class T>
typename T::referent_type& GetMapObject(typename T::key_type itemName, T& table)
{
	T::iterator iter = table.find(itemName);

	if(iter != table.end()) return iter->second;

	pair<T::iterator, bool>& result =
		table.insert(T::value_type(itemName, T::referent_type()));

	sAssert(result.second == true);

	return result.first->second;
}

SKILL_INFO::SKILL_INFO(int dummy) : m_isValid(0), skillName(NULL)
{
	skillName = unknownSkill;
}

const char * SKILL_INFO::GetSkillIdName()
{
	return GetSkillIdNameBySkillId(SKID);
}

const char * SKILL_INFO::GetName()
{
	return GetSkillNameBySkillId(SKID);
}


std::vector<char const *>& SKILL_INFO::GetDescription()
{
	SKILL_DESC& skilldesc = GetDesc(SKID, s_skillDescMap);
	return skilldesc.descriptionName;
}

void SKILL_INFO::InitSkillInfoTables()
{
	s_isLevelUseSkillList.clear();
	s_skillDescMap.clear();
	s_obj.Clear();

	if (s_skillIdNameList.size() != SKID_LAST_HP)
		InitTable();
	if (s_guildSkillIdNameList.size() != SKID_GD_LAST - SKID_GD_START)
		InitGuildTable();
	if (s_homunSkillIdNameList.size() != SKID_HOMUN_LAST - SKID_HOMUN_BEGIN)
		InitHomunTable();
	if (s_merSkillIdNameList.size() != SKID_MERCENARY_LAST - SKID_MERCENARY_BEGIN)
		InitMerTable();

	PCharList table;
	PCharList::iterator tableiter;

	const int ReserveSize = 2048;
	table.reserve(ReserveSize);

	int count = 1;
	s_obj.TokenFileToPCharList(table, "skillnametable.txt", 0);
	for (tableiter = table.begin(); tableiter != table.end(); ++tableiter) {
		count = GetIdBySkillIdName(*tableiter, count);
		SKILL_DESC& skilldesc = GetMapObject(count, s_skillDescMap);
		skilldesc.skillIdName = *tableiter;
		++tableiter; sAssert(tableiter != table.end());
		g_session.RemoveUnderbar(*tableiter, (char *)*tableiter);
		skilldesc.displayName = *tableiter;
	}

	count = 1;
	int skillID = 1;
	s_obj.TokenFileToPCharList(table, "skilldesctable.txt", TRUE);
	for (tableiter = table.begin(); tableiter != table.end(); ++tableiter) {
		count = GetIdBySkillIdName(*tableiter, skillID);
		if (count) {
			skillID = count;
		} else {
			const char * str = *tableiter;
			GetDesc(skillID, s_skillDescMap).descriptionName.push_back(*tableiter);
		}
	}

	count = 1;
	skillID = 1;
	s_obj.TokenFileToPCharList(table, "leveluseskillspamount.txt", 0);
	for (tableiter = table.begin(); tableiter != table.end(); ++tableiter) {
		count = GetIdBySkillIdName(*tableiter, skillID);
		if (count) {
			skillID = count;
		} else {
			if (strcmp(*tableiter, "@") == 0) continue;
			GetDesc(skillID, s_skillDescMap).spUsage.push_back(atoi(*tableiter));
		}
	}

	SKILL_DESC& unknownskilldesc = GetDesc(0, s_skillDescMap);
	unknownskilldesc.descriptionName.push_back(unknownSkill);
	unknownskilldesc.displayName = unknownSkill;
	unknownskilldesc.spUsage.push_back(0);

	s_isLevelUseSkillList.reserve(SKID_MERCENARY_LAST);

	SKILL_DESC& unknownskilldesc2 = GetDesc(-1, s_skillDescMap);
	unknownskilldesc2 = unknownskilldesc;

	for (int skillId = 0; skillId < SKID_MERCENARY_LAST; ++skillId)
	{
		BOOL isExist;
		SKILL_DESC& skilldesc = GetDesc(skillId, s_skillDescMap);
		if (skilldesc.spUsage.empty())
			isExist = false;
		else
			isExist = true;
		s_isLevelUseSkillList.push_back(isExist);
	}

}


bool SKILL_INFO::IsGoodSkillForActor(int skillId)
{
	switch (skillId) {
		case SKID_SM_RECOVERY:
		case SKID_AL_HEAL:
		case SKID_ALL_ANGEL_PROTECT:
		case SKID_AL_INCAGI:
		case SKID_CASH_INCAGI:
		case SKID_AL_CURE:
		case SKID_AL_BLESSING:
		case SKID_CASH_BLESSING:
		case SKID_ALL_RESURRECTION:
		case SKID_TF_DETOXIFY:
		case SKID_PR_IMPOSITIO:
		case SKID_PR_ASPERSIO:
		case SKID_PR_STRECOVERY:
		case SKID_PR_SLOWPOISON:
		case SKID_PR_SUFFRAGIUM:
		case SKID_AS_POISONREACT:
		case SKID_AS_ENCHANTPOISON:
		case SKID_PR_KYRIE:
		case SKID_CR_DEVOTION:
		case SKID_CR_PROVIDENCE:
		case SKID_SA_FLAMELAUNCHER:
		case SKID_SA_FROSTWEAPON:
		case SKID_SA_LIGHTNINGLOADER:
		case SKID_SA_SEISMICWEAPON:
		case SKID_AM_POTIONPITCHER:
		case SKID_AM_CP_WEAPON:
		case SKID_AM_CP_SHIELD:
		case SKID_AM_CP_ARMOR:
		case SKID_AM_CP_HELM:
		case SKID_MO_ABSORBSPIRITS:
		case SKID_MO_KITRANSLATION:
		case SKID_HP_ASSUMPTIO:
		case SKID_CASH_ASSUMPTIO:
		case SKID_BS_REPAIRWEAPON:
		case SKID_CR_FULLPROTECTION:
		case SKID_SL_ALCHEMIST:
		case SKID_SL_MONK:
		case SKID_AM_BERSERKPITCHER:
		case SKID_SL_STAR:
		case SKID_SL_SAGE:
		case SKID_SL_CRUSADER:
		case SKID_SL_SUPERNOVICE:
		case SKID_SL_KNIGHT:
		case SKID_SL_WIZARD:
		case SKID_SL_PRIEST:
		case SKID_SL_BARDDANCER:
		case SKID_SL_ROGUE:
		case SKID_SL_ASSASIN:
		case SKID_SL_BLACKSMITH:
		case SKID_SL_HUNTER:
		case SKID_SL_SOULLINKER:
		case SKID_SL_HIGH:
		case SKID_SL_DEATHKNIGHT:
		case SKID_SL_COLLECTOR:
		case SKID_SL_NINJA:
		case SKID_SL_GUNNER:
		case SKID_SL_KAIZEL:
		case SKID_SL_KAAHI:
		case SKID_SL_KAUPE:
		case SKID_SL_KAITE:
		case SKID_MB_B_EXCLUDE:
		case SKID_DE_COIL:
		case SKID_DA_FIRSTSLOT:
		case SKID_DA_HEADDEF:
		case SKID_DA_REWARD:
		case SKID_DA_EARPLUG:
		case SKID_DA_CONTRACT:
		case SKID_DA_RESET:
		case SKID_DA_COPY:
		case SKID_DA_EXP:
		case SKID_DA_REBUILD:
		case SKID_DA_JOBCHANGE:
		case SKID_DA_CRYSTAL:
		case SKID_GM_SANDMAN:
		case SKID_MER_REGAIN:
		case SKID_MER_TENDER:
		case SKID_MER_BENEDICTION:
		case SKID_MER_RECUPERATE:
		case SKID_MER_MENTALCURE:
		case SKID_MER_COMPRESS:
			return true;
		default:
			return false;
	}
}

bool SKILL_INFO::IsSkillNotDelayed(int skillId)
{
	switch (skillId) {
		case	SKID_MG_NAPALMBEAT:
		case	SKID_MG_SAFETYWALL:
		case	SKID_MG_SOULSTRIKE:
		case	SKID_NPC_DARKSTRIKE:
		case	SKID_MG_COLDBOLT:
		case	SKID_MG_FROSTDIVER:
		case	SKID_MG_STONECURSE:
		case	SKID_MG_FIREBALL:
		case	SKID_MG_FIREWALL:
		case	SKID_MG_FIREBOLT:
		case	SKID_MG_LIGHTNINGBOLT:
		case	SKID_MG_THUNDERSTORM:
		case	SKID_AL_WARP:
		case	SKID_AL_INCAGI:
		case	SKID_CASH_INCAGI:
		case	SKID_AL_DECAGI:
		case	SKID_AL_CRUCIS:
		case	SKID_AL_ANGELUS:
		case	SKID_WZ_WATERBALL:
		case	SKID_PR_TURNUNDEAD:
			return	false;
		default:
			return	true;
	}
}

const char * SKILL_INFO::GetSkillIdNameBySkillId(int skillId)
{
	const int SKID_GD_START = 10000;
	if (skillId >= SKID_NV_BASIC && skillId < 1039)
		return s_skillIdNameList[skillId];
	else if (skillId >= SKID_GD_START && skillId < SKID_GD_LAST)
		return s_guildSkillIdNameList[skillId-SKID_GD_START];
	else if (skillId >= SKID_HOMUN_BEGIN && skillId < SKID_HOMUN_LAST)
		return s_homunSkillIdNameList[skillId-SKID_HOMUN_BEGIN];
	else if (skillId >= SKID_MERCENARY_BEGIN && skillId < SKID_MERCENARY_LAST)
		return s_merSkillIdNameList[skillId-SKID_MERCENARY_BEGIN];

	return s_skillIdNameList[0];
}

const char * SKILL_INFO::GetSkillNameBySkillId(int skillId)
{
	SKILL_DESC& skilldesc = GetDesc(skillId, s_skillDescMap);
	return skilldesc.displayName;
}

int SKILL_INFO::GetIdBySkillIdName(const char* skillIdName, int findStart)
{
	int i;
	if (findStart >= SKID_NV_BASIC - 1 && findStart < SKID_LAST_HP)
	{
		if (strcmp(skillIdName, s_skillIdNameList[findStart]) == 0)
			return findStart;
	}

	for (i = 0; i < SKID_GD_LAST - SKID_GD_START; i++)
	{
		if (strcmp(skillIdName, s_guildSkillIdNameList[i]) == 0)
			return i + SKID_GD_START;
	}

	for (i = 0; i < SKID_HOMUN_LAST - SKID_HOMUN_BEGIN; i++)
	{
		if (strcmp(skillIdName, s_homunSkillIdNameList[i]) == 0)
			return i + SKID_HOMUN_BEGIN;
	}

	for (i = 0; i < SKID_MERCENARY_LAST - SKID_MERCENARY_BEGIN; i++)
	{
		if (strcmp(skillIdName, s_merSkillIdNameList[i]) == 0)
			return i + SKID_MERCENARY_BEGIN;
	}

	for (i = 0; i < SKID_LAST_HP; i++)
	{
		if (strcmp(skillIdName, s_skillIdNameList[i]) == 0)
			return i;
	}

	return 0;
}

const IntegerList & SKILL_INFO::LevelUseSpAmountList()
{
	SKILL_DESC& skilldesc = GetDesc(SKID, s_skillDescMap);
	static IntegerList tmpIntegerList(2);
	if (skilldesc.spUsage.empty())
		return tmpIntegerList;
	else
		return skilldesc.spUsage;
}

int SKILL_INFO::IsLevelUseSkill(int skillId)
{
	return s_isLevelUseSkillList[skillId];
}

void SKILL_INFO::InitGuildTable()
{
	s_guildSkillIdNameList.resize(SKID_GD_LAST - SKID_GD_START, "SKILL-UNKNOWN");
	s_guildSkillIdNameList[SKID_GD_APPROVAL - SKID_GD_START] = "GD_APPROVAL";
	s_guildSkillIdNameList[SKID_GD_KAFRACONTRACT - SKID_GD_START] = "GD_KAFRACONTRACT";
	s_guildSkillIdNameList[SKID_GD_GUARDRESEARCH - SKID_GD_START] = "GD_GUARDRESEARCH";
	s_guildSkillIdNameList[SKID_GD_GUARDUP - SKID_GD_START] = "GD_GUARDUP";
	s_guildSkillIdNameList[SKID_GD_EXTENSION - SKID_GD_START] = "GD_EXTENSION";

	s_guildSkillIdNameList[SKID_GD_GLORYGUILD - SKID_GD_START] = "GD_GLORYGUILD";
	s_guildSkillIdNameList[SKID_GD_LEADERSHIP - SKID_GD_START] = "GD_LEADERSHIP";
	s_guildSkillIdNameList[SKID_GD_GLORYWOUNDS - SKID_GD_START] = "GD_GLORYWOUNDS";
	s_guildSkillIdNameList[SKID_GD_SOULCOLD - SKID_GD_START] = "GD_SOULCOLD";
	s_guildSkillIdNameList[SKID_GD_HAWKEYES - SKID_GD_START] = "GD_HAWKEYES";

	s_guildSkillIdNameList[SKID_GD_BATTLEORDER - SKID_GD_START] = "GD_BATTLEORDER";
	s_guildSkillIdNameList[SKID_GD_REGENERATION - SKID_GD_START] = "GD_REGENERATION";
	s_guildSkillIdNameList[SKID_GD_RESTORE - SKID_GD_START] = "GD_RESTORE";
	s_guildSkillIdNameList[SKID_GD_EMERGENCYCALL - SKID_GD_START] = "GD_EMERGENCYCALL";
	s_guildSkillIdNameList[SKID_GD_DEVELOPMENT - SKID_GD_START] = "GD_DEVELOPMENT";
	s_guildSkillIdNameList[SKID_GD_ITEMEMERGENCYCALL - SKID_GD_START] = "GD_ITEMEMERGENCYCALL";
}

void SKILL_INFO::InitHomunTable()
{
	s_homunSkillIdNameList.resize(SKID_HOMUN_LAST - SKID_HOMUN_BEGIN, "SKILL-UNKNOWN");
	s_homunSkillIdNameList[SKID_HLIF_HEAL - SKID_HOMUN_BEGIN] = "HLIF_HEAL";
	s_homunSkillIdNameList[SKID_HLIF_AVOID - SKID_HOMUN_BEGIN] = "HLIF_AVOID";
	s_homunSkillIdNameList[SKID_HLIF_BRAIN - SKID_HOMUN_BEGIN] = "HLIF_BRAIN";
	s_homunSkillIdNameList[SKID_HLIF_CHANGE - SKID_HOMUN_BEGIN] = "HLIF_CHANGE";

	s_homunSkillIdNameList[SKID_HAMI_CASTLE - SKID_HOMUN_BEGIN] = "HAMI_CASTLE";
	s_homunSkillIdNameList[SKID_HAMI_DEFENCE - SKID_HOMUN_BEGIN] = "HAMI_DEFENCE";
	s_homunSkillIdNameList[SKID_HAMI_SKIN - SKID_HOMUN_BEGIN] = "HAMI_SKIN";
	s_homunSkillIdNameList[SKID_HAMI_BLOODLUST - SKID_HOMUN_BEGIN] = "HAMI_BLOODLUST";

	s_homunSkillIdNameList[SKID_HFLI_MOON - SKID_HOMUN_BEGIN] = "HFLI_MOON";
	s_homunSkillIdNameList[SKID_HFLI_FLEET - SKID_HOMUN_BEGIN] = "HFLI_FLEET";
	s_homunSkillIdNameList[SKID_HFLI_SPEED - SKID_HOMUN_BEGIN] = "HFLI_SPEED";
	s_homunSkillIdNameList[SKID_HFLI_SBR44 - SKID_HOMUN_BEGIN] = "HFLI_SBR44";

	s_homunSkillIdNameList[SKID_HVAN_CAPRICE - SKID_HOMUN_BEGIN] = "HVAN_CAPRICE";
	s_homunSkillIdNameList[SKID_HVAN_CHAOTIC - SKID_HOMUN_BEGIN] = "HVAN_CHAOTIC";
	s_homunSkillIdNameList[SKID_HVAN_INSTRUCT - SKID_HOMUN_BEGIN] = "HVAN_INSTRUCT";
	s_homunSkillIdNameList[SKID_HVAN_EXPLOSION - SKID_HOMUN_BEGIN] = "HVAN_EXPLOSION";
}

void SKILL_INFO::InitMerTable()
{
	s_merSkillIdNameList.resize(SKID_MERCENARY_LAST - SKID_MERCENARY_BEGIN, "SKILL-UNKNOWN");
	s_merSkillIdNameList[SKID_MS_BASH - SKID_MERCENARY_BEGIN] = "MS_BASH";
	s_merSkillIdNameList[SKID_MS_MAGNUM - SKID_MERCENARY_BEGIN] = "MS_MAGNUM";
	s_merSkillIdNameList[SKID_MS_BOWLINGBASH - SKID_MERCENARY_BEGIN] = "MS_BOWLINGBASH";
	s_merSkillIdNameList[SKID_MS_PARRYING - SKID_MERCENARY_BEGIN] = "MS_PARRYING";
	s_merSkillIdNameList[SKID_MS_REFLECTSHIELD - SKID_MERCENARY_BEGIN] = "MS_REFLECTSHIELD";
	s_merSkillIdNameList[SKID_MS_BERSERK - SKID_MERCENARY_BEGIN] = "MS_BERSERK";

	s_merSkillIdNameList[SKID_MA_DOUBLE - SKID_MERCENARY_BEGIN] = "MA_DOUBLE";
	s_merSkillIdNameList[SKID_MA_SHOWER - SKID_MERCENARY_BEGIN] = "MA_SHOWER";
	s_merSkillIdNameList[SKID_MA_SKIDTRAP - SKID_MERCENARY_BEGIN] = "MA_SKIDTRAP";
	s_merSkillIdNameList[SKID_MA_LANDMINE - SKID_MERCENARY_BEGIN] = "MA_LANDMINE";
	s_merSkillIdNameList[SKID_MA_SANDMAN - SKID_MERCENARY_BEGIN] = "MA_SANDMAN";
	s_merSkillIdNameList[SKID_MA_FREEZINGTRAP - SKID_MERCENARY_BEGIN] = "MA_FREEZINGTRAP";
	s_merSkillIdNameList[SKID_MA_REMOVETRAP - SKID_MERCENARY_BEGIN] = "MA_REMOVETRAP";
	s_merSkillIdNameList[SKID_MA_CHARGEARROW - SKID_MERCENARY_BEGIN] = "MA_CHARGEARROW";
	s_merSkillIdNameList[SKID_MA_SHARPSHOOTING - SKID_MERCENARY_BEGIN] = "MA_SHARPSHOOTING";

	s_merSkillIdNameList[SKID_ML_PIERCE - SKID_MERCENARY_BEGIN] = "ML_PIERCE";
	s_merSkillIdNameList[SKID_ML_BRANDISH - SKID_MERCENARY_BEGIN] = "ML_BRANDISH";
	s_merSkillIdNameList[SKID_ML_SPIRALPIERCE - SKID_MERCENARY_BEGIN] = "ML_SPIRALPIERCE";
	s_merSkillIdNameList[SKID_ML_DEFENDER - SKID_MERCENARY_BEGIN] = "ML_DEFENDER";
	s_merSkillIdNameList[SKID_ML_AUTOGUARD - SKID_MERCENARY_BEGIN] = "ML_AUTOGUARD";
	s_merSkillIdNameList[SKID_ML_DEVOTION - SKID_MERCENARY_BEGIN] = "ML_DEVOTION";

	s_merSkillIdNameList[SKID_MER_MAGNIFICAT - SKID_MERCENARY_BEGIN] = "MER_MAGNIFICAT";
	s_merSkillIdNameList[SKID_MER_QUICKEN - SKID_MERCENARY_BEGIN] = "MER_QUICKEN";
	s_merSkillIdNameList[SKID_MER_SIGHT - SKID_MERCENARY_BEGIN] = "MER_SIGHT";
	s_merSkillIdNameList[SKID_MER_CRASH - SKID_MERCENARY_BEGIN] = "MER_CRASH";
	s_merSkillIdNameList[SKID_MER_REGAIN - SKID_MERCENARY_BEGIN] = "MER_REGAIN";
	s_merSkillIdNameList[SKID_MER_TENDER - SKID_MERCENARY_BEGIN] = "MER_TENDER";
	s_merSkillIdNameList[SKID_MER_BENEDICTION - SKID_MERCENARY_BEGIN] = "MER_BENEDICTION";
	s_merSkillIdNameList[SKID_MER_RECUPERATE - SKID_MERCENARY_BEGIN] = "MER_RECUPERATE";
	s_merSkillIdNameList[SKID_MER_MENTALCURE - SKID_MERCENARY_BEGIN] = "MER_MENTALCURE";
	s_merSkillIdNameList[SKID_MER_COMPRESS - SKID_MERCENARY_BEGIN] = "MER_COMPRESS";
	s_merSkillIdNameList[SKID_MER_PROVOKE - SKID_MERCENARY_BEGIN] = "MER_PROVOKE";
	s_merSkillIdNameList[SKID_MER_AUTOBERSERK - SKID_MERCENARY_BEGIN] = "MER_AUTOBERSERK";
	s_merSkillIdNameList[SKID_MER_DECAGI - SKID_MERCENARY_BEGIN] = "MER_DECAGI";
	s_merSkillIdNameList[SKID_MER_SCAPEGOAT - SKID_MERCENARY_BEGIN] = "MER_SCAPEGOAT";
	s_merSkillIdNameList[SKID_MER_LEXDIVINA - SKID_MERCENARY_BEGIN] = "MER_LEXDIVINA";
	s_merSkillIdNameList[SKID_MER_ESTIMATION - SKID_MERCENARY_BEGIN] = "MER_ESTIMATION";
}

void SKILL_INFO :: InitTable()
{
	s_skillIdNameList.resize(SKID_LAST_HP, "Skill not defined !!");
	s_skillIdNameList[0]							= "Zero Skill";
	s_skillIdNameList[SKID_NV_BASIC]				= "NV_BASIC";
	s_skillIdNameList[SKID_SM_SWORD]				= "SM_SWORD";
	s_skillIdNameList[SKID_SM_TWOHAND]				= "SM_TWOHAND";
	s_skillIdNameList[SKID_SM_RECOVERY]				= "SM_RECOVERY";
	s_skillIdNameList[SKID_SM_BASH]					= "SM_BASH";
	s_skillIdNameList[SKID_SM_PROVOKE]				= "SM_PROVOKE";
	s_skillIdNameList[SKID_SM_MAGNUM]				= "SM_MAGNUM";
	s_skillIdNameList[SKID_SM_ENDURE]				= "SM_ENDURE";
	s_skillIdNameList[SKID_MG_SRECOVERY]			= "MG_SRECOVERY";
	s_skillIdNameList[SKID_MG_SIGHT]				= "MG_SIGHT";
	s_skillIdNameList[SKID_MG_NAPALMBEAT]			= "MG_NAPALMBEAT";
	s_skillIdNameList[SKID_MG_SAFETYWALL]			= "MG_SAFETYWALL";
	s_skillIdNameList[SKID_MG_SOULSTRIKE]			= "MG_SOULSTRIKE";
	s_skillIdNameList[SKID_MG_COLDBOLT]				= "MG_COLDBOLT";
	s_skillIdNameList[SKID_MG_FROSTDIVER]			= "MG_FROSTDIVER";
	s_skillIdNameList[SKID_MG_STONECURSE]			= "MG_STONECURSE";
	s_skillIdNameList[SKID_MG_FIREBALL]				= "MG_FIREBALL";
	s_skillIdNameList[SKID_MG_FIREWALL]				= "MG_FIREWALL";
	s_skillIdNameList[SKID_MG_FIREBOLT]				= "MG_FIREBOLT";
	s_skillIdNameList[SKID_MG_LIGHTNINGBOLT]		= "MG_LIGHTNINGBOLT";
	s_skillIdNameList[SKID_MG_THUNDERSTORM]			= "MG_THUNDERSTORM";
	s_skillIdNameList[SKID_AL_DP]					= "AL_DP";
	s_skillIdNameList[SKID_AL_DEMONBANE]			= "AL_DEMONBANE";
	s_skillIdNameList[SKID_AL_RUWACH]				= "AL_RUWACH";
	s_skillIdNameList[SKID_AL_PNEUMA]				= "AL_PNEUMA";
	s_skillIdNameList[SKID_AL_TELEPORT]				= "AL_TELEPORT";
	s_skillIdNameList[SKID_AL_WARP]					= "AL_WARP";
	s_skillIdNameList[SKID_AL_HEAL]					= "AL_HEAL";

	s_skillIdNameList[SKID_AL_INCAGI]				= "AL_INCAGI";
	s_skillIdNameList[SKID_CASH_INCAGI]				= "CASH_INCAGI";

	s_skillIdNameList[SKID_AL_DECAGI]				= "AL_DECAGI";
	s_skillIdNameList[SKID_AL_HOLYWATER]			= "AL_HOLYWATER";
	s_skillIdNameList[SKID_AL_CRUCIS]				= "AL_CRUCIS";
	s_skillIdNameList[SKID_AL_ANGELUS]				= "AL_ANGELUS";
	s_skillIdNameList[SKID_AL_BLESSING]				= "AL_BLESSING";
	s_skillIdNameList[SKID_CASH_BLESSING]			= "CASH_BLESSING";
	s_skillIdNameList[SKID_AL_CURE]					= "AL_CURE";
	s_skillIdNameList[SKID_MC_INCCARRY]				= "MC_INCCARRY";
	s_skillIdNameList[SKID_MC_DISCOUNT]				= "MC_DISCOUNT";
	s_skillIdNameList[SKID_MC_OVERCHARGE]			= "MC_OVERCHARGE";
	s_skillIdNameList[SKID_MC_PUSHCART]				= "MC_PUSHCART";
	s_skillIdNameList[SKID_MC_IDENTIFY]				= "MC_IDENTIFY";
	s_skillIdNameList[SKID_MC_VENDING]				= "MC_VENDING";
	s_skillIdNameList[SKID_MC_MAMMONITE]			= "MC_MAMMONITE";
	s_skillIdNameList[SKID_AC_OWL]					= "AC_OWL";
	s_skillIdNameList[SKID_AC_VULTURE]				= "AC_VULTURE";
	s_skillIdNameList[SKID_AC_CONCENTRATION]		= "AC_CONCENTRATION";
	s_skillIdNameList[SKID_AC_DOUBLE]				= "AC_DOUBLE";
	s_skillIdNameList[SKID_HT_POWER]				= "HT_POWER";
	s_skillIdNameList[SKID_AC_SHOWER]				= "AC_SHOWER";
	s_skillIdNameList[SKID_TF_DOUBLE]				= "TF_DOUBLE";
	s_skillIdNameList[SKID_TF_MISS]					= "TF_MISS";
	s_skillIdNameList[SKID_TF_STEAL]				= "TF_STEAL";
	s_skillIdNameList[SKID_TF_HIDING]				= "TF_HIDING";
	s_skillIdNameList[SKID_TF_POISON]				= "TF_POISON";
	s_skillIdNameList[SKID_TF_DETOXIFY]				= "TF_DETOXIFY";
	s_skillIdNameList[SKID_ALL_RESURRECTION]		= "ALL_RESURRECTION";
	s_skillIdNameList[SKID_KN_SPEARMASTERY]			= "KN_SPEARMASTERY";
	s_skillIdNameList[SKID_KN_PIERCE]				= "KN_PIERCE";
	s_skillIdNameList[SKID_KN_BRANDISHSPEAR]		= "KN_BRANDISHSPEAR";
	s_skillIdNameList[SKID_KN_SPEARSTAB]			= "KN_SPEARSTAB";
	s_skillIdNameList[SKID_KN_SPEARBOOMERANG]		= "KN_SPEARBOOMERANG";
	s_skillIdNameList[SKID_KN_TWOHANDQUICKEN]		= "KN_TWOHANDQUICKEN";
	s_skillIdNameList[SKID_KN_AUTOCOUNTER]			= "KN_AUTOCOUNTER";
	s_skillIdNameList[SKID_KN_BOWLINGBASH]			= "KN_BOWLINGBASH";
	s_skillIdNameList[SKID_KN_RIDING]				= "KN_RIDING";
	s_skillIdNameList[SKID_KN_CAVALIERMASTERY]		= "KN_CAVALIERMASTERY";
	s_skillIdNameList[SKID_PR_MACEMASTERY]			= "PR_MACEMASTERY";
	s_skillIdNameList[SKID_PR_IMPOSITIO]			= "PR_IMPOSITIO";
	s_skillIdNameList[SKID_PR_SUFFRAGIUM]			= "PR_SUFFRAGIUM";
	s_skillIdNameList[SKID_PR_ASPERSIO]				= "PR_ASPERSIO";

	s_skillIdNameList[SKID_PR_BENEDICTIO]			= "PR_BENEDICTIO";
	s_skillIdNameList[SKID_ALL_ANGEL_PROTECT]		= "ALL_ANGEL_PROTECT";

	s_skillIdNameList[SKID_PR_SANCTUARY]			= "PR_SANCTUARY";
	s_skillIdNameList[SKID_PR_SLOWPOISON]			= "PR_SLOWPOISON";
	s_skillIdNameList[SKID_PR_STRECOVERY]			= "PR_STRECOVERY";
	s_skillIdNameList[SKID_PR_KYRIE]				= "PR_KYRIE";
	s_skillIdNameList[SKID_PR_MAGNIFICAT]			= "PR_MAGNIFICAT";
	s_skillIdNameList[SKID_PR_GLORIA]				= "PR_GLORIA";
	s_skillIdNameList[SKID_PR_LEXDIVINA]			= "PR_LEXDIVINA";
	s_skillIdNameList[SKID_PR_TURNUNDEAD]			= "PR_TURNUNDEAD";
	s_skillIdNameList[SKID_PR_LEXAETERNA]			= "PR_LEXAETERNA";
	s_skillIdNameList[SKID_PR_MAGNUS]				= "PR_MAGNUS";
	s_skillIdNameList[SKID_WZ_FIREPILLAR]			= "WZ_FIREPILLAR";
	s_skillIdNameList[SKID_WZ_SIGHTRASHER]			= "WZ_SIGHTRASHER";
	s_skillIdNameList[SKID_WZ_FIREIVY]				= "WZ_FIREIVY";
	s_skillIdNameList[SKID_WZ_WATERBALL]			= "WZ_WATERBALL";
	s_skillIdNameList[SKID_WZ_ICEWALL]				= "WZ_ICEWALL";
	s_skillIdNameList[SKID_WZ_METEOR]				= "WZ_METEOR";
	s_skillIdNameList[SKID_WZ_JUPITEL]				= "WZ_JUPITEL";
	s_skillIdNameList[SKID_WZ_VERMILION]			= "WZ_VERMILION";
	s_skillIdNameList[SKID_WZ_FROSTNOVA]			= "WZ_FROSTNOVA";
	s_skillIdNameList[SKID_WZ_STORMGUST]			= "WZ_STORMGUST";
	s_skillIdNameList[SKID_WZ_EARTHSPIKE]			= "WZ_EARTHSPIKE";
	s_skillIdNameList[SKID_WZ_HEAVENDRIVE]			= "WZ_HEAVENDRIVE";
	s_skillIdNameList[SKID_WZ_QUAGMIRE]				= "WZ_QUAGMIRE";
	s_skillIdNameList[SKID_WZ_ESTIMATION]			= "WZ_ESTIMATION";
	s_skillIdNameList[SKID_HT_SKIDTRAP]				= "HT_SKIDTRAP";
	s_skillIdNameList[SKID_HT_LANDMINE]				= "HT_LANDMINE";
	s_skillIdNameList[SKID_HT_ANKLESNARE]			= "HT_ANKLESNARE";
	s_skillIdNameList[SKID_HT_SHOCKWAVE]			= "HT_SHOCKWAVE";

	s_skillIdNameList[SKID_HT_SANDMAN]				= "HT_SANDMAN";
	s_skillIdNameList[SKID_GM_SANDMAN]				= "GM_SANDMAN";
	s_skillIdNameList[SKID_ALL_CATCRY]				= "ALL_CATCRY";
	s_skillIdNameList[SKID_ALL_DREAM_SUMMERNIGHT]	= "ALL_DREAM_SUMMERNIGHT";
	s_skillIdNameList[SKID_ALL_PARTYFLEE]			= "ALL_PARTYFLEE";

	s_skillIdNameList[SKID_HT_FLASHER]				= "HT_FLASHER";
	s_skillIdNameList[SKID_HT_FREEZINGTRAP]			= "HT_FREEZINGTRAP";
	s_skillIdNameList[SKID_HT_BLASTMINE]			= "HT_BLASTMINE";
	s_skillIdNameList[SKID_HT_CLAYMORETRAP]			= "HT_CLAYMORETRAP";
	s_skillIdNameList[SKID_HT_REMOVETRAP]			= "HT_REMOVETRAP";
	s_skillIdNameList[SKID_HT_TALKIEBOX]			= "HT_TALKIEBOX";
	s_skillIdNameList[SKID_HT_BEASTBANE]			= "HT_BEASTBANE";
	s_skillIdNameList[SKID_HT_FALCON]				= "HT_FALCON";
	s_skillIdNameList[SKID_HT_STEELCROW]			= "HT_STEELCROW";
	s_skillIdNameList[SKID_HT_BLITZBEAT]			= "HT_BLITZBEAT";
	s_skillIdNameList[SKID_HT_DETECTING]			= "HT_DETECTING";
	s_skillIdNameList[SKID_HT_SPRINGTRAP]			= "HT_SPRINGTRAP";
	s_skillIdNameList[SKID_AS_RIGHT]				= "AS_RIGHT";
	s_skillIdNameList[SKID_AS_LEFT]					= "AS_LEFT";
	s_skillIdNameList[SKID_AS_KATAR]				= "AS_KATAR";
	s_skillIdNameList[SKID_AS_CLOAKING]				= "AS_CLOAKING";
	s_skillIdNameList[SKID_AS_SONICBLOW]			= "AS_SONICBLOW";
	s_skillIdNameList[SKID_AS_GRIMTOOTH]			= "AS_GRIMTOOTH";
	s_skillIdNameList[SKID_AS_ENCHANTPOISON]		= "AS_ENCHANTPOISON";
	s_skillIdNameList[SKID_AS_POISONREACT]			= "AS_POISONREACT";
	s_skillIdNameList[SKID_AS_VENOMDUST]			= "AS_VENOMDUST";
	s_skillIdNameList[SKID_AS_SPLASHER]				= "AS_SPLASHER";
	s_skillIdNameList[SKID_BS_IRON]					= "BS_IRON";
	s_skillIdNameList[SKID_BS_STEEL]				= "BS_STEEL";
	s_skillIdNameList[SKID_BS_ENCHANTEDSTONE]		= "BS_ENCHANTEDSTONE";
	s_skillIdNameList[SKID_BS_ORIDEOCON]			= "BS_ORIDEOCON";
	s_skillIdNameList[SKID_BS_DAGGER]				= "BS_DAGGER";
	s_skillIdNameList[SKID_BS_SWORD]				= "BS_SWORD";
	s_skillIdNameList[SKID_BS_TWOHANDSWORD]			= "BS_TWOHANDSWORD";
	s_skillIdNameList[SKID_BS_AXE]					= "BS_AXE";
	s_skillIdNameList[SKID_BS_MACE]					= "BS_MACE";
	s_skillIdNameList[SKID_BS_KNUCKLE]				= "BS_KNUCKLE";
	s_skillIdNameList[SKID_BS_SPEAR]				= "BS_SPEAR";
	s_skillIdNameList[SKID_BS_HILTBINDING]			= "BS_HILTBINDING";
	s_skillIdNameList[SKID_BS_FINDINGORE]			= "BS_FINDINGORE";
	s_skillIdNameList[SKID_BS_WEAPONRESEARCH]		= "BS_WEAPONRESEARCH";
	s_skillIdNameList[SKID_BS_REPAIRWEAPON]			= "BS_REPAIRWEAPON";
	s_skillIdNameList[SKID_BS_SKINTEMPER]			= "BS_SKINTEMPER";
	s_skillIdNameList[SKID_BS_HAMMERFALL]			= "BS_HAMMERFALL";
	s_skillIdNameList[SKID_BS_ADRENALINE]			= "BS_ADRENALINE";
	s_skillIdNameList[SKID_BS_WEAPONPERFECT]		= "BS_WEAPONPERFECT";
	s_skillIdNameList[SKID_BS_OVERTHRUST]			= "BS_OVERTHRUST";
	s_skillIdNameList[SKID_BS_MAXIMIZE]				= "BS_MAXIMIZE";

#if defined(BALANCE_PATCH)

	s_skillIdNameList[SKID_NV_FIRSTAID]				= "NV_FIRSTAID";
	s_skillIdNameList[SKID_NV_TRICKDEAD]			= "NV_TRICKDEAD";
	s_skillIdNameList[SKID_SM_MOVINGRECOVERY]		= "SM_MOVINGRECOVERY";
	s_skillIdNameList[SKID_SM_FATALBLOW]			= "SM_FATALBLOW";
	s_skillIdNameList[SKID_SM_AUTOBERSERK]			= "SM_AUTOBERSERK";
	s_skillIdNameList[SKID_AC_MAKINGARROW]			= "AC_MAKINGARROW";
	s_skillIdNameList[SKID_AC_CHARGEARROW]			= "AC_CHARGEARROW";
	s_skillIdNameList[SKID_TF_SPRINKLESAND]		= "TF_SPRINKLESAND";
	s_skillIdNameList[SKID_TF_BACKSLIDING]			= "TF_BACKSLIDING";
	s_skillIdNameList[SKID_TF_PICKSTONE]			= "TF_PICKSTONE";
	s_skillIdNameList[SKID_TF_THROWSTONE]			= "TF_THROWSTONE";
	s_skillIdNameList[SKID_MC_CARTREVOLUTION]		= "MC_CARTREVOLUTION";
	s_skillIdNameList[SKID_MC_CHANGECART]			= "MC_CHANGECART";
	s_skillIdNameList[SKID_MC_LOUD]					= "MC_LOUD";
	s_skillIdNameList[SKID_AL_HOLYLIGHT]			= "AL_HOLYLIGHT";
	s_skillIdNameList[SKID_MG_ENERGYCOAT]			= "MG_ENERGYCOAT";
#endif
#ifdef	MOB_SKILL
	s_skillIdNameList[SKID_NPC_PIERCINGATT]			= "NPC_PIERCINGATT";
	s_skillIdNameList[SKID_NPC_MENTALBREAKER]			= "NPC_MENTALBREAKER";
	s_skillIdNameList[SKID_NPC_RANGEATTACK]			= "NPC_RANGEATTACK";
	s_skillIdNameList[SKID_NPC_ATTRICHANGE]			= "NPC_ATTRICHANGE";
	s_skillIdNameList[SKID_NPC_CHANGEWATER]			= "NPC_CHANGEWATER";
	s_skillIdNameList[SKID_NPC_CHANGEGROUND]			= "NPC_CHANGEGROUND";
	s_skillIdNameList[SKID_NPC_CHANGEFIRE]				= "NPC_CHANGEFIRE";
	s_skillIdNameList[SKID_NPC_CHANGEWIND]				= "NPC_CHANGEWIND";
	s_skillIdNameList[SKID_NPC_CHANGEPOISON]			= "NPC_CHANGEPOISON";
	s_skillIdNameList[SKID_NPC_CHANGEHOLY]				= "NPC_CHANGEHOLY";
	s_skillIdNameList[SKID_NPC_CHANGEDARKNESS]		= "NPC_CHANGETELEKINESIS";
	s_skillIdNameList[SKID_NPC_CRITICALSLASH]			= "NPC_CRITICALSLASH";
	s_skillIdNameList[SKID_NPC_COMBOATTACK]			= "NPC_COMBOATTACK";
	s_skillIdNameList[SKID_NPC_GUIDEDATTACK]			= "NPC_GUIDEDATTACK";
	s_skillIdNameList[SKID_NPC_SELFDESTRUCTION]		= "NPC_SELFDESTRUCTION";
	s_skillIdNameList[SKID_NPC_SPLASHATTACK]			= "NPC_SPLASHATTACK";
	s_skillIdNameList[SKID_NPC_SUICIDE]					= "NPC_SUICIDE";
	s_skillIdNameList[SKID_NPC_POISON]					= "NPC_POISON";
	s_skillIdNameList[SKID_NPC_BLINDATTACK]			= "NPC_BLINDATTACK";
	s_skillIdNameList[SKID_NPC_SILENCEATTACK]			= "NPC_SILENCEATTACK";
	s_skillIdNameList[SKID_NPC_STUNATTACK]				= "NPC_STUNATTACK";
	s_skillIdNameList[SKID_NPC_PETRIFYATTACK]			= "NPC_PETRIFYATTACK";
	s_skillIdNameList[SKID_NPC_CURSEATTACK]			= "NPC_CURSEATTACK";
	s_skillIdNameList[SKID_NPC_SLEEPATTACK]			= "NPC_SLEEPATTACK";
	s_skillIdNameList[SKID_NPC_RANDOMATTACK]			= "NPC_RANDOMATTACK";
	s_skillIdNameList[SKID_NPC_WATERATTACK]			= "NPC_WATERATTACK";
	s_skillIdNameList[SKID_NPC_GROUNDATTACK]			= "NPC_GROUNDATTACK";
	s_skillIdNameList[SKID_NPC_FIREATTACK]				= "NPC_FIREATTACK";
	s_skillIdNameList[SKID_NPC_WINDATTACK]				= "NPC_WINDATTACK";
	s_skillIdNameList[SKID_NPC_POISONATTACK]			= "NPC_POISONATTACK";
	s_skillIdNameList[SKID_NPC_HOLYATTACK]				= "NPC_HOLYATTACK";
	s_skillIdNameList[SKID_NPC_DARKNESSATTACK]		= "NPC_DARKNESSATTACK";
	s_skillIdNameList[SKID_NPC_TELEKINESISATTACK]	= "NPC_TELEKINESISATTACK";
	s_skillIdNameList[SKID_NPC_MAGICALATTACK]			= "NPC_MAGICALATTACK";
	s_skillIdNameList[SKID_NPC_METAMORPHOSIS]			= "NPC_METAMORPHOSIS";
	s_skillIdNameList[SKID_NPC_PROVOCATION]			= "NPC_PROVOCATION";
	s_skillIdNameList[SKID_NPC_SMOKING]					= "NPC_SMOKING";
	s_skillIdNameList[SKID_NPC_SUMMONSLAVE]			= "NPC_SUMMONSLAVE";
	s_skillIdNameList[SKID_NPC_EMOTION]					= "NPC_EMOTION";
	s_skillIdNameList[SKID_NPC_TRANSFORMATION]		= "NPC_TRANSFORMATION";
	s_skillIdNameList[SKID_NPC_BLOODDRAIN]				= "NPC_BLOODDRAIN";
	s_skillIdNameList[SKID_NPC_ENERGYDRAIN]			= "NPC_ENERGYDRAIN";
	s_skillIdNameList[SKID_NPC_KEEPING]					= "NPC_KEEPING";
	s_skillIdNameList[SKID_NPC_DARKBREATH]				= "NPC_DARKBREATH";
	s_skillIdNameList[SKID_NPC_DARKBLESSING]			= "NPC_DARKBLESSING";
	s_skillIdNameList[SKID_NPC_BARRIER]					= "NPC_BARRIER";
	s_skillIdNameList[SKID_NPC_DEFENDER]				= "NPC_DEFENDER";
	s_skillIdNameList[SKID_NPC_LICK]						= "NPC_LICK";
	s_skillIdNameList[SKID_NPC_HALLUCINATION]			= "NPC_HALLUCINATION";
	s_skillIdNameList[SKID_NPC_REBIRTH]					= "NPC_REBIRTH";
	s_skillIdNameList[SKID_NPC_SUMMONMONSTER]			= "NPC_SUMMONMONSTER";
	s_skillIdNameList[SKID_NPC_DRAGONFEAR]				= "NPC_DRAGONFEAR";
	s_skillIdNameList[SKID_NPC_BLOODDRAIN]				= "NPC_BLOODDRAIN";
#endif

#ifdef	SECONDJOB_SKILL2
	s_skillIdNameList[SKID_RG_SNATCHER]					= "RG_SNATCHER";
	s_skillIdNameList[SKID_RG_STEALCOIN]				= "RG_STEALCOIN";
	s_skillIdNameList[SKID_RG_BACKSTAP]					= "RG_BACKSTAP";
	s_skillIdNameList[SKID_RG_TUNNELDRIVE]				= "RG_TUNNELDRIVE";
	s_skillIdNameList[SKID_RG_RAID]						= "RG_RAID";
	s_skillIdNameList[SKID_RG_STRIPWEAPON]				= "RG_STRIPWEAPON";
	s_skillIdNameList[SKID_RG_STRIPSHIELD]				= "RG_STRIPSHIELD";
	s_skillIdNameList[SKID_RG_STRIPARMOR]				= "RG_STRIPARMOR";
	s_skillIdNameList[SKID_RG_STRIPHELM]				= "RG_STRIPHELM";
	s_skillIdNameList[SKID_RG_INTIMIDATE]				= "RG_INTIMIDATE";
	s_skillIdNameList[SKID_RG_GRAFFITI]					= "RG_GRAFFITI";
	s_skillIdNameList[SKID_RG_FLAGGRAFFITI]			= "RG_FLAGGRAFFITI";
	s_skillIdNameList[SKID_RG_CLEANER]					= "RG_CLEANER";
	s_skillIdNameList[SKID_RG_GANGSTER]					= "RG_GANGSTER";
	s_skillIdNameList[SKID_RG_COMPULSION]				= "RG_COMPULSION";
	s_skillIdNameList[SKID_RG_PLAGIARISM]				= "RG_PLAGIARISM";

	s_skillIdNameList[SKID_AM_AXEMASTERY]				= "AM_AXEMASTERY";
	s_skillIdNameList[SKID_AM_LEARNINGPOTION]			= "AM_LEARNINGPOTION";
	s_skillIdNameList[SKID_AM_PHARMACY]					= "AM_PHARMACY";
	s_skillIdNameList[SKID_AM_DEMONSTRATION]			= "AM_DEMONSTRATION";
	s_skillIdNameList[SKID_AM_ACIDTERROR]				= "AM_ACIDTERROR";
	s_skillIdNameList[SKID_AM_POTIONPITCHER]			= "AM_POTIONPITCHER";
	s_skillIdNameList[SKID_AM_CANNIBALIZE]				= "AM_CANNIBALIZE";
	s_skillIdNameList[SKID_AM_SPHEREMINE]				= "AM_SPHEREMINE";
	s_skillIdNameList[SKID_AM_CP_WEAPON]				= "AM_CP_WEAPON";
	s_skillIdNameList[SKID_AM_CP_SHIELD]				= "AM_CP_SHIELD";
	s_skillIdNameList[SKID_AM_CP_ARMOR]					= "AM_CP_ARMOR";
	s_skillIdNameList[SKID_AM_CP_HELM]					= "AM_CP_HELM";
	s_skillIdNameList[SKID_AM_BIOETHICS]				= "AM_BIOETHICS";
	s_skillIdNameList[SKID_AM_BIOTECHNOLOGY]			= "AM_BIOTECHNOLOGY";
	s_skillIdNameList[SKID_AM_CREATECREATURE]			= "AM_CREATECREATURE";
	s_skillIdNameList[SKID_AM_CULTIVATION]				= "AM_CULTIVATION";
	s_skillIdNameList[SKID_AM_FLAMECONTROL]			= "AM_FLAMECONTROL";
	s_skillIdNameList[SKID_AM_CALLHOMUN]				= "AM_CALLHOMUN";
	s_skillIdNameList[SKID_AM_REST]						= "AM_REST";
	s_skillIdNameList[SKID_AM_DRILLMASTER]				= "AM_DRILLMASTER";
	s_skillIdNameList[SKID_AM_HEALHOMUN]				= "AM_HEALHOMUN";
	s_skillIdNameList[SKID_AM_RESURRECTHOMUN]			= "AM_RESURRECTHOMUN";

	s_skillIdNameList[SKID_CR_TRUST]						= "CR_TRUST";
	s_skillIdNameList[SKID_CR_AUTOGUARD]				= "CR_AUTOGUARD";
	s_skillIdNameList[SKID_CR_SHIELDCHARGE]			= "CR_SHIELDCHARGE";
	s_skillIdNameList[SKID_CR_SHIELDBOOMERANG]		= "CR_SHIELDBOOMERANG";
	s_skillIdNameList[SKID_CR_REFLECTSHIELD]			= "CR_REFLECTSHIELD";
	s_skillIdNameList[SKID_CR_HOLYCROSS]				= "CR_HOLYCROSS";
	s_skillIdNameList[SKID_CR_GRANDCROSS]				= "CR_GRANDCROSS";
	s_skillIdNameList[SKID_CR_DEVOTION]					= "CR_DEVOTION";
	s_skillIdNameList[SKID_CR_PROVIDENCE]				= "CR_PROVIDENCE";
	s_skillIdNameList[SKID_CR_DEFENDER]					= "CR_DEFENDER";
	s_skillIdNameList[SKID_CR_SPEARQUICKEN]			= "CR_SPEARQUICKEN";

	s_skillIdNameList[SKID_MO_IRONHAND]					= "MO_IRONHAND";
	s_skillIdNameList[SKID_MO_SPIRITSRECOVERY]		= "MO_SPIRITSRECOVERY";
	s_skillIdNameList[SKID_MO_CALLSPIRITS]				= "MO_CALLSPIRITS";
	s_skillIdNameList[SKID_MO_ABSORBSPIRITS]			= "MO_ABSORBSPIRITS";
	s_skillIdNameList[SKID_MO_TRIPLEATTACK]			= "MO_TRIPLEATTACK";
	s_skillIdNameList[SKID_MO_BODYRELOCATION]			= "MO_BODYRELOCATION";
	s_skillIdNameList[SKID_MO_DODGE]						= "MO_DODGE";
	s_skillIdNameList[SKID_MO_INVESTIGATE]				= "MO_INVESTIGATE";
	s_skillIdNameList[SKID_MO_FINGEROFFENSIVE]		= "MO_FINGEROFFENSIVE";
	s_skillIdNameList[SKID_MO_STEELBODY]				= "MO_STEELBODY";
	s_skillIdNameList[SKID_MO_BLADESTOP]				= "MO_BLADESTOP";
	s_skillIdNameList[SKID_MO_EXPLOSIONSPIRITS]		= "MO_EXPLOSIONSPIRITS";
	s_skillIdNameList[SKID_MO_EXTREMITYFIST]			= "MO_EXTREMITYFIST";
	s_skillIdNameList[SKID_MO_CHAINCOMBO]				= "MO_CHAINCOMBO";
	s_skillIdNameList[SKID_MO_COMBOFINISH]				= "MO_COMBOFINISH";

	s_skillIdNameList[SKID_SA_ADVANCEDBOOK]				= "SA_ADVANCEDBOOK";
	s_skillIdNameList[SKID_SA_SPELLBREAKER]				= "SA_SPELLBREAKER";
	s_skillIdNameList[SKID_SA_CASTCANCEL]				= "SA_CASTCANCEL";
	s_skillIdNameList[SKID_SA_MAGICROD]					= "SA_MAGICROD";
	s_skillIdNameList[SKID_SA_FREECAST]					= "SA_FREECAST";
	s_skillIdNameList[SKID_SA_FLAMELAUNCHER]			= "SA_FLAMELAUNCHER";
	s_skillIdNameList[SKID_SA_FROSTWEAPON]				= "SA_FROSTWEAPON";
	s_skillIdNameList[SKID_SA_LIGHTNINGLOADER]			= "SA_LIGHTNINGLOADER";
	s_skillIdNameList[SKID_SA_SEISMICWEAPON]			= "SA_SEISMICWEAPON";
	s_skillIdNameList[SKID_SA_DRAGONOLOGY]				= "SA_DRAGONOLOGY";
	s_skillIdNameList[SKID_SA_VOLCANO]					= "SA_VOLCANO";
	s_skillIdNameList[SKID_SA_DELUGE]					= "SA_DELUGE";
	s_skillIdNameList[SKID_SA_VIOLENTGALE]				= "SA_VIOLENTGALE";
	s_skillIdNameList[SKID_SA_LANDPROTECTOR]			= "SA_LANDPROTECTOR";

	s_skillIdNameList[SKID_SA_AUTOSPELL]				= "SA_AUTOSPELL";

	s_skillIdNameList[SKID_SA_LANDPROTECTOR]			= "SA_LANDPROTECTOR";
	s_skillIdNameList[SKID_SA_DISPELL]					= "SA_DISPELL";
	s_skillIdNameList[SKID_SA_ABRACADABRA]				= "SA_ABRACADABRA";
	s_skillIdNameList[SKID_SA_MONOCELL]					= "SA_MONOCELL";
	s_skillIdNameList[SKID_SA_CLASSCHANGE]				= "SA_CLASSCHANGE";
	s_skillIdNameList[SKID_SA_SUMMONMONSTER]			= "SA_SUMMONMONSTER";
	s_skillIdNameList[SKID_SA_REVERSEORCISH]			= "SA_REVERSEORCISH";
	s_skillIdNameList[SKID_SA_DEATH]					= "SA_DEATH";
	s_skillIdNameList[SKID_SA_FORTUNE]					= "SA_FORTUNE";
	s_skillIdNameList[SKID_SA_TAMINGMONSTER]			= "SA_TAMINGMONSTER";
	s_skillIdNameList[SKID_SA_QUESTION]					= "SA_QUESTION";
	s_skillIdNameList[SKID_SA_GRAVITY]					= "SA_GRAVITY";
	s_skillIdNameList[SKID_SA_LEVELUP]					= "SA_LEVELUP";
	s_skillIdNameList[SKID_SA_INSTANTDEATH]				= "SA_INSTANTDEATH";
	s_skillIdNameList[SKID_SA_FULLRECOVERY]				= "SA_FULLRECOVERY";
	s_skillIdNameList[SKID_SA_COMA]						= "SA_COMA";
	s_skillIdNameList[SKID_BD_ADAPTATION]				= "BD_ADAPTATION";
	s_skillIdNameList[SKID_BD_ENCORE]					= "BD_ENCORE";
	s_skillIdNameList[SKID_BD_LULLABY]					= "BD_LULLABY";
	s_skillIdNameList[SKID_BD_RICHMANKIM]				= "BD_RICHMANKIM";
	s_skillIdNameList[SKID_BD_ETERNALCHAOS]				= "BD_ETERNALCHAOS";
	s_skillIdNameList[SKID_BD_DRUMBATTLEFIELD]			= "BD_DRUMBATTLEFIELD";
	s_skillIdNameList[SKID_BD_RINGNIBELUNGEN]			= "BD_RINGNIBELUNGEN";
	s_skillIdNameList[SKID_BD_ROKISWEIL]				= "BD_ROKISWEIL";
	s_skillIdNameList[SKID_BD_INTOABYSS]				= "BD_INTOABYSS";
	s_skillIdNameList[SKID_BD_SIEGFRIED]				= "BD_SIEGFRIED";
	s_skillIdNameList[SKID_BD_RAGNAROK]					= "BD_RAGNAROK";
	s_skillIdNameList[SKID_BA_MUSICALLESSON]			= "BA_MUSICALLESSON";
	s_skillIdNameList[SKID_BA_MUSICALSTRIKE]			= "BA_MUSICALSTRIKE";
	s_skillIdNameList[SKID_BA_DISSONANCE]				= "BA_DISSONANCE";
	s_skillIdNameList[SKID_BA_FROSTJOKER]				= "BA_FROSTJOKE";
	s_skillIdNameList[SKID_BA_WHISTLE]					= "BA_WHISTLE";
	s_skillIdNameList[SKID_BA_ASSASSINCROSS]			= "BA_ASSASSINCROSS";
	s_skillIdNameList[SKID_BA_POEMBRAGI]				= "BA_POEMBRAGI";
	s_skillIdNameList[SKID_BA_APPLEIDUN]				= "BA_APPLEIDUN";
	s_skillIdNameList[SKID_DC_THROWARROW]				= "DC_THROWARROW";
	s_skillIdNameList[SKID_DC_UGLYDANCE]				= "DC_UGLYDANCE";
	s_skillIdNameList[SKID_DC_SCREAM]					= "DC_SCREAM";
	s_skillIdNameList[SKID_DC_HUMMING]					= "DC_HUMMING";
	s_skillIdNameList[SKID_DC_DONTFORGETME]				= "DC_DONTFORGETME";
	s_skillIdNameList[SKID_DC_FORTUNEKISS]				= "DC_FORTUNEKISS";
	s_skillIdNameList[SKID_DC_SERVICEFORYOU]			= "DC_SERVICEFORYOU";
	s_skillIdNameList[SKID_DC_DANCINGLESSON]			= "DC_DANCINGLESSON";
#endif

#ifdef WEDDING_PATCH
	s_skillIdNameList[SKID_WE_MALE]						= "WE_MALE";
	s_skillIdNameList[SKID_WE_FEMALE]					= "WE_FEMALE";
	s_skillIdNameList[SKID_WE_CALLPARTNER]				= "WE_CALLPARTNER";
#endif

	s_skillIdNameList[SKID_LK_AURABLADE]				= "LK_AURABLADE";
	s_skillIdNameList[SKID_LK_PARRYING]					= "LK_PARRYING";
	s_skillIdNameList[SKID_LK_CONCENTRATION]			= "LK_CONCENTRATION";
	s_skillIdNameList[SKID_LK_TENSIONRELAX]				= "LK_TENSIONRELAX";
	s_skillIdNameList[SKID_LK_BERSERK]					= "LK_BERSERK";
	s_skillIdNameList[SKID_LK_FURY]						= "LK_FURY";

	s_skillIdNameList[SKID_PA_PRESSURE]					= "PA_PRESSURE";
	s_skillIdNameList[SKID_PA_SACRIFICE]				= "PA_SACRIFICE";
	s_skillIdNameList[SKID_PA_GOSPEL]					= "PA_GOSPEL";

	s_skillIdNameList[SKID_HP_ASSUMPTIO]				= "HP_ASSUMPTIO";
	s_skillIdNameList[SKID_CASH_ASSUMPTIO]				= "CASH_ASSUMPTIO";

	s_skillIdNameList[SKID_HP_BASILICA]					= "HP_BASILICA";
	s_skillIdNameList[SKID_HP_MEDITATIO]				= "HP_MEDITATIO";

	s_skillIdNameList[SKID_HW_SOULDRAIN]				= "HW_SOULDRAIN";
	s_skillIdNameList[SKID_HW_MAGICCRASHER]				= "HW_MAGICCRASHER";
	s_skillIdNameList[SKID_HW_MAGICPOWER]				= "HW_MAGICPOWER";
	s_skillIdNameList[SKID_CH_PALMSTRIKE]				= "CH_PALMSTRIKE";
	s_skillIdNameList[SKID_CH_TIGERFIST]				= "CH_TIGERFIST";
	s_skillIdNameList[SKID_CH_CHAINCRUSH]				= "CH_CHAINCRUSH";
	s_skillIdNameList[SKID_PF_HPCONVERSION]				= "PF_HPCONVERSION";
	s_skillIdNameList[SKID_PF_SOULCHANGE]				= "PF_SOULCHANGE";
	s_skillIdNameList[SKID_PF_SOULBURN]					= "PF_SOULBURN";
	s_skillIdNameList[SKID_ASC_KATAR]					= "ASC_KATAR";
	s_skillIdNameList[SKID_ASC_HALLUCINATION]			= "ASC_HALLUCINATION";

	s_skillIdNameList[SKID_ASC_BREAKER]					= "ASC_BREAKER";
	s_skillIdNameList[SKID_SN_SIGHT]					= "SN_SIGHT";
	s_skillIdNameList[SKID_SN_FALCONASSAULT]			= "SN_FALCONASSAULT";
	s_skillIdNameList[SKID_SN_SHARPSHOOTING]			= "SN_SHARPSHOOTING";
	s_skillIdNameList[SKID_SN_WINDWALK]					= "SN_WINDWALK";
	s_skillIdNameList[SKID_WS_MELTDOWN]					= "WS_MELTDOWN";
	s_skillIdNameList[SKID_WS_CREATECOIN]				= "WS_CREATECOIN";
	s_skillIdNameList[SKID_WS_CREATENUGGET]				= "WS_CREATENUGGET";
	s_skillIdNameList[SKID_WS_CARTBOOST]				= "WS_CARTBOOST";
	s_skillIdNameList[SKID_WS_SYSTEMCREATE]				= "WS_SYSTEMCREATE";
	s_skillIdNameList[SKID_ST_CHASEWALK]				= "ST_CHASEWALK";
	s_skillIdNameList[SKID_ST_REJECTSWORD]				= "ST_REJECTSWORD";
	s_skillIdNameList[SKID_ST_STEALBACKPACK]			= "ST_STEALBACKPACK";
	s_skillIdNameList[SKID_CR_ALCHEMY]					= "CR_ALCHEMY";
	s_skillIdNameList[SKID_CR_SYNTHESISPOTION]			= "CR_SYNTHESISPOTION";
	s_skillIdNameList[SKID_CG_ARROWVULCAN]				= "CG_ARROWVULCAN";
	s_skillIdNameList[SKID_CG_MOONLIT]					= "CG_MOONLIT";
	s_skillIdNameList[SKID_CG_MARIONETTE]				= "CG_MARIONETTE";

	s_skillIdNameList[SKID_LK_SPIRALPIERCE]				= "LK_SPIRALPIERCE";
	s_skillIdNameList[SKID_LK_HEADCRUSH]				= "LK_HEADCRUSH";
	s_skillIdNameList[SKID_LK_JOINTBEAT]				= "LK_JOINTBEAT";
	s_skillIdNameList[SKID_HW_NAPALMVULCAN]				= "HW_NAPALMVULCAN";
	s_skillIdNameList[SKID_CH_SOULCOLLECT]				= "CH_SOULCOLLECT";
	s_skillIdNameList[SKID_PF_MINDBREAKER]				= "PF_MINDBREAKER";
	s_skillIdNameList[SKID_PF_MEMORIZE]					= "PF_MEMORIZE";
	s_skillIdNameList[SKID_PF_FOGWALL]					= "PF_FOGWALL";
	s_skillIdNameList[SKID_PF_SPIDERWEB]				= "PF_SPIDERWEB";
	s_skillIdNameList[SKID_ASC_METEORASSAULT]			= "ASC_METEORASSAULT";

	s_skillIdNameList[SKID_ASC_CDP]						= "ASC_CDP";
	s_skillIdNameList[SKID_ASC_EDP]						= "ASC_EDP";

	s_skillIdNameList[SKID_WE_BABY]						= "WE_BABY";
	s_skillIdNameList[SKID_WE_CALLBABY]					= "WE_CALLBABY";
	s_skillIdNameList[SKID_WE_CALLPARENT]				= "WE_CALLPARENT";

	s_skillIdNameList[SKID_TK_RUN]						= "TK_RUN";
	s_skillIdNameList[SKID_TK_READYSTORM]				= "TK_READYSTORM";
	s_skillIdNameList[SKID_TK_STORMKICK]				= "TK_STORMKICK";
	s_skillIdNameList[SKID_TK_READYDOWN]				= "TK_READYDOWN";
	s_skillIdNameList[SKID_TK_DOWNKICK]					= "TK_DOWNKICK";
	s_skillIdNameList[SKID_TK_READYTURN]				= "TK_READYTURN";
	s_skillIdNameList[SKID_TK_TURNKICK]					= "TK_TURNKICK";
	s_skillIdNameList[SKID_TK_READYCOUNTER]				= "TK_READYCOUNTER";
	s_skillIdNameList[SKID_TK_COUNTER]					= "TK_COUNTER";
	s_skillIdNameList[SKID_TK_DODGE]					= "TK_DODGE";
	s_skillIdNameList[SKID_TK_JUMPKICK]					= "TK_JUMPKICK";
	s_skillIdNameList[SKID_TK_HPTIME]					= "TK_HPTIME";
	s_skillIdNameList[SKID_TK_SPTIME]					= "TK_SPTIME";
	s_skillIdNameList[SKID_TK_POWER]					= "TK_POWER";
	s_skillIdNameList[SKID_TK_SEVENWIND]				= "TK_SEVENWIND";
	s_skillIdNameList[SKID_TK_MISSION]					= "TK_MISSION";
	s_skillIdNameList[SKID_TK_HIGHJUMP]					= "TK_HIGHJUMP";
	s_skillIdNameList[SKID_SG_FEEL]						= "SG_FEEL";
	s_skillIdNameList[SKID_SG_SUN_WARM]					= "SG_SUN_WARM";
	s_skillIdNameList[SKID_SG_MOON_WARM]				= "SG_MOON_WARM";
	s_skillIdNameList[SKID_SG_STAR_WARM]				= "SG_STAR_WARM";
	s_skillIdNameList[SKID_SG_SUN_COMFORT]				= "SG_SUN_COMFORT";
	s_skillIdNameList[SKID_SG_MOON_COMFORT]				= "SG_MOON_COMFORT";
	s_skillIdNameList[SKID_SG_STAR_COMFORT]				= "SG_STAR_COMFORT";
	s_skillIdNameList[SKID_SG_HATE]						= "SG_HATE";
	s_skillIdNameList[SKID_SG_SUN_ANGER]				= "SG_SUN_ANGER";
	s_skillIdNameList[SKID_SG_MOON_ANGER]				= "SG_MOON_ANGER";
	s_skillIdNameList[SKID_SG_STAR_ANGER]				= "SG_STAR_ANGER";
	s_skillIdNameList[SKID_SG_SUN_BLESS]				= "SG_SUN_BLESS";
	s_skillIdNameList[SKID_SG_MOON_BLESS]				= "SG_MOON_BLESS";
	s_skillIdNameList[SKID_SG_STAR_BLESS]				= "SG_STAR_BLESS";
	s_skillIdNameList[SKID_SG_DEVIL]					= "SG_DEVIL";
	s_skillIdNameList[SKID_SG_FRIEND]					= "SG_FRIEND";
	s_skillIdNameList[SKID_SG_KNOWLEDGE]				= "SG_KNOWLEDGE";
	s_skillIdNameList[SKID_SG_FUSION]					= "SG_FUSION";
	s_skillIdNameList[SKID_SL_ALCHEMIST]				= "SL_ALCHEMIST";
	s_skillIdNameList[SKID_AM_BERSERKPITCHER]			= "AM_BERSERKPITCHER";
	s_skillIdNameList[SKID_SL_MONK]						= "SL_MONK";
	s_skillIdNameList[SKID_SL_STAR]						= "SL_STAR";
	s_skillIdNameList[SKID_SL_SAGE]						= "SL_SAGE";
	s_skillIdNameList[SKID_SL_CRUSADER]					= "SL_CRUSADER";
	s_skillIdNameList[SKID_SL_SUPERNOVICE]				= "SL_SUPERNOVICE";
	s_skillIdNameList[SKID_SL_KNIGHT]					= "SL_KNIGHT";
	s_skillIdNameList[SKID_SL_WIZARD]					= "SL_WIZARD";
	s_skillIdNameList[SKID_SL_PRIEST]					= "SL_PRIEST";
	s_skillIdNameList[SKID_SL_BARDDANCER]				= "SL_BARDDANCER";
	s_skillIdNameList[SKID_SL_ROGUE]					= "SL_ROGUE";
	s_skillIdNameList[SKID_SL_ASSASIN]					= "SL_ASSASIN";
	s_skillIdNameList[SKID_SL_BLACKSMITH]				= "SL_BLACKSMITH";
	s_skillIdNameList[SKID_BS_ADRENALINE2]				= "BS_ADRENALINE2";
	s_skillIdNameList[SKID_SL_HUNTER]					= "SL_HUNTER";
	s_skillIdNameList[SKID_SL_SOULLINKER]				= "SL_SOULLINKER";
	s_skillIdNameList[SKID_SL_KAIZEL]					= "SL_KAIZEL";
	s_skillIdNameList[SKID_SL_KAAHI]					= "SL_KAAHI";
	s_skillIdNameList[SKID_SL_KAUPE]					= "SL_KAUPE";
	s_skillIdNameList[SKID_SL_KAITE]					= "SL_KAITE";
	s_skillIdNameList[SKID_SL_KAINA]					= "SL_KAINA";
	s_skillIdNameList[SKID_SL_STIN]						= "SL_STIN";
	s_skillIdNameList[SKID_SL_STUN]						= "SL_STUN";
	s_skillIdNameList[SKID_SL_SMA]						= "SL_SMA";
	s_skillIdNameList[SKID_SL_SWOO]						= "SL_SWOO";
	s_skillIdNameList[SKID_SL_SKE]						= "SL_SKE";
	s_skillIdNameList[SKID_SL_SKA]						= "SL_SKA";
	s_skillIdNameList[SKID_SL_HIGH]						= "SL_HIGH";
	s_skillIdNameList[SKID_KN_ONEHAND]					= "KN_ONEHAND";

	s_skillIdNameList[SKID_AM_TWILIGHT1]				= "AM_TWILIGHT1";
	s_skillIdNameList[SKID_AM_TWILIGHT2]				= "AM_TWILIGHT2";
	s_skillIdNameList[SKID_AM_TWILIGHT3]				= "AM_TWILIGHT3";

#if defined(MOB_SKILL_PATCH)
	s_skillIdNameList[SKID_NPC_DARKCROSS]				= "NPC_DARKCROSS";
#endif

	s_skillIdNameList[SKID_ST_PRESERVE]					= "ST_PRESERVE";
	s_skillIdNameList[SKID_ST_FULLSTRIP]				= "ST_FULLSTRIP";
	s_skillIdNameList[SKID_WS_WEAPONREFINE]				= "WS_WEAPONREFINE";
	s_skillIdNameList[SKID_CR_SLIMPITCHER]				= "CR_SLIMPITCHER";
	s_skillIdNameList[SKID_CR_FULLPROTECTION]			= "CR_FULLPROTECTION";

	s_skillIdNameList[SKID_ITM_TOMAHAWK]				= "ITM_TOMAHAWK";

	s_skillIdNameList[SKID_PA_SHIELDCHAIN]				= "PA_SHIELDCHAIN";
	s_skillIdNameList[SKID_HP_MANARECHARGE]				= "HP_MANARECHARGE";
	s_skillIdNameList[SKID_PF_DOUBLECASTING]			= "PF_DOUBLECASTING";
	s_skillIdNameList[SKID_HW_GANBANTEIN]				= "HW_GANBANTEIN";
	s_skillIdNameList[SKID_HW_GRAVITATION]				= "HW_GRAVITATION";
	s_skillIdNameList[SKID_WS_CARTTERMINATION]			= "WS_CARTTERMINATION";
	s_skillIdNameList[SKID_WS_OVERTHRUSTMAX]			= "WS_OVERTHRUSTMAX";
	s_skillIdNameList[SKID_CG_LONGINGFREEDOM]			= "CG_LONGINGFREEDOM";
	s_skillIdNameList[SKID_CG_HERMODE]					= "CG_HERMODE";
	s_skillIdNameList[SKID_CG_TAROTCARD]				= "CG_TAROTCARD";
	s_skillIdNameList[SKID_CR_ACIDDEMONSTRATION]		= "CR_ACIDDEMONSTRATION";
	s_skillIdNameList[SKID_CR_CULTIVATION]				= "CR_CULTIVATION";

	s_skillIdNameList[SKID_KN_CHARGEATK]				= "KN_CHARGEATK";
	s_skillIdNameList[SKID_CR_SHRINK]					= "CR_SHRINK";
	s_skillIdNameList[SKID_AS_SONICACCEL]				= "AS_SONICACCEL";
	s_skillIdNameList[SKID_AS_VENOMKNIFE]				= "AS_VENOMKNIFE";
	s_skillIdNameList[SKID_RG_CLOSECONFINE]				= "RG_CLOSECONFINE";
	s_skillIdNameList[SKID_WZ_SIGHTBLASTER]				= "WZ_SIGHTBLASTER";
	s_skillIdNameList[SKID_SA_CREATECON]				= "SA_CREATECON";
	s_skillIdNameList[SKID_HT_PHANTASMIC]				= "HT_PHANTASMIC";
	s_skillIdNameList[SKID_BA_PANGVOICE]				= "BA_PANGVOICE";
	s_skillIdNameList[SKID_DC_WINKCHARM]				= "DC_WINKCHARM";
	s_skillIdNameList[SKID_BS_UNFAIRLYTRICK]			= "BS_UNFAIRLYTRICK";
	s_skillIdNameList[SKID_BS_GREED]					= "BS_GREED";
	s_skillIdNameList[SKID_PR_REDEMPTIO]				= "PR_REDEMPTIO";
	s_skillIdNameList[SKID_MO_KITRANSLATION]			= "MO_KITRANSLATION";
	s_skillIdNameList[SKID_MO_BALKYOUNG]				= "MO_BALKYOUNG";

	s_skillIdNameList[SKID_SA_ELEMENTWATER]				= "SA_ELEMENTWATER";
	s_skillIdNameList[SKID_SA_ELEMENTGROUND]			= "SA_ELEMENTGROUND";
	s_skillIdNameList[SKID_SA_ELEMENTFIRE]				= "SA_ELEMENTFIRE";
	s_skillIdNameList[SKID_SA_ELEMENTWIND]				= "SA_ELEMENTWIND";


	s_skillIdNameList[SKID_GS_GLITTERING]				= "GS_GLITTERING";
	s_skillIdNameList[SKID_GS_FLING]					= "GS_FLING";
	s_skillIdNameList[SKID_GS_TRIPLEACTION]				= "GS_TRIPLEACTION";
	s_skillIdNameList[SKID_GS_BULLSEYE]					= "GS_BULLSEYE";
	s_skillIdNameList[SKID_GS_MADNESSCANCEL]			= "GS_MADNESSCANCEL";

	s_skillIdNameList[SKID_GS_ADJUSTMENT]				= "GS_ADJUSTMENT";
	s_skillIdNameList[SKID_GS_INCREASING]				= "GS_INCREASING";
	s_skillIdNameList[SKID_GS_MAGICALBULLET]			= "GS_MAGICALBULLET";
	s_skillIdNameList[SKID_GS_CRACKER]					= "GS_CRACKER";
	s_skillIdNameList[SKID_GS_SINGLEACTION]				= "GS_SINGLEACTION";

	s_skillIdNameList[SKID_GS_SNAKEEYE]					= "GS_SNAKEEYE";
	s_skillIdNameList[SKID_GS_CHAINACTION]				= "GS_CHAINACTION";
	s_skillIdNameList[SKID_GS_TRACKING]					= "GS_TRACKING";
	s_skillIdNameList[SKID_GS_DISARM]					= "GS_DISARM";
	s_skillIdNameList[SKID_GS_PIERCINGSHOT]				= "GS_PIERCINGSHOT";

	s_skillIdNameList[SKID_GS_RAPIDSHOWER]				= "GS_RAPIDSHOWER";
	s_skillIdNameList[SKID_GS_DESPERADO]				= "GS_DESPERADO";
	s_skillIdNameList[SKID_GS_GATLINGFEVER]				= "GS_GATLINGFEVER";
	s_skillIdNameList[SKID_GS_DUST]						= "GS_DUST";
	s_skillIdNameList[SKID_GS_FULLBUSTER]				= "GS_FULLBUSTER";

	s_skillIdNameList[SKID_GS_SPREADATTACK]				= "GS_SPREADATTACK";
	s_skillIdNameList[SKID_GS_GROUNDDRIFT]				= "GS_GROUNDDRIFT";
	s_skillIdNameList[SKID_NJ_TOBIDOUGU]				= "NJ_TOBIDOUGU";
	s_skillIdNameList[SKID_NJ_SYURIKEN]					= "NJ_SYURIKEN";
	s_skillIdNameList[SKID_NJ_KUNAI]					= "NJ_KUNAI";

	s_skillIdNameList[SKID_NJ_HUUMA]					= "NJ_HUUMA";
	s_skillIdNameList[SKID_NJ_ZENYNAGE]					= "NJ_ZENYNAGE";
	s_skillIdNameList[SKID_NJ_TATAMIGAESHI]				= "NJ_TATAMIGAESHI";
	s_skillIdNameList[SKID_NJ_KASUMIKIRI]				= "NJ_KASUMIKIRI";
	s_skillIdNameList[SKID_NJ_SHADOWJUMP]				= "NJ_SHADOWJUMP";

	s_skillIdNameList[SKID_NJ_KIRIKAGE]					= "NJ_KIRIKAGE";
	s_skillIdNameList[SKID_NJ_UTSUSEMI]					= "NJ_UTSUSEMI";
	s_skillIdNameList[SKID_NJ_BUNSINJYUTSU]				= "NJ_BUNSINJYUTSU";
	s_skillIdNameList[SKID_NJ_NINPOU]					= "NJ_NINPOU";
	s_skillIdNameList[SKID_NJ_KOUENKA]					= "NJ_KOUENKA";

	s_skillIdNameList[SKID_NJ_KAENSIN]					= "NJ_KAENSIN";
	s_skillIdNameList[SKID_NJ_BAKUENRYU]				= "NJ_BAKUENRYU";
	s_skillIdNameList[SKID_NJ_HYOUSENSOU]				= "NJ_HYOUSENSOU";
	s_skillIdNameList[SKID_NJ_SUITON]					= "NJ_SUITON";
	s_skillIdNameList[SKID_NJ_HYOUSYOURAKU]				= "NJ_HYOUSYOURAKU";

	s_skillIdNameList[SKID_NJ_HUUJIN]					= "NJ_HUUJIN";
	s_skillIdNameList[SKID_NJ_RAIGEKISAI]				= "NJ_RAIGEKISAI";
	s_skillIdNameList[SKID_NJ_KAMAITACHI]				= "NJ_KAMAITACHI";
	s_skillIdNameList[SKID_NJ_NEN]						= "NJ_NEN";
	s_skillIdNameList[SKID_NJ_ISSEN]					= "NJ_ISSEN";

	s_skillIdNameList[SKID_MB_FIGHTING]					= "MB_FIGHTING";
	s_skillIdNameList[SKID_MB_NEUTRAL]					= "MB_NEUTRAL";
	s_skillIdNameList[SKID_MB_TAIMING_PUTI]				= "MB_TAIMING_PUTI";
	s_skillIdNameList[SKID_MB_WHITEPOTION]				= "MB_WHITEPOTION";

	s_skillIdNameList[SKID_MB_MENTAL]					= "MB_MENTAL";
	s_skillIdNameList[SKID_MB_CARDPITCHER]				= "MB_CARDPITCHER";
	s_skillIdNameList[SKID_MB_PETPITCHER]				= "MB_PETPITCHER";
	s_skillIdNameList[SKID_MB_BODYSTUDY]				= "MB_BODYSTUDY";
	s_skillIdNameList[SKID_MB_BODYALTER]				= "MB_BODYALTER";

	s_skillIdNameList[SKID_MB_PETMEMORY]				= "MB_PETMEMORY";
	s_skillIdNameList[SKID_MB_M_TELEPORT]				= "MB_M_TELEPORT";
	s_skillIdNameList[SKID_MB_B_GAIN]					= "MB_B_GAIN";
	s_skillIdNameList[SKID_MB_M_GAIN]					= "MB_M_GAIN";
	s_skillIdNameList[SKID_MB_MISSION]					= "MB_MISSION";

	s_skillIdNameList[SKID_MB_MUNAKKNOWLEDGE]			= "MB_MUNAKKNOWLEDGE";
	s_skillIdNameList[SKID_MB_MUNAKBALL]				= "MB_MUNAKBALL";
	s_skillIdNameList[SKID_MB_SCROLL]					= "MB_SCROLL";
	s_skillIdNameList[SKID_MB_B_GATHERING]				= "MB_B_GATHERING";
	s_skillIdNameList[SKID_MB_M_GATHERING]				= "MB_M_GATHERING";

	s_skillIdNameList[SKID_MB_B_EXCLUDE]				= "MB_B_EXCLUDE";
	s_skillIdNameList[SKID_MB_B_DRIFT]					= "MB_B_DRIFT";
	s_skillIdNameList[SKID_MB_B_WALLRUSH]				= "MB_B_WALLRUSH";
	s_skillIdNameList[SKID_MB_M_WALLRUSH]				= "MB_M_WALLRUSH";
	s_skillIdNameList[SKID_MB_B_WALLSHIFT]				= "MB_B_WALLSHIFT";

	s_skillIdNameList[SKID_MB_M_WALLCRASH]				= "MB_M_WALLCRASH";
	s_skillIdNameList[SKID_MB_M_REINCARNATION]			= "MB_M_REINCARNATION";
	s_skillIdNameList[SKID_MB_B_EQUIP]					= "MB_B_EQUIP";

	s_skillIdNameList[SKID_SL_DEATHKNIGHT]				= "SL_DEATHKNIGHT";
	s_skillIdNameList[SKID_SL_COLLECTOR]				= "SL_COLLECTOR";
	s_skillIdNameList[SKID_SL_NINJA]					= "SL_NINJA";
	s_skillIdNameList[SKID_SL_GUNNER]					= "SL_GUNNER";
	s_skillIdNameList[SKID_AM_TWILIGHT4]				= "AM_TWILIGHT4";

	s_skillIdNameList[SKID_DE_PASSIVE]					= "DE_PASSIVE";
	s_skillIdNameList[SKID_DE_PATTACK]					= "DE_PATTACK";
	s_skillIdNameList[SKID_DE_PSPEED]					= "DE_PSPEED";
	s_skillIdNameList[SKID_DE_PDEFENSE]					= "DE_PDEFENSE";
	s_skillIdNameList[SKID_DE_PCRITICAL]				= "DE_PCRITICAL";

	s_skillIdNameList[SKID_DE_PHP]						= "DE_PHP";
	s_skillIdNameList[SKID_DE_PSP]						= "DE_PSP";
	s_skillIdNameList[SKID_DE_RESET]					= "DE_RESET";
	s_skillIdNameList[SKID_DE_RANKING]					= "DE_RANKING";
	s_skillIdNameList[SKID_DE_PTRIPLE]					= "DE_PTRIPLE";

	s_skillIdNameList[SKID_DE_ENERGY]					= "DE_ENERGY";
	s_skillIdNameList[SKID_DE_NIGHTMARE]				= "DE_NIGHTMARE";
	s_skillIdNameList[SKID_DE_SLASH]					= "DE_SLASH";
	s_skillIdNameList[SKID_DE_COIL]						= "DE_COIL";
	s_skillIdNameList[SKID_DE_WAVE]						= "DE_WAVE";

	s_skillIdNameList[SKID_DE_REBIRTH]					= "DE_REBIRTH";
	s_skillIdNameList[SKID_DE_AURA]						= "DE_AURA";
	s_skillIdNameList[SKID_DE_FREEZER]					= "DE_FREEZER";
	s_skillIdNameList[SKID_DE_CHANGEATTACK]				= "DE_CHANGEATTACK";
	s_skillIdNameList[SKID_DE_PUNISH]					= "DE_PUNISH";

	s_skillIdNameList[SKID_DE_POISON]					= "DE_POISON";
	s_skillIdNameList[SKID_DE_INSTANT]					= "DE_INSTANT";
	s_skillIdNameList[SKID_DE_WARNING]					= "DE_WARNING";
	s_skillIdNameList[SKID_DE_RANKEDKNIFE]				= "DE_RANKEDKNIFE";
	s_skillIdNameList[SKID_DE_RANKEDGRADIUS]			= "DE_RANKEDGRADIUS";

	s_skillIdNameList[SKID_DE_GAUGE]					= "DE_GAUGE";
	s_skillIdNameList[SKID_DE_GTIME]					= "DE_GTIME";
	s_skillIdNameList[SKID_DE_GPAIN]					= "DE_GPAIN";
	s_skillIdNameList[SKID_DE_GSKILL]					= "DE_GSKILL";
	s_skillIdNameList[SKID_DE_GKILL]					= "DE_GKILL";

	s_skillIdNameList[SKID_DE_ACCEL]					= "DE_ACCEL";
	s_skillIdNameList[SKID_DE_BLOCKDOUBLE]				= "DE_BLOCKDOUBLE";
	s_skillIdNameList[SKID_DE_BLOCKMELEE]				= "DE_BLOCKMELEE";
	s_skillIdNameList[SKID_DE_BLOCKFAR]					= "DE_BLOCKFAR";
	s_skillIdNameList[SKID_DE_FRONTATTACK]				= "DE_FRONTATTACK";

	s_skillIdNameList[SKID_DE_DANGERATTACK]				= "DE_DANGERATTACK";
	s_skillIdNameList[SKID_DE_TWINATTACK]				= "DE_TWINATTACK";
	s_skillIdNameList[SKID_DE_WINDATTACK]				= "DE_WINDATTACK";
	s_skillIdNameList[SKID_DE_WATERATTACK]				= "DE_WATERATTACK";

	s_skillIdNameList[SKID_DA_ENERGY]					= "DA_ENERGY";
	s_skillIdNameList[SKID_DA_CLOUD]					= "DA_CLOUD";
	s_skillIdNameList[SKID_DA_FIRSTSLOT]				= "DA_FIRSTSLOT";
	s_skillIdNameList[SKID_DA_HEADDEF]					= "DA_HEADDEF";
	s_skillIdNameList[SKID_DA_SPACE]					= "DA_SPACE";

	s_skillIdNameList[SKID_DA_TRANSFORM]				= "DA_TRANSFORM";
	s_skillIdNameList[SKID_DA_EXPLOSION]				= "DA_EXPLOSION";
	s_skillIdNameList[SKID_DA_REWARD]					= "DA_REWARD";
	s_skillIdNameList[SKID_DA_CRUSH]					= "DA_CRUSH";
	s_skillIdNameList[SKID_DA_ITEMREBUILD]				= "DA_ITEMREBUILD";

	s_skillIdNameList[SKID_DA_ILLUSION]					= "DA_ILLUSION";
	s_skillIdNameList[SKID_DA_NUETRALIZE]				= "DA_NUETRALIZE";
	s_skillIdNameList[SKID_DA_RUNNER]					= "DA_RUNNER";
	s_skillIdNameList[SKID_DA_TRANSFER]					= "DA_TRANSFER";
	s_skillIdNameList[SKID_DA_WALL]						= "DA_WALL";

	s_skillIdNameList[SKID_DA_REVENGE]					= "DA_REVENGE";
	s_skillIdNameList[SKID_DA_EARPLUG]					= "DA_EARPLUG";
	s_skillIdNameList[SKID_DA_CONTRACT]					= "DA_CONTRACT";
	s_skillIdNameList[SKID_DA_BLACK]					= "DA_BLACK";

	s_skillIdNameList[SKID_DA_DREAM]					= "DA_DREAM";
	s_skillIdNameList[SKID_DA_MAGICCART]				= "DA_MAGICCART";
	s_skillIdNameList[SKID_DA_COPY]						= "DA_COPY";
	s_skillIdNameList[SKID_DA_CRYSTAL]					= "DA_CRYSTAL";

	s_skillIdNameList[SKID_DA_EXP]						= "DA_EXP";
	s_skillIdNameList[SKID_DA_CARTSWING]				= "DA_CARTSWING";
	s_skillIdNameList[SKID_DA_REBUILD]					= "DA_REBUILD";
	s_skillIdNameList[SKID_DA_JOBCHANGE]				= "DA_JOBCHANGE";
	s_skillIdNameList[SKID_DA_EDARKNESS]				= "DA_EDARKNESS";

	s_skillIdNameList[SKID_DA_EGUARDIAN]				= "DA_EGUARDIAN";
	s_skillIdNameList[SKID_DA_TIMEOUT]					= "DA_TIMEOUT";
	s_skillIdNameList[SKID_ALL_TIMEIN]					= "ALL_TIMEIN";
	s_skillIdNameList[SKID_DA_ZENYRANK]					= "DA_ZENYRANK";
	s_skillIdNameList[SKID_DA_ACCESSORYMIX]				= "DA_ACCESSORYMIX";

	s_skillIdNameList[SKID_DE_BERSERKAIZER]				= "DE_BERSERKAIZER";
	s_skillIdNameList[SKID_DA_DARKPOWER]				= "DA_DARKPOWER";
	s_skillIdNameList[SKID_ALL_INCCARRY]				= "ALL_INCCARRY";
}
