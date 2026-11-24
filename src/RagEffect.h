#ifndef HIGHPRIEST_RAGEFFECT_H
#define HIGHPRIEST_RAGEFFECT_H

#include "base/Util.h"
#include "GameActor.h"
#include "3dlib/Renderer.h"
#include "resource/EZeffect.h"
const int PT_UPDATEFADEOUT	=	0x01;
const int PT2DTEX_SIZEUP			= 0x01;
const	int PT2DTEX_SIZEACCELLIMIT	= 0x02;
const	int PT2DTEX_FIXPOSITION		= 0x04;
const	int PT3DTEX_MOVETEXTUREV	=	0x02;
const	int PT3DTEX_CHECKGROUND		=	0x04;
const	int PT3DTEX_SIZEUP			=	0x08;
const	int PT3DTEX_MOVEDUMMY		=	0x10;
const	int PT3DTEX_WAVERINGLY		=	0x20;
const	int PT3DTEX_SHOWFRONT		=	0x40;
const	int PT3DTEX_GROWUPPER		=	0x80;
const	int PT3DTEX_ACCELARATION	=	0x100;
const	int PT3DTEX_SHOWSIDEFACE	=	0x200;
const	int PT3DTEX_CHKTARGETPOS	=	0x400;
const	int PT3DTEX_RGBCYCLE		=	0x800;
const	int PT_NOWAIT			= 0x01;
const	int PT_SPRINKLE			= 0x02;
const	int PT_CURVE			= 0x04;
const	int PT_BLOW				= 0x08;
const	int PT_ROTATESPR		= 0x20;
const	int PT_TWINKLE			= 0x80;
const	int PT_CHKTARGETPOS		= 0x100;
const	int PT_USEORGARGB		= 0x200;
const	int PT_DELTASPEED		= 0x400;
const	int PT_STOP_AT_LAST		= 0x800;
const	int PT_FILLCIRCLE	= 0x01;
const int PT2D_STOP				=	0x01;
const	int PT_DISAPPEAR	= 0x01;
const	int PTQH_SPEEDLIMIT	= 0x01;
const	int PTQH_RETURNDOWN	= 0x02;
const	int MAX_RENDERPRIM = 512;

enum {
	EF_HIT1,
	EF_HIT2,
	EF_HIT3,
	EF_HIT4,
	EF_HIT5,
	EF_HIT6,
	EF_ENTRY,
	EF_EXIT,
	EF_WARP,
	EF_ENHANCE,
	EF_COIN,
	EF_ENDURE,
	EF_BEGINSPELL,
	EF_GLASSWALL,
	EF_HEALSP,
	EF_SOULSTRIKE,
	EF_BASH,
	EF_MAGNUMBREAK,
	EF_STEAL,
	EF_HIDING,
	EF_PATTACK,
	EF_DETOXICATION,
	EF_SIGHT,
	EF_STONECURSE,
	EF_FIREBALL,
	EF_FIREWALL,
	EF_ICEARROW,
	EF_FROSTDIVER,
	EF_FROSTDIVER2,
	EF_LIGHTBOLT,
	EF_THUNDERSTORM,
	EF_FIREARROW,
	EF_NAPALMBEAT,
	EF_RUWACH,
	EF_TELEPORTATION,
	EF_READYPORTAL,
	EF_PORTAL,
	EF_INCAGILITY,
	EF_DECAGILITY,
	EF_AQUA,
	EF_SIGNUM,
	EF_ANGELUS,
	EF_BLESSING,
	EF_INCAGIDEX,
	EF_SMOKE,
	EF_FIREFLY,
	EF_SANDWIND,
	EF_TORCH,
	EF_SPRAYPOND,
	EF_FIREHIT,
	EF_FIRESPLASHHIT,
	EF_COLDHIT,
	EF_WINDHIT,
	EF_POISONHIT,
	EF_BEGINSPELL2,
	EF_BEGINSPELL3,
	EF_BEGINSPELL4,
	EF_BEGINSPELL5,
	EF_BEGINSPELL6,
	EF_BEGINSPELL7,
	EF_LOCKON,
	EF_WARPZONE,
	EF_SIGHTRASHER,
	EF_BARRIER,
	EF_ARROWSHOT,
	EF_INVENOM,
	EF_CURE,
	EF_PROVOKE,
	EF_MVP,
	EF_SKIDTRAP,
	EF_BRANDISHSPEAR,
	EF_CONE,
	EF_SPHERE,
	EF_BOWLINGBASH,
	EF_ICEWALL,
	EF_GLORIA,
	EF_MAGNIFICAT,
	EF_RESURRECTION,
	EF_RECOVERY,
	EF_EARTHSPIKE,
	EF_SPEARBMR,
	EF_PIERCE,
	EF_TURNUNDEAD,
	EF_SANCTUARY,
	EF_IMPOSITIO,
	EF_LEXAETERNA,
	EF_ASPERSIO,
	EF_LEXDIVINA,
	EF_SUFFRAGIUM,
	EF_STORMGUST,
	EF_LORD,
	EF_BENEDICTIO,
	EF_METEORSTORM,
	EF_YUFITEL,
	EF_YUFITELHIT,
	EF_QUAGMIRE,
	EF_FIREPILLAR,
	EF_FIREPILLARBOMB,
	EF_HASTEUP,
	EF_FLASHER,
	EF_REMOVETRAP,
	EF_REPAIRWEAPON,
	EF_CRASHEARTH,
	EF_PERFECTION,
	EF_MAXPOWER,
	EF_BLASTMINE,
	EF_BLASTMINEBOMB,
	EF_CLAYMORE,
	EF_FREEZING,
	EF_BUBBLE,
	EF_GASPUSH,
	EF_SPRINGTRAP,
	EF_KYRIE,
	EF_MAGNUS,
	EF_BOTTOM,
	EF_BLITZBEAT,
	EF_WATERBALL,
	EF_WATERBALL2,
	EF_FIREIVY,
	EF_DETECTING,
	EF_CLOAKING,
	EF_SONICBLOW,
	EF_SONICBLOWHIT,
	EF_GRIMTOOTH,
	EF_VENOMDUST,
	EF_ENCHANTPOISON,
	EF_POISONREACT,
	EF_POISONREACT2,
	EF_OVERTHRUST,
	EF_SPLASHER,
	EF_TWOHANDQUICKEN,
	EF_AUTOCOUNTER,
	EF_GRIMTOOTHATK,
	EF_FREEZE,
	EF_FREEZED,
	EF_ICECRASH,
	EF_SLOWPOISON,
	EF_BOTTOM2,
	EF_FIREPILLARON,
	EF_SANDMAN,
	EF_REVIVE,
	EF_PNEUMA,
	EF_HEAVENSDRIVE,
	EF_SONICBLOW2,
	EF_BRANDISH2,
	EF_SHOCKWAVE,
	EF_SHOCKWAVEHIT,
	EF_EARTHHIT,
	EF_PIERCESELF,
	EF_BOWLINGSELF,
	EF_SPEARSTABSELF,
	EF_SPEARBMRSELF,
	EF_HOLYHIT,
	EF_CONCENTRATION,
	EF_REFINEOK,
	EF_REFINEFAIL,
	EF_JOBCHANGE,
	EF_LVUP,
	EF_JOBLVUP,
	EF_TOPRANK,
	EF_PARTY,
	EF_RAIN,
	EF_SNOW,
	EF_SAKURA,
	EF_STATUS_STATE,
	EF_BANJJAKII,
	EF_MAKEBLUR,
	EF_TAMINGSUCCESS,
	EF_TAMINGFAILED,
	EF_ENERGYCOAT,
	EF_CARTREVOLUTION,
	EF_VENOMDUST2,
	EF_CHANGEDARK,
	EF_CHANGEFIRE,
	EF_CHANGECOLD,
	EF_CHANGEWIND,
	EF_CHANGEFLAME,
	EF_CHANGEEARTH,
	EF_CHAINGEHOLY,
	EF_CHANGEPOISON,
	EF_HITDARK,
	EF_MENTALBREAK,
	EF_MAGICALATTHIT,
	EF_SUI_EXPLOSION,
	EF_DARKATTACK,
	EF_SUICIDE,
	EF_COMBOATTACK1,
	EF_COMBOATTACK2,
	EF_COMBOATTACK3,
	EF_COMBOATTACK4,
	EF_COMBOATTACK5,
	EF_GUIDEDATTACK,
	EF_POISONATTACK,
	EF_SILENCEATTACK,
	EF_STUNATTACK,
	EF_PETRIFYATTACK,
	EF_CURSEATTACK,
	EF_SLEEPATTACK,
	EF_TELEKHIT,
	EF_PONG,
	EF_LEVEL99,
	EF_LEVEL99_2,
	EF_LEVEL99_3,
	EF_GUMGANG,
	EF_POTION1,
	EF_POTION2,
	EF_POTION3,
	EF_POTION4,
	EF_POTION5,
	EF_POTION6,
	EF_POTION7,
	EF_POTION8,
	EF_DARKBREATH,
	EF_DEFFENDER,
	EF_KEEPING,
	EF_SUMMONSLAVE,
	EF_BLOODDRAIN,
	EF_ENERGYDRAIN,
	EF_POTION_CON,
	EF_POTION_,
	EF_POTION_BERSERK,
	EF_POTIONPILLAR,
	EF_DEFENDER,
	EF_GANBANTEIN,
	EF_WIND,
	EF_VOLCANO,
	EF_GRANDCROSS,
	EF_INTIMIDATE,
	EF_CHOOKGI,
	EF_CLOUD,
	EF_CLOUD2,
	EF_MAPPILLAR,
	EF_LINELINK,
	EF_CLOUD3,
	EF_SPELLBREAKER,
	EF_DISPELL,
	EF_DELUGE,
	EF_VIOLENTGALE,
	EF_LANDPROTECTOR,
	EF_BOTTOM_VO,
	EF_BOTTOM_DE,
	EF_BOTTOM_VI,
	EF_BOTTOM_LA,
	EF_FASTMOVE,
	EF_MAGICROD,
	EF_HOLYCROSS,
	EF_SHIELDCHARGE,
	EF_MAPPILLAR2,
	EF_PROVIDENCE,
	EF_SHIELDBOOMERANG,
	EF_SPEARQUICKEN,
	EF_DEVOTION,
	EF_REFLECTSHIELD,
	EF_ABSORBSPIRITS,
	EF_STEELBODY,
	EF_FLAMELAUNCHER,
	EF_FROSTWEAPON,
	EF_LIGHTNINGLOADER,
	EF_SEISMICWEAPON,
	EF_MAPPILLAR3,
	EF_MAPPILLAR4,
	EF_GUMGANG2,
	EF_TEIHIT1,
	EF_GUMGANG3,
	EF_TEIHIT2,
	EF_TANJI,
	EF_TEIHIT1X,
	EF_CHIMTO,
	EF_STEALCOIN,
	EF_STRIPWEAPON,
	EF_STRIPSHIELD,
	EF_STRIPARMOR,
	EF_STRIPHELM,
	EF_CHAINCOMBO,
	EF_RG_COIN,
	EF_BACKSTAP,
	EF_TEIHIT3,
	EF_BOTTOM_DISSONANCE,
	EF_BOTTOM_LULLABY,
	EF_BOTTOM_RICHMANKIM,
	EF_BOTTOM_ETERNALCHAOS,
	EF_BOTTOM_DRUMBATTLEFIELD,
	EF_BOTTOM_RINGNIBELUNGEN,
	EF_BOTTOM_ROKISWEIL,
	EF_BOTTOM_INTOABYSS,
	EF_BOTTOM_SIEGFRIED,
	EF_BOTTOM_WHISTLE,
	EF_BOTTOM_ASSASSINCROSS,
	EF_BOTTOM_POEMBRAGI,
	EF_BOTTOM_APPLEIDUN,
	EF_BOTTOM_UGLYDANCE,
	EF_BOTTOM_HUMMING,
	EF_BOTTOM_DONTFORGETME,
	EF_BOTTOM_FORTUNEKISS,
	EF_BOTTOM_SERVICEFORYOU,
	EF_TALK_FROSTJOKE,
	EF_TALK_SCREAM,
	EF_POKJUK,
	EF_THROWITEM,
	EF_THROWITEM2,
	EF_CHEMICALPROTECTION,
	EF_POKJUK_SOUND,
	EF_DEMONSTRATION,
	EF_CHEMICAL2,
	EF_TELEPORTATION2,
	EF_PHARMACY_OK,
	EF_PHARMACY_FAIL,
	EF_FORESTLIGHT,
	EF_THROWITEM3,
	EF_FIRSTAID,
	EF_SPRINKLESAND,
	EF_LOUD,
	EF_HEAL,
	EF_HEAL2,
	EF_EXIT2,
	EF_GLASSWALL2,
	EF_READYPORTAL2,
	EF_PORTAL2,
	EF_BOTTOM_MAG,
	EF_BOTTOM_SANC,
	EF_HEAL3,
	EF_WARPZONE2,
	EF_FORESTLIGHT2,
	EF_FORESTLIGHT3,
	EF_FORESTLIGHT4,
	EF_HEAL4,
	EF_FOOT,
	EF_FOOT2,
	EF_BEGINASURA,
	EF_TRIPLEATTACK,
	EF_HITLINE,
	EF_HPTIME,
	EF_SPTIME,
	EF_MAPLE,
	EF_BLIND,
	EF_POISON,
	EF_GUARD,
	EF_JOBLVUP50,
	EF_ANGEL2,
	EF_MAGNUM2,
	EF_CALLZONE,
	EF_PORTAL3,
	EF_COUPLECASTING,
	EF_HEARTCASTING,
	EF_ENTRY2,
	EF_SAINTWING,
	EF_SPHEREWIND,
	EF_COLORPAPER,
	EF_LIGHTSPHERE,
	EF_WATERFALL,
	EF_WATERFALL_90,
	EF_WATERFALL_SMALL,
	EF_WATERFALL_SMALL_90,
	EF_WATERFALL_T2,
	EF_WATERFALL_T2_90,
	EF_WATERFALL_SMALL_T2,
	EF_WATERFALL_SMALL_T2_90,
	EF_MINI_TETRIS,
	EF_GHOST,
	EF_BAT,
	EF_BAT2,
	EF_SOULBREAKER,
	EF_LEVEL99_4,
	EF_VALLENTINE,
	EF_VALLENTINE2,
	EF_PRESSURE,
	EF_BASH3D,
	EF_AURABLADE,
	EF_REDBODY,
	EF_LKCONCENTRATION,
	EF_BOTTOM_GOSPEL,
	EF_ANGEL,
	EF_DEVIL,
	EF_DRAGONSMOKE,
	EF_BOTTOM_BASILICA,
	EF_ASSUMPTIO,
	EF_HITLINE2,
	EF_BASH3D2,
	EF_ENERGYDRAIN2,
	EF_TRANSBLUEBODY,
	EF_MAGICCRASHER,
	EF_LIGHTSPHERE2,
	EF_LIGHTBLADE,
	EF_ENERGYDRAIN3,
	EF_LINELINK2,
	EF_LINKLIGHT,
	EF_TRUESIGHT,
	EF_FALCONASSAULT,
	EF_TRIPLEATTACK2,
	EF_PORTAL4,
	EF_MELTDOWN,
	EF_CARTBOOST,
	EF_REJECTSWORD,
	EF_TRIPLEATTACK3,
	EF_SPHEREWIND2,
	EF_LINELINK3,
	EF_PINKBODY,
	EF_LEVEL99_5,
	EF_LEVEL99_6,
	EF_BASH3D3,
	EF_BASH3D4,
	EF_NAPALMVALCAN,
	EF_PORTAL5,
	EF_MAGICCRASHER2,
	EF_BOTTOM_SPIDER,
	EF_BOTTOM_FOGWALL,
	EF_SOULBURN,
	EF_SOULCHANGE,
	EF_BABY,
	EF_SOULBREAKER2,
	EF_RAINBOW,
	EF_PEONG,
	EF_TANJI2,
	EF_PRESSEDBODY,
	EF_SPINEDBODY,
	EF_KICKEDBODY,
	EF_AIRTEXTURE,
	EF_HITBODY,
	EF_DOUBLEGUMGANG,
	EF_REFLECTBODY,
	EF_BABYBODY,
	EF_BABYBODY2,
	EF_GIANTBODY,
	EF_GIANTBODY2,
	EF_ASURABODY,
	EF_4WAYBODY,
	EF_QUAKEBODY,
	EF_ASURABODY_MONSTER,
	EF_HITLINE3,
	EF_HITLINE4,
	EF_HITLINE5,
	EF_HITLINE6,
	EF_ELECTRIC,
	EF_ELECTRIC2,
	EF_HITLINE7,
	EF_STORMKICK,
	EF_HALFSPHERE,
	EF_ATTACKENERGY,
	EF_ATTACKENERGY2,
	EF_CHEMICAL3,
	EF_ASSUMPTIO2,
	EF_BLUECASTING,
	EF_RUN,
	EF_STOPRUN,
	EF_STOPEFFECT,
	EF_JUMPBODY,
	EF_LANDBODY,
	EF_FOOT3,
	EF_FOOT4,
	EF_TAE_READY,
	EF_GRANDCROSS2,
	EF_SOULSTRIKE2,
	EF_YUFITEL2,
	EF_NPC_STOP,
	EF_DARKCASTING,
	EF_GUMGANGNPC,
	EF_AGIUP,
	EF_JUMPKICK,
	EF_QUAKEBODY2,
	EF_STORMKICK1,
	EF_STORMKICK2,
	EF_STORMKICK3,
	EF_STORMKICK4,
	EF_STORMKICK5,
	EF_STORMKICK6,
	EF_STORMKICK7,
	EF_SPINEDBODY2,
	EF_BEGINASURA1,
	EF_BEGINASURA2,
	EF_BEGINASURA3,
	EF_BEGINASURA4,
	EF_BEGINASURA5,
	EF_BEGINASURA6,
	EF_BEGINASURA7,
	EF_AURABLADE2,
	EF_DEVIL1,
	EF_DEVIL2,
	EF_DEVIL3,
	EF_DEVIL4,
	EF_DEVIL5,
	EF_DEVIL6,
	EF_DEVIL7,
	EF_DEVIL8,
	EF_DEVIL9,
	EF_DEVIL10,
	EF_DOUBLEGUMGANG2,
	EF_DOUBLEGUMGANG3,
	EF_BLACKDEVIL,
	EF_FLOWERCAST,
	EF_FLOWERCAST2,
	EF_FLOWERCAST3,
	EF_MOCHI,
	EF_LAMADAN,
	EF_EDP,
	EF_SHIELDBOOMERANG2,
	EF_RG_COIN2,
	EF_GUARD2,
	EF_SLIM,
	EF_SLIM2,
	EF_SLIM3,
	EF_CHEMICALBODY,
	EF_CASTSPIN,
	EF_PIERCEBODY,
	EF_SOULLINK,
	EF_CHOOKGI2,
	EF_MEMORIZE,
	EF_SOULLIGHT,
	EF_MAPAE,
	EF_ITEMPOKJUK,
	EF_05VAL,
	EF_BEGINASURA11,
	EF_NIGHT,
	EF_CHEMICAL2DASH,
	EF_GROUNDSAMPLE,
	EF_GI_EXPLOSION,
	EF_CLOUD4,
	EF_CLOUD5,
	EF_BOTTOM_HERMODE,
	EF_CARTTER,
	EF_ITEMFAST,
	EF_SHIELDBOOMERANG3,
	EF_DOUBLECASTBODY,
	EF_GRAVITATION,
	EF_TAROTCARD1,
	EF_TAROTCARD2,
	EF_TAROTCARD3,
	EF_TAROTCARD4,
	EF_TAROTCARD5,
	EF_TAROTCARD6,
	EF_TAROTCARD7,
	EF_TAROTCARD8,
	EF_TAROTCARD9,
	EF_TAROTCARD10,
	EF_TAROTCARD11,
	EF_TAROTCARD12,
	EF_TAROTCARD13,
	EF_TAROTCARD14,
	EF_ACIDDEMON,
	EF_GREENBODY,
	EF_THROWITEM4,
	EF_BABYBODY_BACK,
	EF_THROWITEM5,
	EF_BLUEBODY,
	EF_HATED,
	EF_REDLIGHTBODY,
	EF_RO2YEAR,
	EF_SMA_READY,
	EF_STIN,
	EF_RED_HIT,
	EF_BLUE_HIT,
	EF_QUAKEBODY3,
	EF_SMA,
	EF_SMA2,
	EF_STIN2,
	EF_HITTEXTURE,
	EF_STIN3,
	EF_SMA3,
	EF_BLUEFALL,
	EF_BLUEFALL_90,
	EF_FASTBLUEFALL,
	EF_FASTBLUEFALL_90,
	EF_BIG_PORTAL,
	EF_BIG_PORTAL2,
	EF_SCREEN_QUAKE,
	EF_HOMUNCASTING,
	EF_HFLIMOON1,
	EF_HFLIMOON2,
	EF_HFLIMOON3,
	EF_HO_UP,
	EF_HAMIDEFENCE,
	EF_HAMICASTLE,
	EF_HAMIBLOOD,
	EF_HATED2,
	EF_TWILIGHT1,
	EF_TWILIGHT2,
	EF_TWILIGHT3,
	EF_ITEM_THUNDER,
	EF_ITEM_CLOUD,
	EF_ITEM_CURSE,
	EF_ITEM_ZZZ,
	EF_ITEM_RAIN,
	EF_ITEM_LIGHT,
	EF_ANGEL3,
	EF_M01,
	EF_M02,
	EF_M03,
	EF_M04,
	EF_M05,
	EF_M06,
	EF_M07,
	EF_KAIZEL,
	EF_KAAHI,
	EF_CLOUD6,
	EF_FOOD01,
	EF_FOOD02,
	EF_FOOD03,
	EF_FOOD04,
	EF_FOOD05,
	EF_FOOD06,
	EF_SHRINK,
	EF_THROWITEM6,
	EF_SIGHT2,
	EF_QUAKEBODY4,
	EF_FIREHIT2,
	EF_NPC_STOP2,
	EF_NPC_STOP2_DEL,
	EF_FVOICE,
	EF_WINK,
	EF_COOKING_OK,
	EF_COOKING_FAIL,
	EF_TEMP_OK,
	EF_TEMP_FAIL,
	EF_HAPGYEOK,
	EF_THROWITEM7,
	EF_THROWITEM8,
	EF_THROWITEM9,
	EF_THROWITEM10,
	EF_BUNSINJYUTSU,
	EF_KOUENKA,
	EF_HYOUSENSOU,
	EF_BOTTOM_SUITON,
	EF_STIN4,
	EF_THUNDERSTORM2,
	EF_CHEMICAL4,
	EF_STIN5,
	EF_MADNESS_BLUE,
	EF_MADNESS_RED,
	EF_RG_COIN3,
	EF_BASH3D5,
	EF_CHOOKGI3,
	EF_KIRIKAGE,
	EF_TATAMI,
	EF_KASUMIKIRI,
	EF_ISSEN,
	EF_KAEN,
	EF_BAKU,
	EF_HYOUSYOURAKU,
	EF_DESPERADO,
	EF_LIGHTNING_S,
	EF_BLIND_S,
	EF_POISON_S,
	EF_FREEZING_S,
	EF_FLARE_S,
	EF_RAPIDSHOWER,
	EF_MAGICALBULLET,
	EF_SPREADATTACK,
	EF_TRACKCASTING,
	EF_TRACKING,
	EF_TRIPLEACTION,
	EF_BULLSEYE,
	EF_MAP_MAGICZONE,
	EF_MAP_MAGICZONE2,
	EF_DAMAGE1,
	EF_DAMAGE1_2,
	EF_DAMAGE1_3,
	EF_UNDEADBODY,
	EF_UNDEADBODY_DEL,
	EF_GREEN_NUMBER,
	EF_BLUE_NUMBER,
	EF_RED_NUMBER,
	EF_PURPLE_NUMBER,
	EF_BLACK_NUMBER,
	EF_WHITE_NUMBER,
	EF_YELLOW_NUMBER,
	EF_PINK_NUMBER,
	EF_BUBBLE_DROP,
	EF_NPC_EARTHQUAKE,
	EF_DA_SPACE,
	EF_DRAGONFEAR,
	EF_BLEEDING,
	EF_WIDECONFUSE,
	EF_BOTTOM_RUNNER,
	EF_BOTTOM_TRANSFER,
	EF_CRYSTAL_BLUE,
	EF_BOTTOM_EVILLAND,
	EF_GUARD3,
	EF_NPC_SLOWCAST,
	EF_CRITICALWOUND,
	EF_GREEN99_3,
	EF_GREEN99_5,
	EF_GREEN99_6,
	EF_MAPSPHERE,
	EF_POK_LOVE,
	EF_POK_WHITE,
	EF_POK_VALEN,
	EF_POK_BIRTH,
	EF_POK_CHRISTMAS,
	EF_MAP_MAGICZONE3,
	EF_MAP_MAGICZONE4,
	EF_DUST,
	EF_TORCH_RED,
	EF_TORCH_GREEN,
	EF_MAP_GHOST,
	EF_GLOW1,
	EF_GLOW2,
	EF_GLOW4,
	EF_TORCH_PURPLE,
	EF_CLOUD7,
	EF_CLOUD8,
	EF_FLOWERLEAF,
	EF_MAPSPHERE2,
	EF_GLOW11,
	EF_GLOW12,
	EF_BACKTEXTURE1,
	EF_HP_DAMAGE,
	EF_BLESSBODY1,
	EF_BLESSBODY2,
	EF_BLESSBODY3,
	EF_POKGIBODY1,
	EF_TEST,
};
enum EFFECTPRIMID {
	PP_2DTEXTURE = 0,
	PP_2DTEXTUREORBIT = 1,
	PP_2DQUAD = 2,
	PP_2DFLASH = 3,
	PP_2DCIRCLE = 4,
	PP_3DPARTICLE = 5,
	PP_3DPARTICLEGRAVITY = 6,
	PP_3DPARTICLEORBIT = 7,
	PP_3DPARTICLESPLINE = 8,
	PP_3DQUADHORN = 9,
	PP_3DSPLINE = 10,
	PP_3DSPIRAL = 11,
	PP_3DCIRCLE = 12,
	PP_3DCYLINDER = 13,
	PP_3DSPHERE = 14,
	PP_3DTEXTURE = 15,
	PP_3DCROSSTEXTURE = 16,
	PP_3DSHINELIGHT = 17,
	PP_3DCYLINDERHALF = 18,
	PP_3DFLASH = 19,
	PP_3DLINE = 20,
	PP_3DPARTICLE_NOMASTER = 21,
	PP_3DTEXTURE_NOMASTER = 22,
	PP_3DCASTING = 23,
	PP_3DCASTING_2 = 24,
	PP_3DCASTING_3 = 25,
	PP_MAPMAGICZONE = 26,
	PP_MAPPARTICLE = 27,
	PP_3DAURA = 28,
	PP_3DAURA_2 = 29,
	PP_GI_1 = 30,
	PP_GI_2 = 31,
	PP_CHOOKGI = 32,
	PP_CLOUD = 33,
	PP_GI_3 = 34,
	PP_GUMGANG = 35,
	PP_GI_4 = 36,
	PP_GRANDCROSS2 = 37,
	PP_LINELINK = 38,
	PP_LANDPROTECTOR = 39,
	PP_SAKURA = 40,
	PP_MAPPILLAR = 41,
	PP_SHIELDBOOMERANG = 42,
	PP_INTIMIDATE = 43,
	PP_TEIHIT1 = 44,
	PP_TEIHIT2 = 45,
	PP_CHIMTO = 46,
	PP_BOTTOM = 47,
	PP_BOTTOM2 = 48,
	PP_BOTTOMOUT = 49,
	PP_GI_5 = 50,
	PP_BOTTOMSPR = 51,
	PP_POKJUK = 52,
	PP_THROWITEM = 53,
	PP_CHEMICALPROTECTION = 54,
	PP_CHEMICAL2 = 55,
	PP_CHEMICAL3 = 56,
	PP_TELEPORTATION2 = 57,
	PP_FIRSTAID = 58,
	PP_PARTICLE_UP = 59,
	PP_HEAL = 60,
	PP_GUARD = 61,
	PP_WIND = 62,
	PP_PORTAL = 63,
	PP_RECT_UP = 64,
	PP_3DCASTING_4 = 65,
	PP_FOREST = 66,
	PP_FOOT = 67,
	PP_ASURACASTING = 68,
	PP_TRIPLEATTACK = 69,
	PP_HITLINE = 70,
	PP_BLIND = 71,
	PP_POISON = 72,
	PP_MAGNUM2 = 73,
	PP_CALLZONE = 74,
	PP_SLASH1 = 75,
	PP_SAINTWING = 76,
	PP_SPHEREWIND = 77,
	PP_COLORPAPER = 78,
	PP_WATERFALL = 79,
	PP_WATERFALLPARTICLE = 80,
	PP_LIGHTSPHERE = 81,
	PP_EFFECTTEXTURE = 82,
	PP_MINI_TETRIS = 83,
	PP_GHOST = 84,
	PP_SUPERANGEL = 85,
	PP_SOULBREAKER = 86,
	PP_PRESSURE = 87,
	PP_GROUNDSHAKE = 88,
	PP_BASH3D = 89,
	PP_RECT_UP2 = 90,
	PP_SPHEREWIND2 = 91,
	PP_SOULBREAKER2 = 92,
	PP_RAINBOW = 93,
	PP_PEONG = 94,
	PP_EFFECTTEXTURE2 = 95,
	PP_PEONGUP = 96,
	PP_TANJI2 = 97,
	PP_AIRTEXTURE = 98,
	PP_DOUBLEGUMGANG = 99,
	PP_HITLINE3 = 100,
	PP_ELECTRIC = 101,
	PP_ELECTRIC2 = 102,
	PP_STORMKICK = 103,
	PP_HALFSPHERE = 104,
	PP_ATTACKENERGY = 105,
	PP_ATTACKENERGY2 = 106,
	PP_3DCASTING_5 = 107,
	PP_FLOWERCAST = 108,
	PP_SOULLINK = 109,
	PP_HEAL_LOW = 110,
	PP_SOULPARTICLE = 111,
	PP_GROUNDSAMPLE = 112,
	PP_GI_EXPLOSION = 113,
	PP_HERMODE = 114,
	PP_3DAURA_3 = 115,
	PP_SHIELDBOOMERANG3 = 116,
	PP_3DQUADHORN2 = 117,
	PP_ACIDDEMON = 118,
	PP_STIN = 119,
	PP_SMA = 120,
	PP_SMA2 = 121,
	PP_STIN2 = 122,
	PP_TWILIGHT = 123,
	PP_KOUENKA = 124,
	PP_STIN4 = 125,
	PP_STIN5 = 126,
	PP_BUBBLE_DROP = 127,
	PP_EFFECTTEXTURE_ANI = 128,
};

class CEffectPrim;
class CRagEffect;

struct PrimSegment {
public:
	vector3d pos;
	vector3d segPos[4];
	float radius;
	float size;
	float longitude;
	matrix mat;
	int red;
	int green;
	int blue;
	float alpha;
	unsigned int argb;
};

struct objectSegment {
public:
	vector3d pos;
	vector3d deltaPos;
	float longitude;
	float latitude;
	matrix mat;
};

class CRagEffect : public CRenderObject {
public:
	CRagEffect();
	virtual ~CRagEffect();
	virtual bool OnProcess();
	virtual bool OnProcess2();
	virtual bool OnProcess3();
	virtual void SendMsg(CGameObject* sender, int msgid, INT_PTR val1 = 0, INT_PTR val2 = 0, INT_PTR val3 = 0);
	virtual void Render(matrix& wtm);
	void Init(CRenderObject* master, int effectId, vector3d fDeltaPos);
	CEffectPrim* LaunchEffectPrim(EFFECTPRIMID effectPrimId, vector3d fDeltaPos = vector3d(0.0f, 0.0f, 0.0f));
	int GetType() { return m_type; }
	int GetDuration() { return m_duration; }
	int GetRagStateCnt() { return m_stateCnt; }
	void SetDuration(int option) { m_duration = option; }
protected:
	void LoadEzEffect(char* fName);
	void InitEZ2STRFrame();
	bool ProcessEZ2STR(float x = 0, float y = -80, float z = 0);
	void RenderAniClip(KAC_LAYER& pLayer, const KAC_XFORMDATA* pXformData, int rpCnt, matrix& wtm);
private:
	void BeginCasting(char* tName, char option);
	void BeginCasting2(char* tName, char option);
	void FlowerCasting(char* tName, char option, char F1);
	void Test();
	void LockOn();
	void ColdHit();
	void TopRank();
	void WarpZone();
	void PoisonHit();
	void BlueCasting();
	void DarkCasting();
	void AuraBladeCasting();
	void BeginSpell7();
	void BeginSpell6();
	void BeginSpell5();
	void BeginSpell4();
	void BeginSpell3();
	void BeginSpell2();
	void FireSplashHit();
	bool Signum();
	bool Angelus();
	bool Mvp();
	bool Cure();
	bool Lord();
	bool Kyrie();
	bool Freeze();
	bool Magnus();
	bool Gloria();
	bool Freezed();
	bool Invenom();
	bool Provoke();
	bool FireHit();
	bool RefineOk();
	bool MaxPower();
	bool Quagmire();
	bool SkidTrap();
	bool Recovery();
	bool Aspersio();
	bool Claymore();
	bool Freezing();
	bool Splasher();
	bool ShockWave();
	bool Sanctuary();
	bool Impositio();
	bool Coin();
	bool FireWall();
	bool GlassWall();
	bool LightBolt();
	bool StoneCurse();
	bool ThunderStorm();
	void ThunderStorm2();
	bool ShieldCharge();
	bool JobLvUp50(int time);
	bool JobLvUp50_2(int time);
	bool RunStop(int time);
	bool LexDivina();
	bool StormGust();
	bool VenomDust();
	bool SpringTrap();
	bool CrashEarth();
	bool Perfection();
	bool FirePillar();
	bool Benedictio();
	bool Suffragium();
	bool LexAeterna();
	bool Magnificat();
	bool RefineFail();
	bool AutoCounter();
	bool MeteorStorm();
	bool PoisonReact2();
	bool RepairWeapon();
	bool Resurrection();
	bool Concentration();
	bool BlastMineBomb();
	bool FirePillarBomb();
	bool TwoHandQuicken();
	unsigned char EnergyCoat();
	bool CartRevolution();
	bool EZ2STROnlyEffect();
	void PotionPillar(float speed = 1.0f, int duration = 50);
	bool Potion_Con();
	bool Potion_();
	bool Potion_Berserk();
	void Snow();
	void Torch();
	void Bubble_Drop();
	void Sakura();
	void Maple();
	void Portal();
	void FireFly();
	void SandWind();
	void SprayPond();
	void Banjjakii();
	void Cloud(char F1);
	void PokJuk();
	void PokJuk_Sound();
	void Hit1();
	void Hit2(int option);
	void Hit3();
	void Hit4();
	void Hit5();
	void Hit6();
	void Warp();
	void Exit();
	void Bash();
	void Aqua();
	void Vallentine(char F1);
	void Effect_SPR(char F1);
	void Party();
	void Entry();
	void Steal();
	void Sight();
	void Sight2();
	void Smoke();
	void Hiding();
	void HealSP();
	void Endure();
	void Ruwach();
	void Enhance();
	void MakeBlur();
	void MakeBlur2();
	void Blessing();
	void FireBall();
	void IceArrow();
	void IncAGIDEX();
	void FireArrow();
	void VenomDust2();
	void NapalmBeat();
	void BeginSpell();
	void SoulStrike(char F1);
	void FrostDiver();
	void IncAgility();
	void CartBoost();
	void DecAgility();
	void TetraVoltex();
	void FrostDiver2();
	void StatusState();
	void MagnumBreak();
	void ReadyPortal();
	void Detoxication();
	void Teleportation();
	void DragonSmoke();
	void Cone();
	void Sphere();
	void Pierce();
	void Bottom();
	void Revive();
	void Bottom2();
	void Flasher();
	void IceWall();
	void Barrier();
	void Yufitel(char F1);
	void Gravitation();
	void TurnUndead();
	void SlowPoison();
	void BowlingBash();
	void Sightrasher();
	void FirePillarOn();
	void HeavensDrive();
	void SonicBlowHit();
	void GrimToothAtk();
	void EnchantPoison();
	void EnchantPoison2();
	void HasteUp();
	void FireIvy();
	void SpearBmr();
	void Cloaking();
	void BlastMine();
	void GrimTooth();
	void Detecting();
	void BlitzBeat();
	void SonicBlow();
	void WaterBall();
	void WaterBall2();
	void OverThrust();
	void RemoveTrap();
	void YufitelHit(char F1);
	void EarthSpike(char F1);
	void HitDark();
	void ChangeDark();
	void ChangeFire();
	void ChangeCold();
	void ChangeWind();
	void ChangeFlame();
	void ChangeEarth();
	void ChangeHoly();
	void ChangePoison();
	void CurseAttack();
	void DarkBreath();
	void SummonSlave();
	void BloodDrain();
	void EnergyDrain();
	void EnergyDrain2();
	void EnergyDrain3();
	bool Suicide();
	bool SuiExplosion();
	void Kouenka();
	void Map_MagicZone(char* tName, char option);
	void Map_Aura(char* fName);
	void Map_Pika(char* fName);
	void Map_Particle(char* tName, char option, char F1 = 0);
	void Level99(char* tName, char option);
	void Level99_2(char* tName, char option);
	void Level99_3(char F1);
	void SoulParticle(char* fName);
	void SoulLinkLight(char* fName);
	void WaterFallParticle(char F1, char time);
	void GumGang(char F1, char time);
	void DoubleGumGang(char F1, char time);
	void SMA2(char* fName);
	void DEFENDER(char* fName);
	void SAINTCASTING(short time, char* tName, char F1);
	void CoupleCasting(char* fName);
	void HeartCasting(char* tName, float x, float z);
	void WIND();
	void VOLCANO(char* tName, char option);
	void VOLCANO2(char* tName, char option);
	void GRANDCROSS(short time, char F1);
	void GRANDCROSS2(char F1);
	void RECT_UP();
	void INTIMIDATE();
	void RG_COIN(char* fName);
	void RG_COIN2(char* tName, float size);
	void PRESSURE(char* tName, char option);
	void BASH3D(char* tName, char option, char F1);
	void GroundShake(char* tName, int minus, char F1 = 0);
	void TEIHIT1(char* tName, char option);
	void TEIHIT2();
	void TEIHIT3(char* tName, char option);
	void TANJI();
	void TANJI2();
	void CHIMTO(char F1);
	void TRIPLEATTACK();
	void TRIPLEATTACK2();
	void TRIPLEATTACK3();
	void Demonstration();
	void NpcStop(char F1);
	void CHEMICALPROTECTION(short time);
	void CHEMICAL2(short time);
	void CHEMICAL3(short time);
	void GROUNDSAMPLE();
	void GI_EXPLOSION(char* fName);
	void CALLZONE(char* fName);
	void BLIND(char F1);
	void POISON(char F1);
	void TELEPORTATION2(char* tName, short time);
	void PARTICLE_UP();
	void Twilight(char F1);
	void CHOOKGI(char F1);
	void MAPPILLAR(char* tName, char option);
	void LINELINK();
	void LINELINK2();
	void LINELINK3();
	void Bottom_Volcano(char* tName, char PW = 0, float x = 0, float z = 0);
	void Bottom_Light(char* tName, char option);
	void Bottom_LandProtector(char* tName, char option);
	void Bottom_Hermode(char* fName);
	void Bottom_Music(char* tName, char option);
	void EffectTextureSet(char* tName, char option);
	void EffectTextureSet_Animation(char F1);
	void PeongMiddle(char* fName);
	void PeongUp(char* tName, char option);
	void Peong(char* tName, short time);
	void Rainbow(char* tName, short time);
	void Bottom_Vertical(char* tName, char option);
	void Bottom_Out(char* tName, char option);
	void Bottom_Spr();
	void MagicRod();
	void BeginAsura(char F1);
	void BeginAsura1(char* fName);
	void BeginAsura2(char F1);
	void HitLine(char F1);
	void HitLine3(char F1);
	void HitLine4(char F1);
	void HitLine5(char F1);
	void Electric(char F1, short F2);
	void Electric2(char F1, short F2);
	void Providence(char* tName, char option);
	void ForestLight(char* tName, char option);
	void WaterFall(char* tName, char F1, char F2, char F3);
	void BlueFall(char F1, char F2, char F3);
	void LightSphere(char* fName);
	void ShieldBoomerang(char* fName);
	void ShieldBoomerang3(char* fName);
	void FirstAid();
	void AirTexture(char* fName);
	void ThrowItem(char* tName, char option);
	void HealTime(char* tName, char option);
	void ParticleTime(char* tName, char option);
	void ParticlePath(char* tName, char option);
	void Heal(char* tName, char option);
	void Heal2(char* tName, char option);
	void Exit2(char* fName);
	void Guard(char* tName, char option);
	void Guard2(char* fName);
	void HalfSphere(char* fName);
	void AttackEnergy(char* fName);
	void AttackEnergy2(char* fName);
	void SaintWing(char* fName);
	void SphereWind(char* tName, int minus, char F1);
	void SphereWind2(char* tName, int minus, char F1);
	void ParticleSpread(char* tName, char option = 0);
	bool GlassWall2(char* tName);
	void Bottom_Magnus(char* tName, char option);
	void Basilica(char* tName, char option);
	void ReadyPortal2();
	void Portal2(char F1);
	void Portal3(char F1 = 0);
	void PortalWind(char F1);
	void WarpZone2(char* fName);
	void BLACKDEVIL(char* fName);
	void Magnum2(char* fName);
	void Entry2();
	void ColorPaper(char* fName);
	void Foot(char* tName, char option);
	void StormKick(char* tName, char option);
	void Ghost(char F1);
	void SuperAngel(char F1, char time = 0);
	void SoulBreaker();
	void SoulBreaker2(int option);
	void STIN(char* fName);
	void STIN5(char* fName);
	void STIN2(char* tName, char option);
	void STIN4(char* tName, char option);
	void SMA(char F1 = 0);
	void Talk_FrostJoke(char F1);
private:
	friend class CEffectPrim;
	CRenderObject* m_master;
	float m_param[4];
	float m_longitude;
	float m_latitude;
	float m_tlvertX;
	float m_tlvertY;
	float m_tlvertZ;
	int m_defZ;
	int m_isDirection;
	int m_type;
	int m_level;
	int m_flag;
	int m_count;
	int m_stateCnt;
	int m_duration;
	vector3d m_deltaPos;
	vector3d m_deltaPos2;
	vector3d m_deltaPos3;
	std::list<CEffectPrim *> m_primList;
	objectSegment m_objSeg[20];
	CEZeffectRes* m_ezEffectRes;
	KANICLIP* m_aniClips;
	KAC_XFORMDATA m_actXformData[128];
	RPTriFace m_rp[512];
	int m_iCurLayer;
	int m_cEndLayer;
	unsigned char m_isLayerDrawn[128];
	int m_aiCurAniKey[128];
};
extern std::vector<int> stateId;

inline tlvertex3d TransformVert(const matrix& wtm, vector3d vec)
{
	tlvertex3d tlvert;

	g_renderer->ProjectVertex(vec, wtm, &tlvert);

	return tlvert;
}

#endif
