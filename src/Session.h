#ifndef HIGHPRIEST_SESSION_H
#define HIGHPRIEST_SESSION_H

#include "ItemInfo.h"
#include "SkillInfo.h"
#include "base/xml.h"
#include "base/File.h"
#include "packet.h"

enum {
	ATTR_NORMAL,
	ATTR_WATER,
	ATTR_EARTH,
	ATTR_FIRE,
	ATTR_WIND,
	ATTR_POISON,
};
enum TALKTYPE {
	TT_NORMAL = 0,
	TT_BROADCAST = 1,
	TT_STAT = 2,
	TT_UNKNOWN = 3,
	TT_DISCONNECT_CHARACTER = 4,
	TT_DISCONNECT_ALL_CHARACTER = 5,
	TT_REQ_NUM_USER = 6,
	TT_MUSIC_ON_OFF = 7,
	TT_EFFECT_ON_OFF = 8,
	TT_MAP_POS = 9,
	TT_FRAMESKIP_ON_OFF = 10,
	TT_EFFECT_SOUND_ON_OFF = 11,
	TT_VOLUME = 12,
	TT_BVOLUME = 13,
	TT_HELP = 14,
	TT_REQ_WHISPER_LIST = 15,
	TT_REQ_WHISPER_STATE_EX = 16,
	TT_REQ_WHISPER_STATE_IN = 17,
	TT_REQ_WHISPER_PC_EX = 18,
	TT_REQ_WHISPER_PC_IN = 19,
	TT_IS_REDCELL = 20,
	TT_REQ_STAND = 21,
	TT_REQ_SIT = 22,
	TT_MAKE_CHATROOMMAKEWND = 23,
	TT_REQ_EXIT_ROOM = 24,
	TT_REQ_EXCHANGE_ITEM = 25,
	TT_REQ_MAKE_GROUP = 26,
	TT_REQ_LEAVE_GROUP = 27,
	TT_REQ_EXPEL_GROUP_MEMBER = 28,
	TT_REQ_EMOTION_SURPRISE = 29,
	TT_REQ_EMOTION_QUESTION = 30,
	TT_REQ_EMOTION_DELIGHT = 31,
	TT_REQ_EMOTION_THROB = 32,
	TT_REQ_EMOTION_SWEAT = 33,
	TT_REQ_EMOTION_AHA = 34,
	TT_REQ_EMOTION_FRET = 35,
	TT_REQ_EMOTION_ANGER = 36,
	TT_REQ_EMOTION_MONEY = 37,
	TT_REQ_EMOTION_THINK = 38,
	TT_REQ_EMOTION_SCISSOR = 39,
	TT_REQ_EMOTION_ROCK = 40,
	TT_REQ_EMOTION_WRAP = 41,
	TT_OPEN_TIP = 42,
	TT_REMEMBER_WARPPOINT = 43,
	TT_REQ_PK = 44,
	TT_SKILL_SNAP_ON_OFF = 45,
	TT_ATTACK_SNAP_ON_OFF = 46,
	TT_ITEM_SNAP_ON_OFF = 47,
	TT_REQ_ITEM_CREATE = 48,
	TT_REQ_MOVETO_MAP = 49,
	TT_NORMAL_FROM_CHATROOM = 50,
	TT_REQ_SAVE_CHAT = 51,
	TT_REQ_SAVE_CHAT_FROM_CHATROOM = 52,
	TT_MISS_EFFECT_ON_OFF = 53,
	TT_FIXED_CAMERA_ON_OFF = 54,
	TT_REQ_MAKE_GUILD = 55,
	TT_REQ_BREAK_GUILD = 56,
	TT_GUILD_CHAT = 57,
	TT_TEST_SHOW_IMAGE = 58,
	TT_TESTPACKET = 59,
	TT_FOG = 60,
	TT_REQ_RESETSTATE = 61,
	TT_REQ_RESETSKILL = 62,
	TT_CHANGE_MAPTYPE = 63,
	TT_TOGGLE_EMBLEM = 64,
	TT_TOGGLE_OBJLIGHT = 65,
	TT_REQ_EMOTION_THANKS = 66,
	TT_REQ_EMOTION_KEK = 67,
	TT_REQ_EMOTION_SORRY = 68,
	TT_REQ_EMOTION_SMILE = 69,
	TT_REQ_EMOTION_PROFUSELY_SWEAT = 70,
	TT_REQ_EMOTION_SCRATCH = 71,
	TT_REQ_EMOTION_BEST = 72,
	TT_REQ_EMOTION_STARE_ABOUT = 73,
	TT_REQ_EMOTION_HUK = 74,
	TT_REQ_EMOTION_O = 75,
	TT_REQ_EMOTION_X = 76,
	TT_REQ_EMOTION_HELP = 77,
	TT_REQ_EMOTION_GO = 78,
	TT_REQ_EMOTION_BIGTHROB = 79,
	TT_REQ_EMOTION_CRY = 80,
	TT_REQ_EMOTION_KIK = 81,
	TT_REQ_EMOTION_CHUP = 82,
	TT_REQ_EMOTION_CHUPCHUP = 83,
	TT_REQ_EMOTION_HNG = 84,
	TT_REQ_EMOTION_OK = 85,
	TT_SHOW_EMOTIONLIST = 86,
	TT_LOCALBROADCAST = 87,
	TT_HIDE = 88,
	TT_NAME_BALLOON = 89,
	TT_NAMELESS_BROADCAST = 90,
	TT_NAMELESS_LOCALBROADCAST = 91,
	TT_FONT = 92,
	TT_SAVEMONEY = 93,
	TT_LOADMONEY = 94,
	TT_REMOVE = 95,
	TT_SHIFT = 96,
	TT_RECALL_AID = 97,
	TT_RECALL_GID = 98,
	TT_FORCING_MOVE_AID = 99,
	TT_FORCING_MOVE_GID = 100,
	TT_BILLING_INFO = 101,
	TT_NAMELESS_BLUEBROADCAST = 102,
	TT_NAMELESS_BLUELOCALBROADCAST = 103,
	TT_REQ_AUTO_SAVE_CHAT = 104,
	TT_NOSHIFT_ON_OFF = 105,
	TT_AURA_ON_OFF = 106,
	TT_SHOW_NAME_TYPE = 107,
	TT_NOCTRL_ON_OFF = 108,
	TT_DORIDORI_ON_OFF = 109,
	TT_BINGBING_ON_OFF = 110,
	TT_BANGBANG_ON_OFF = 111,
	TT_SKILLFAIL_ON_OFF = 112,
	TT_NOTALKMSG_ON_OFF = 113,
	TT_SET1_ON_OFF = 114,
	TT_REQ_REPORT_CHAT = 115,
	TT_REQ_REPORT_CHAT_FROM_CHATROOM = 116,
	TT_CLIPMOUSE = 117,
	TT_LIGHTMAP = 118,
	TT_QUICKSPELL_ON_OFF = 119,
	TT_QUICKSPELL2_ON_OFF = 120,
	TT_QUICKSPELL3_ON_OFF = 121,
	TT_REQ_EMOTION_STARE = 122,
	TT_REQ_EMOTION_HUNGRY = 123,
	TT_REQ_EMOTION_COOL = 124,
	TT_REQ_EMOTION_MERONG = 125,
	TT_REQ_EMOTION_SHY = 126,
	TT_REQ_EMOTION_GOODBOY = 127,
	TT_REQ_EMOTION_SPTIME = 128,
	TT_REQ_EMOTION_SEXY = 129,
	TT_REQ_EMOTION_COMEON = 130,
	TT_REQ_EMOTION_SLEEPY = 131,
	TT_REQ_EMOTION_CONGRATULATION = 132,
	TT_REQ_EMOTION_HPTIME = 133,
	TT_BATTLE_ON_OFF = 134,
	TT_NOTRADE_ON_OFF = 135,
	TT_SIYEON_STATUS = 136,
	TT_SET2_ON_OFF = 137,
	TT_SYSTEM_CHECK_ON_OFF = 138,
	TT_FRAME_ON_OFF = 139,
	TT_WINDOW_ON_OFF = 140,
	TT_SHOW_PVP_INFO = 141,
	TT_CHECK_ID = 142,
	TT_RC_ID = 143,
	TT_RANDOM_LOADING_SCREEN_ON_OFF = 144,
	TT_REQ_BLACKSMITH = 145,
	TT_REQ_ALCHEMIST = 146,
	TT_NOTALKMSG2_ON_OFF = 147,
	TT_LEVEL = 148,
	TT_SP = 149,
	TT_HI = 150,
	TT_REQ_TAEKWON = 151,
	TT_AUTO_MSG_ON_OFF = 152,
	TT_REQ_EMOTION_SPARK = 153,
	TT_REQ_EMOTION_CONFUSE = 154,
	TT_REQ_EMOTION_OHNO = 155,
	TT_REQ_EMOTION_HUM = 156,
	TT_REQ_EMOTION_BLABLA = 157,
	TT_REQ_EMOTION_OTL = 158,
	TT_REQ_EMOTION_DICE = 159,
	TT_REQ_KILLER_RANK = 160,
	TT_TRACEAI_ON_OFF = 161,
	TT_HOMUN_WAIT = 162,
	TT_HOMUN_USERAI_ON_OFF = 163,
	TT_LOGINOUT_ON_OFF = 164,
	TT_SET3_ON_OFF = 165,
	TT_SHOPPING_ON_OFF = 166,
	TT_REQ_MUNAK = 167,
	TT_REQ_HUNTING = 168,
	TT_REQ_DEATHKNIGHT = 169,
	TT_REQ_EMOTION_LUV = 170,
	TT_REQ_COLLECTOR = 171,
	TT_MINIGAME_ON_OFF = 172,
	TT_MER_USERAI_ON_OFF = 173,
	TT_REQ_EMOTION_MOBILE = 174,
	TT_REQ_EMOTION_MAIL = 175,
	TT_REQ_EMOTION_ANTENNA0 = 176,
	TT_REQ_EMOTION_ANTENNA1 = 177,
	TT_REQ_EMOTION_ANTENNA2 = 178,
	TT_REQ_EMOTION_ANTENNA3 = 179,
	TT_COPY_NAME = 180,
	TT_REQ_ACCEPT_INVITE_PARTY = 181,
	TT_REQ_REFUSE_INVITE_PARTY = 182,
	TT_REQ_INVITE_PARTY = 183,
	TT_REQ_OPEN_EQUIPEDITEM_ONOFF = 184,
	TT_BATTLECHAT_ON_OFF = 185,
	TT_REQ_EMOTION_HUM2 = 186,
	TT_REQ_EMOTION_ABS = 187,
	TT_REQ_EMOTION_OOPS = 188,
	TT_REQ_EMOTION_SPIT = 189,
	TT_REQ_EMOTION_ENE = 190,
	TT_REQ_EMOTION_PANIC = 191,
	TT_REQ_EMOTION_WHISP = 192,
};

enum {
	MAT_ATTACK,
	MAT_CASTSKILL,
	MAT_USESKILL,
	MAT_ATTACKED_SKILL,
	MAT_HP50,
	MAT_HP25,
	MAT_DEAD,
};
enum {
	ACCESSORY_GOGGLES = 1,
	ACCESSORY_CATTYHAIRBAND = 2,
	ACCESSORY_GLASS = 3,
	ACCESSORY_FLOWER = 4,
	ACCESSORY_FLORALHAIRBAND = 5,
	ACCESSORY_HOOD = 6,
	ACCESSORY_ROUNDCAP = 7,
	ACCESSORY_FLUMASK = 8,
	ACCESSORY_HAIRBAND = 9,
	ACCESSORY_DIVERSGOGGLES = 10,
	ACCESSORY_BIRETTA = 11,
	ACCESSORY_SUNGLASS = 12,
	ACCESSORY_EYEBANDAGE = 13,
	ACCESSORY_CAP = 14,
	ACCESSORY_BUNNYBAND = 15,
	ACCESSORY_HAT = 16,
	ACCESSORY_RIBBON = 17,
	ACCESSORY_CIRCLET = 18,
	ACCESSORY_TIARA = 19,
	ACCESSORY_SANTAHAT = 20,
	ACCESSORY_BEARD = 21,
	ACCESSORY_MUSTACHE = 22,
	ACCESSORY_SPECTACLE = 23,
	ACCESSORY_BLACK_BEARD = 24,
	ACCESSORY_WHITE_BEARD = 25,
	ACCESSORY_QUALITY_SUNGLASS = 26,
	ACCESSORY_SPIN_GLASS = 27,
	ACCESSORY_LARGE_RIBBON = 28,
	ACCESSORY_SWEET_GENTLE = 29,
	ACCESSORY_GOLDEN_HEADGEAR = 30,
	ACCESSORY_OLDSTER_ROMANCE = 31,
	ACCESSORY_WESTERN_GRACE = 32,
	ACCESSORY_CORONET = 33,
	ACCESSORY_HAIR_STRING = 34,
	ACCESSORY_PRIEST_CAP = 35,
	ACCESSORY_WIZARD_HAT = 36,
	ACCESSORY_SUNFLOWER = 37,
	ACCESSORY_ANGEL_HAIRBAND = 38,
	ACCESSORY_DEVIL_HAIRBAND = 39,
	ACCESSORY_HELM = 40,
	ACCESSORY_MAJESTIC_GOUT = 41,
	ACCESSORY_WHITE_HORN = 42,
	ACCESSORY_SHARP_HEADGEAR = 43,
	ACCESSORY_WEDDING_VEIL = 44,
	ACCESSORY_CROWN = 45,
	ACCESSORY_PROPELLER = 46,
	ACCESSORY_TINY_EYE_GLASSES = 47,
	ACCESSORY_PRONTERA_ARMY_CAP = 48,
	ACCESSORY_PIERROTS_NOSE = 49,
	ACCESSORY_PIRATES_EYEBANDAGE = 50,
	ACCESSORY_MUNAK_TURBAN = 51,
	ACCESSORY_HIP_HOP_MASK = 52,
	ACCESSORY_IRONCANE = 53,
	ACCESSORY_CIGAR = 54,
	ACCESSORY_CIGAR_PIPE = 55,
	ACCESSORY_SENTIMENTAL_FLOWER = 56,
	ACCESSORY_SENTIMENTAL_BLADE = 57,
	ACCESSORY_HEY_DUDE = 58,
	ACCESSORY_STOP_SIGNPOST = 59,
	ACCESSORY_DOCTOR_BAND = 60,
	ACCESSORY_JAPANESE_GHOST = 61,
	ACCESSORY_SCARLET_BANDANA = 62,
	ACCESSORY_EAGLE_EYES = 63,
	ACCESSORY_NURSE_CAP = 64,
	ACCESSORY_SMILE = 65,
	ACCESSORY_BOMB_WICK = 66,
	ACCESSORY_SAHT_GAHT = 67,
	ACCESSORY_MASQUERADE = 68,
	ACCESSORY_SERAPHIC_RING = 69,
	ACCESSORY_EARMUFFS = 70,
	ACCESSORY_MOOSE_HORN = 71,
	ACCESSORY_THE_APPLE_OF_WILHELM_TELL = 72,
	ACCESSORY_TINKER_BELL = 73,
	ACCESSORY_PIRATE_BANDANA = 74,
	ACCESSORY_MUNCHS_SCREAM = 75,
	ACCESSORY_POO_POO_HAT = 76,
	ACCESSORY_KOEAN_FUNERAL_COSTUME = 77,
	ACCESSORY_BUTTERFLY_MASQUE = 78,
	ACCESSORY_WELDER_MASK = 79,
	ACCESSORY_PRETEND_TO_BE_MURDERED = 80,
	ACCESSORY_STAR_DUST = 81,
	ACCESSORY_EYE_MASK = 82,
	ACCESSORY_BINOCULARS = 83,
	ACCESSORY_GOBLIN_MASQUE = 84,
	ACCESSORY_GREEN_FEELER = 85,
	ACCESSORY_VIKING_HELMET = 86,
	ACCESSORY_HEAD_SET = 87,
	ACCESSORY_GEMMED_CROWN = 88,
	ACCESSORY_PIERROT_CROWN = 89,
	ACCESSORY_OXYGEN_MASK = 90,
	ACCESSORY_GAS_MASK = 91,
	ACCESSORY_MACHO_MANS_GLASSES = 92,
	ACCESSORY_LORD_CIRCLET = 93,
	ACCESSORY_PUPPY_LOVE = 94,
	ACCESSORY_CRASH_HELMET = 95,
	ACCESSORY_INDIAN_BAND = 96,
	ACCESSORY_AERIAL = 97,
	ACCESSORY_BA_HAT = 98,
	ACCESSORY_HORN_OF_LORD_KAHO = 99,
	ACCESSORY_FIN_HELM = 100,
	ACCESSORY_EGG_SHELL = 101,
	ACCESSORY_KOREAN_SCHOOL_HAT = 102,
	ACCESSORY_BONE_HELM = 103,
	ACCESSORY_WILHELM_TELLS_HAT = 104,
	ACCESSORY_CORSAIR = 105,
	ACCESSORY_BAND_OF_KAFRA = 106,
	ACCESSORY_LOST_MONEY_HEART = 107,
	ACCESSORY_BUNDLE = 108,
	ACCESSORY_CAKE_HAT = 109,
	ACCESSORY_ANGEL_HELM = 110,
	ACCESSORY_COOKER_HAT = 111,
	ACCESSORY_MAGE_HAT = 112,
	ACCESSORY_CANDLE = 113,
	ACCESSORY_SPORE_HAT = 114,
	ACCESSORY_PANDA_HAT = 115,
	ACCESSORY_MINE_HAT = 116,
	ACCESSORY_OUTSIDE_HAT = 117,
	ACCESSORY_RACCOON_HAT = 118,
	ACCESSORY_LAMP_BAND = 119,
	ACCESSORY_PORING_HAT = 120,
	ACCESSORY_CROSS_BAND = 121,
	ACCESSORY_NUT_SHELL = 122,
	ACCESSORY_YOUNG_DEVIL_HAT = 123,
	ACCESSORY_SPOT_EGG_SHELL = 124,
	ACCESSORY_DYE_RED = 125,
	ACCESSORY_HEART_HAIRPIN = 126,
	ACCESSORY_HAIR_CLOTH = 127,
	ACCESSORY_GHOST_MASK = 128,
	ACCESSORY_DEVIL_WING = 129,
	ACCESSORY_WIZARD_HEADGEAR = 130,
	ACCESSORY_FASION_SUNGLASSES = 131,
	ACCESSORY_MOON_HEARPIN = 132,
	ACCESSORY_STRIPES_BAND = 133,
	ACCESSORY_MYSTIC_NUT_SHELL = 134,
	ACCESSORY_CAT_BELL = 135,
	ACCESSORY_BLUE_BAND = 136,
	ACCESSORY_SPHINX_HAT = 137,
	ACCESSORY_SUN_GOD_HAT = 138,
	ACCESSORY_BOMGUN_HAT = 139,
	ACCESSORY_LOVE_NUT = 140,
	ACCESSORY_BLACKCAT_EAR = 141,
	ACCESSORY_LAZYCAT = 142,
	ACCESSORY_BEAR_HAT = 143,
	ACCESSORY_KOCAL_HAT = 144,
	ACCESSORY_FLOWER_HAIRPIN = 145,
	ACCESSORY_MILZIP_HAT = 146,
	ACCESSORY_DAEILBAND = 147,
	ACCESSORY_LEAF_HELM = 148,
	ACCESSORY_FISH_HAT = 149,
	ACCESSORY_SERCUBUS_HORN = 150,
	ACCESSORY_SOM_BRERO = 151,
	ACCESSORY_DEVILWING_EAR = 152,
	ACCESSORY_FOX_MASK = 153,
	ACCESSORY_FIRE_HEADBAND = 154,
	ACCESSORY_INDIAN_HEADBAND = 155,
	ACCESSORY_INCUBUS_HORN = 156,
	ACCESSORY_STRAIGHT_HAT = 157,
	ACCESSORY_ANGELWING_EAR = 158,
	ACCESSORY_COWBOY_HAT = 159,
	ACCESSORY_FUR_HAT = 160,
	ACCESSORY_TULIP_HAIRPIN = 161,
	ACCESSORY_HAEDAL_HAT = 162,
	ACCESSORY_X_HAIRPIN = 163,
	ACCESSORY_QUEEN_HELM = 164,
	ACCESSORY_MISTRESS_CROWN = 165,
	ACCESSORY_MUSHROOM_HEADBAND = 166,
	ACCESSORY_REDTAIL_RIBBON = 167,
	ACCESSORY_LAZY_RACCOON = 168,
	ACCESSORY_BELOWEAR_RIBBON = 169,
	ACCESSORY_ALARM_MASK = 170,
	ACCESSORY_GOBLIN_MASK1 = 171,
	ACCESSORY_GOBLIN_MASK2 = 172,
	ACCESSORY_GOBLIN_MASK3 = 173,
	ACCESSORY_GOBLIN_MASK4 = 174,
	ACCESSORY_BIGGOLDEN_BELL = 175,
	ACCESSORY_SISTER_HAT = 176,
	ACCESSORY_SISTER_HAT2 = 177,
	ACCESSORY_ORCHERO_HELM = 178,
	ACCESSORY_ORCHERO_HELM2 = 179,
	ACCESSORY_ASSASSIN_MASK = 180,
	ACCESSORY_TIGER_MASK = 181,
	ACCESSORY_CAT_HAT = 182,
	ACCESSORY_SALE_HAT = 183,
	ACCESSORY_TAKIUS = 184,
	ACCESSORY_BATTLEONLINE = 185,
	ACCESSORY_HAEBARAGI_PIN = 186,
	ACCESSORY_BLACK_EYECOVER = 187,
	ACCESSORY_TOAST = 188,
	ACCESSORY_DECTECTIVE_HAT = 189,
	ACCESSORY_CHAYANG_HAT = 190,
	ACCESSORY_BABY = 191,
	ACCESSORY_DUCK_HAT = 192,
	ACCESSORY_SUNO_HAT = 193,
	ACCESSORY_ANGRY_MOUTH = 194,
	ACCESSORY_BEONG_HAT = 195,
	ACCESSORY_COLDPROTECT_HAT = 196,
	ACCESSORY_BANANA_HAT = 197,
	ACCESSORY_MYSTICROSE = 198,
	ACCESSORY_PUPPY_EAR = 199,
	ACCESSORY_ZIRTAS_MASK = 200,
	ACCESSORY_BIRD_NEST = 201,
	ACCESSORY_LION_MASK = 202,
	ACCESSORY_CROSS_HELMET = 203,
	ACCESSORY_ANGELING_HAT = 204,
	ACCESSORY_GOAT_HAT = 205,
	ACCESSORY_PUMPKIN_HAT = 206,
	ACCESSORY_CYCLOPS_EYE = 207,
	ACCESSORY_ALICE_HAT = 208,
	ACCESSORY_MAGIC_EYES = 209,
	ACCESSORY_PUYONG = 210,
	ACCESSORY_PRETTY_RIBBON = 211,
	ACCESSORY_MARIONET_HAT = 212,
	ACCESSORY_MOON_HELM = 213,
	ACCESSORY_GABUKI_MASK = 214,
	ACCESSORY_KOKIRI_HAT = 215,
	ACCESSORY_BASEBALL_HAT = 216,
	ACCESSORY_GOOKGYEONG_HAT = 217,
	ACCESSORY_SILVERTIARA = 218,
	ACCESSORY_PIERROT_CROWN_ = 219,
	ACCESSORY_MUSICHEADPHONE = 220,
	ACCESSORY_BONGCROWN = 221,
	ACCESSORY_ANGELING_PIN = 222,
	ACCESSORY_GAKSITAL = 223,
	ACCESSORY_FEATHER_BEREHAT = 224,
	ACCESSORY_VALKYRIE_HELMET = 225,
	ACCESSORY_BEREHAT = 226,
	ACCESSORY_SATOHAT = 227,
	ACCESSORY_AYAM = 228,
	ACCESSORY_NONAME = 229,
	ACCESSORY_HAHETAL = 230,
	ACCESSORY_LIONTAL = 231,
	ACCESSORY_GOLDTIARA = 232,
	ACCESSORY_DARKHELMET = 233,
	ACCESSORY_DOGHAT = 234,
	ACCESSORY_BIRDHAT = 235,
	ACCESSORY_CAPTAINHAT = 236,
	ACCESSORY_BAYTREEHAT = 237,
	ACCESSORY_GEOGRAPHER = 238,
	ACCESSORY_TWINRIBBON = 239,
	ACCESSORY_SINGERHAT = 240,
	ACCESSORY_LEAF = 241,
	ACCESSORY_BLACKRIBBON = 242,
	ACCESSORY_YELLOWRIBBON = 243,
	ACCESSORY_GREENRIBBON = 244,
	ACCESSORY_PINKRIBBON = 245,
	ACCESSORY_REDRIBBON = 246,
	ACCESSORY_ORANGERIBBON = 247,
	ACCESSORY_WHITERIBBON = 248,
	ACCESSORY_LAZYRABBIT = 249,
	ACCESSORY_MEGINCAP = 250,
	ACCESSORY_PRIKACIRCLET = 251,
	ACCESSORY_COPOLA = 252,
	ACCESSORY_MERONGMASK = 253,
	ACCESSORY_WOOLCAP = 254,
	ACCESSORY_ANGELKISS = 255,
	ACCESSORY_MORPHISDOOGUN = 256,
	ACCESSORY_MORIANEHELM = 257,
	ACCESSORY_GEVNEITUGOO = 258,
	ACCESSORY_WHITEFLOWER = 259,
	ACCESSORY_GOLDENGORI = 260,
	ACCESSORY_LAUREL = 261,
	ACCESSORY_RIDEWORD = 262,
	ACCESSORY_YELLOW_BASEBALLCAP = 263,
	ACCESSORY_FLYING_ANGEL = 264,
	ACCESSORY_DRESSHAT = 265,
	ACCESSORY_SATELITE_HAIRBAND = 266,
	ACCESSORY_BLACK_BUNNYBAND = 267,
	ACCESSORY_WALLYAHWA_HAT = 268,
	ACCESSORY_MARCHING_HAT = 269,
	ACCESSORY_PROPELLER2 = 270,
	ACCESSORY_RED_DEVIRUCHI = 271,
	ACCESSORY_WHITE_DEVIRUCHI = 272,
	ACCESSORY_GRAY_DEVIRUCHI = 273,
	ACCESSORY_WHITE_LAZYCAT = 274,
	ACCESSORY_GRAY_LAZYCAT = 275,
	ACCESSORY_PINK_LAZYCAT = 276,
	ACCESSORY_BLUE_LAZYCAT = 277,
	ACCESSORY_YELLOW_LAZYCAT = 278,
	ACCESSORY_GRAY_FURHAT = 279,
	ACCESSORY_BLUE_FURHAT = 280,
	ACCESSORY_PINK_FURHAT = 281,
	ACCESSORY_RED_MAGICIANHAT = 282,
	ACCESSORY_WHITE_MAGICIANHAT = 283,
	ACCESSORY_GRAY_MAGICIANHAT = 284,
	ACCESSORY_BLUE_MAGICIANHAT = 285,
	ACCESSORY_YELLOW_MAGICIANHAT = 286,
	ACCESSORY_EARPLUG_HAT = 287,
	ACCESSORY_ELF_BLINDER = 288,
	ACCESSORY_ELF_SUNGLASS = 289,
	ACCESSORY_ANGEL_MIXHELM = 290,
	ACCESSORY_DEVIL_MIXHELM = 291,
	ACCESSORY_ROBOT_BLINDER = 292,
	ACCESSORY_HUMAN_BLINDER = 293,
	ACCESSORY_ROBOT_EAR = 294,
	ACCESSORY_HUMAN_EAR = 295,
	ACCESSORY_LAZY_NINETAIL = 296,
	ACCESSORY_LAZY_LEAF = 297,
	ACCESSORY_DEVILRING_HAT = 298,
	ACCESSORY_3PORINGS = 299,
	ACCESSORY_VALKYRIE_FEATHER = 300,
	ACCESSORY_SOULESS_WING = 301,
	ACCESSORY_APRO_WIG = 302,
	ACCESSORY_ATIATI_HAT = 303,
	ACCESSORY_AUSTRALIA_HAT = 304,
	ACCESSORY_HAPPY_WIG = 305,
	ACCESSORY_BRIGHT_WIG = 306,
	ACCESSORY_AMAZING_WIG = 307,
	ACCESSORY_FANTASTIC_WIG = 308,
	ACCESSORY_YELLOW_DOOGUN = 309,
	ACCESSORY_YELLOW_RIBBON = 310,
	ACCESSORY_CHICK_HAT = 311,
	ACCESSORY_SUREON_CROWN = 312,
	ACCESSORY_WINDMILL_HAIRPIN = 313,
	ACCESSORY_PECOPECO_HAT = 314,
	ACCESSORY_VACATION_HAT = 315,
	ACCESSORY_RED_GLASSES = 316,
	ACCESSORY_VANIL_HAT = 317,
	ACCESSORY_KETTLE_HAT = 318,
	ACCESSORY_DRAGON_SKULL = 319,
	ACCESSORY_RAMYEON_HAT = 320,
	ACCESSORY_WHISPER_MASK = 321,
	ACCESSORY_HORN_HAT = 322,
	ACCESSORY_RUSSIA_RIBBON = 323,
	ACCESSORY_LOTUS_FLOWER = 324,
	ACCESSORY_CEREMONIAL_CORONET = 325,
	ACCESSORY_MANGMOK = 326,
	ACCESSORY_PIRATE_DAGGER = 327,
	ACCESSORY_FREYA_CROWN = 328,
	ACCESSORY_TROPICALFRUIT = 329,
	ACCESSORY_BRAZIL_HAT = 330,
	ACCESSORY_MOHIKAN = 331,
	ACCESSORY_BANJAK_JEONGOO = 332,
	ACCESSORY_BIG_HIBISCUS = 333,
	ACCESSORY_AYOTAYA_HAT = 334,
	ACCESSORY_DIA_DAM = 335,
	ACCESSORY_HAKI_MASK = 336,
	ACCESSORY_OBSERVER = 337,
	ACCESSORY_UMBRELLA_HAT = 338,
	ACCESSORY_FISHER_HAT = 339,
	ACCESSORY_PORINGPARTY_HAT = 340,
	ACCESSORY_BLUE_ANGELING_HAT = 341,
	ACCESSORY_INDONESIA_HARIBAND = 342,
	ACCESSORY_SCARF = 343,
	ACCESSORY_LITTLEANGELDOLL = 344,
	ACCESSORY_ROBOTEYE = 345,
	ACCESSORY_ROKIMASK = 346,
	ACCESSORY_JEONPA_ANTENNA = 347,
	ACCESSORY_WANTTOFLY_ANGELING = 348,
	ACCESSORY_JUMPING_PORING = 349,
	ACCESSORY_WANT_GUILDMAN = 350,
	ACCESSORY_WANT_PARTY = 351,
	ACCESSORY_WANT_BOY = 352,
	ACCESSORY_WANT_FRIEND = 353,
	ACCESSORY_DIFORTE = 354,
	ACCESSORY_GLARRIS = 355,
	ACCESSORY_SORIN = 356,
	ACCESSORY_TAILING = 357,
	ACCESSORY_BINIT = 358,
	ACCESSORY_W = 359,
	ACCESSORY_WANT_GIRL = 360,
	ACCESSORY_POPORING_HAT = 361,
	ACCESSORY_MUSLIM_M = 362,
	ACCESSORY_MUSLIM_F = 363,
	ACCESSORY_MAWANG_HAT = 364,
	ACCESSORY_VICTORYWING = 365,
	ACCESSORY_PECOPECO_FLY_HAT = 366,
	ACCESSORY_CAPTION_HAT = 367,
	ACCESSORY_WHIKEBAIN_EARS = 368,
	ACCESSORY_GANG_SCARF = 369,
	ACCESSORY_NINJA_SCROLL = 370,
	ACCESSORY_SIMYEON_HAT = 371,
	ACCESSORY_BLACKDEATHKING = 372,
	ACCESSORY_EGGFRY = 373,
	ACCESSORY_EMPEROR_HAT = 374,
	ACCESSORY_HAEGOON_HAT = 375,
	ACCESSORY_NANCHO_HAT = 376,
	ACCESSORY_PANGUAN_HAT = 377,
	ACCESSORY_LAZY_WHITECAT = 378,
	ACCESSORY_DARKNESS_HELM = 379,
	ACCESSORY_HUGE_MAJESTIC = 380,
	ACCESSORY_HUGE_ORCHERO = 381,
	ACCESSORY_SATANIC_CHAIN = 382,
	ACCESSORY_ENTIQUE_CIGAR = 383,
	ACCESSORY_RABBITEAR = 384,
	ACCESSORY_BALLOON_HAT = 385,
	ACCESSORY_FISH_HEAD_HAT = 386,
	ACCESSORY_SANTA_PORING_HAT = 387,
	ACCESSORY_BELL_RIBBON = 388,
	ACCESSORY_HUNTINGCAP = 389,
	ACCESSORY_TWIN_SANTAHAT = 390,
	ACCESSORY_YOYOHAT = 391,
	ACCESSORY_NEKOMIMI_KAFRA = 392,
	ACCESSORY_SNAKE_HAT = 393,
	ACCESSORY_ANGEL_GHOST = 394,
	ACCESSORY_BLUE_SANTAHAT = 395,
	ACCESSORY_SEOLNAL_HAT = 396,
	ACCESSORY_VALENTINE_HAT = 397,
	ACCESSORY_TIRAYABONET = 398,
	ACCESSORY_JASPER = 399,
	ACCESSORY_SCUVER_MASK = 400,
	ACCESSORY_BONEHEAD = 401,
	ACCESSORY_MANDRAGORA = 402,
	ACCESSORY_FOXHAT = 403,
	ACCESSORY_BLACKGLASSES = 404,
	ACCESSORY_PLAYELF = 405,
	ACCESSORY_FISHINMOUTH = 406,
	ACCESSORY_BLUERIBBON = 407,
	ACCESSORY_PHILIRHAT = 408,
	ACCESSORY_ACADEMY_NEWBIE = 409,
	ACCESSORY_ACADEMY_LV1 = 410,
	ACCESSORY_NEW_COWBOY = 411,
	ACCESSORY_BREAD2 = 412,
	ACCESSORY_LAST = 413,
};

struct MapPosInfo {
public:
	MapPosInfo(int xLT, int yLT, int xRB, int yRB)
		: x_LT(xLT), y_LT(yLT), x_RB(xRB), y_RB(yRB) {}
	MapPosInfo() : x_LT(0), y_LT(0), x_RB(0), y_RB(0) {}
	int x_LT;
	int y_LT;
	int x_RB;
	int y_RB;
};



#define	 MAP_IMG_W	1280
#define	 MAP_IMG_H	1024

typedef std::pair<const char *, TALKTYPE> TTPair;
typedef std::pair<int, TALKTYPE> TTPair2;
typedef std::pair<const char *, DWORD> WinPair;



struct FRIEND_INFO {
	FRIEND_INFO(int n) : isValid(0) {}
	FRIEND_INFO() : isValid(1) {}
	int isValid;
	unsigned long AID;
	unsigned long GID;
	std::string characterName;
	std::string mapName;
	int role;
	int state;
	unsigned long color;
};

class CGameActor;


struct FORCE_ANIM_TICK {
	unsigned long AID;
	unsigned long tick;
};

struct CHAT_PROHIBIT_LOG {
	unsigned long AID;
	unsigned long GID;
	unsigned long prohibitTime;
	unsigned long logDate;
	unsigned long logTime;
	char accountName[32];
	char charName[32];
	char serverName[32];
};
struct REQ_ACCOUNT_AID {
public:
	unsigned long AID;
	std::string accountName;
};

struct REQ_ACCOUNTNAME_INFO {
	unsigned long AID;
	unsigned long reqTime;
};


class DComp {
public:
	inline bool operator()(const std::pair<char const *, enum TALKTYPE>& l, const std::pair<char const *, enum TALKTYPE>& r) const
	{ return KeyLess(l.first, r.first); }
	inline bool operator()(std::pair<char const *, enum TALKTYPE>::first_type& k, const std::pair<char const *, enum TALKTYPE>& r) const
	{ return KeyLess(k, r.first); }
	inline bool operator()(const std::pair<char const *, enum TALKTYPE>& l, const std::pair<char const *, enum TALKTYPE>::first_type const& k) const
	{ return KeyLess(l.first, k); }
private:
	inline bool KeyLess(const std::pair<char const *, enum TALKTYPE>::first_type& k1, const std::pair<char const *, enum TALKTYPE>::first_type& k2) const
	{ return strcmp(k1, k2) < 0; }
};

class SComp {
public:
	inline bool operator()(const std::pair<char const *, char const *>& l, const std::pair<char const *, char const *>& r) const
	{ return KeyLess(l.first, r.first); }
	inline bool operator()(std::pair<char const *, char const *>::first_type& k, const std::pair<char const *, char const *>& r) const
	{ return KeyLess(k, r.first); }
	inline bool operator()(const std::pair<char const *, char const *>& l, const std::pair<char const *, char const *>::first_type& k) const
	{ return KeyLess(l.first, k); }
private:
	inline bool KeyLess(const std::pair<char const *, char const *>::first_type& k1, const std::pair<char const *, char const *>::first_type& k2) const
	{ return strcmp(k1, k2) < 0; }
};

class SListComp {
public:
	bool operator()(const std::pair<std::string, std::vector<std::string> >& l, const std::string& k) const { return KeyLess(l.first, k); }
	bool operator()(std::string& k, const std::pair<std::string, std::vector<std::string> >& r) const { return KeyLess(k, r.first); }
	bool operator()(const std::pair<std::string, std::vector<std::string> >& l, const std::pair<std::string, std::vector<std::string> >& r) const { return KeyLess(l.first, r.first); }
private:
	bool KeyLess(const std::string& k1, const std::string& k2) const { return strcmp(k1.c_str(), k2.c_str()) < 0; }
};

class IListComp {
public:
	bool operator()(const std::pair<std::string, std::vector<int> >& l, const std::string& k) const { return KeyLess(l.first, k); }
	bool operator()(std::string& k, const std::pair<std::string, std::vector<int> >& r) const { return KeyLess(k, r.first); }
	bool operator()(const std::pair<std::string, std::vector<int> >& l, const std::pair<std::string, std::vector<int> >& r) const { return KeyLess(l.first, r.first); }
private:
	bool KeyLess(const std::string& k1, const std::string& k2) const { return strcmp(k1.c_str(), k2.c_str()) < 0; }
};



class IListComp2 {
public:
	bool operator()(const std::pair<LPCSTR, bool>& l, const std::pair<LPCSTR, bool>& r) const { return KeyLess(l.first, r.first); }
	bool operator()(std::pair<LPCSTR, bool>::first_type& k, const std::pair<LPCSTR, bool>& r) const { return KeyLess(k, r.first); }
	bool operator()(const std::pair<LPCSTR, bool>& l, const std::pair<LPCSTR, bool>::first_type& k) const { return KeyLess(l.first, k); }

private:
	bool KeyLess(const std::pair<LPCSTR, bool>::first_type& k1, const std::pair<LPCSTR, bool>::first_type& k2) const { return strcmp(k1, k2) < 0; }
};

class LPCSTRComp {
public:
	bool operator()(LPCSTR l, const LPCSTR r) const { return (strcmp(l, r)<0); }
};

class CSession {
public:
	CSession();
	~CSession();
	void Init();
	void Create();
	void InitTable();
	void InitLanguageTable();
	void InitVariables();
	void InitPcNameTable();
	void InitJobNameTable();
	void InitTalkTypeTable();
	void InitMapTypeInfoTable();
	int IsSpace(char ch);
	int IsEnablePK();
	int IsEnableAllyGuild();
	int IsEnableSiegeMode();
	int IsEnableNeverMemory();
	int IsEnableGomok();
	int IsEnableMemory();
	int IsEnableDropExp();
	int IsEnableTeleport();
	int IsEnableDropItem();
	int IsEnableAllyParty();
	int IsEnableUseSimpleEffect();
	int IsEnableDisableLockOn();
	int IsEnableCountPK();
	int IsEnableNoPartyFormation();
	int IsEnableBattleField();
	int InitMapNameTable();
	int InitSystemInfoTable();
	int InitEnableObjLightMapTable();
	int InitMp3NameTable();
	int InitIndoorRswTable();
	int InitViewPointTable();
	int InitMapPosTable();
	int InitDivisionMapPosTable();
	void CalculateMapPos(MapPosInfo* mapPosInfo);
	void RefreshSkillItemWnd();
	void RefreshQuestWnd();
	void RefreshQuestDetailWnd();
	void ClearQuestList() { m_QuestList.clear(); }
	bool AddQuestList(QUEST_INFO& QuestInfo, bool needRefresh);
	void ClearSkillItem() { m_skillInfoList.clear(); }
	int GetSkillItemCount() { return m_skillInfoList.size(); }
	bool AddSkillItem(SKILL_INFO& skillInfo, bool needRefresh);
	SKILL_INFO GetSkillItemInfoBySkillId(int __$ReturnUdt);
	SKILL_INFO GetSkillItemInfoBySequence(int __$ReturnUdt);
	SKILL_INFO GetSkillItemInfoByName(const char* __$ReturnUdt);
	int GetHomunSkillItemCount() { return m_homunSkillList.size(); }
	bool AddHomunSkillItem(SKILL_INFO& skillInfo, bool needRefresh);
	void RefreshHomunSkillItemWnd();
	void ClearHomunSkillItem() { m_homunSkillList.clear(); }
	void SetHomunSkillUseLevel(int sequence, int price);
	int GetHomunSkillPoint() { return m_homunSkillPoint; }
	int GetHomunSkillUseLevel(int index);
	SKILL_INFO GetHomunSkillItemInfoBySkillId(int __$ReturnUdt);
	SKILL_INFO GetHomunSkillItemInfoBySequence(int __$ReturnUdt);
	SKILL_INFO GetHomunSkillItemInfoByName(const char* __$ReturnUdt);
	int GetMerSkillItemCount() { return m_merSkillList.size(); }
	bool AddMerSkillItem(SKILL_INFO& skillInfo, bool needRefresh);
	void RefreshMerSkillItemWnd();
	void ClearMerSkillItem() { m_merSkillList.clear(); }
	void SetMerSkillUseLevel(int sequence, int price);
	int GetMerSkillPoint() { return m_merSkillPoint; }
	int GetMerSkillUseLevel(int index);
	SKILL_INFO GetMerSkillItemInfoBySkillId(int __$ReturnUdt);
	SKILL_INFO GetMerSkillItemInfoBySequence(int __$ReturnUdt);
	SKILL_INFO GetMerSkillItemInfoByName(const char* __$ReturnUdt);
	int GetGuildSkillUseLevel(int index);
	int GetGuildSkillPoint() { return m_guildSkillPoint; }
	void ClearGuildSkillItem() { m_guildSkillList.clear(); }
	void SetGuildSkillUseLevel(int sequence, int price);
	int GetGuildSkillItemCount();
	bool AddGuildSkillItem(SKILL_INFO& skillInfo, bool needRefresh);
	SKILL_INFO GetGuildSkillItemInfoBySequence(int __$ReturnUdt);
	bool AddTempSkillItem(SKILL_INFO& skillInfo);
	SKILL_INFO GetTempSkillItemInfoBySkillId(int __$ReturnUdt);
	void ClearShortenSkillUseLevel();
	int GetShortenSkillUseLevel(int index);
	void SetShortenSkillUseLevel(int sequence, int price);
	void SetShortenSkillName(const char* shortenSkillName, int skillUseLevel, int slot);
	void InitLevelUseSkillList();
	void InitSkillEffectIdTable();
	void SetSkillUseLevel(int sequence, int price);
	void GetSkillBitmapFileName(const char* itemName, char* buf);
	int GetSkillUseLevel(int index);
	int GetAttackEffectIdBySkillId(int index);
	int GetAttackedEffectIdBySkillId(int index);
	const char* GetItemSpriteFileName_Skill(const char* itemName);
	std::vector<int>& LevelUseSkillSpAmountList(const char*, int&);
	void GetSkillActionInfo(int skillId, int& beginEffectId, int& motionType);
	void GetSkillActionInfo2(int skillId, int& beginEffectId, int& motionType, int property, int sjob);
	void LaunchSkillSpecialEffect(int skillId, int level, CGameActor* sActor, CGameActor* tActor);
	int IsAbracatabra(int index);
	void InitWithClear();
	void InitJobHitWaveName();
	int InitFogParameterTable();
	void InitWeaponHitWaveName();
	void InitShadowFactorWithJob();
	void InitWeaponSwingWaveName();
	void WriteOptionToRegistry();
	void WriteExListToRegistry();
	void InitExListWithRegistry();
	void InitOptionWithRegistry();
	void InitWhenServerMove();
	int GetHeadJob(int head, int job);
	void CorrectHead(int& head, int job);
	unsigned long GetServerTime();
	unsigned long GetCurSystemTime();
	void SetServerTime(unsigned long pingTime);
	bool IsOnlyEnglish(const char* rswName);
	void UpdateServerTime(unsigned long pingTime);
	void RecalcAverageDiffTime(int money);
	void RecalcAveragePingTime(unsigned long pingTime);
	void SetPlayerPosDir(int x, int y, int dir);
	bool IsBadNameWithSpaceChar(const char* rswName);
	void SetPlayerInfo(int job, int sex, char* name, int speed);
	int GetTalkType(const char* chatBuf, TALKTYPE* talkType, std::string param[3]);
	void ClearPlusValue();
	void ClearShortenItemName();
	int GetDistByNumCell(int index);
	const char* RemoveUnderbar(const char* msg, char* buf);
	int NumExName();
	void DeleteAllExNameList();
	int IsExName(const char* itemName);
	void AddToExNameList(const char* characterName);
	void DeleteFromExNameList(const char* characterName);
	int NumExAid();
	int IsExAid(unsigned long gid);
	void DeleteAllExAidList();
	void AddToExAidList(unsigned long pingTime);
	void DeleteFromExAidList(unsigned long pingTime);
	std::list<std::string>& GetNumExNameList();
	const char* GetAttrWaveName(int slot);
	const char* GetJobHitWaveName(int slot);
	const char* GetMp3Name(const char* itemName);
	const char* GetMapName(const char* itemName);
	const char* GetWeaponHitWaveName(int slot);
	const char* GetWeaponSwingWaveName(int slot);
	const char* GatName2RswName(const char* itemName);
	const char* GetItemSpriteFileName(const char* itemName, int isIdentified);
	bool IsIndoorRswName(const char* rswName);
	bool IsEnableObjLightMap(const char* rswName);
	std::vector<std::string>& GetFogParameter(const char* rswName, int& isExist);
	std::vector<std::string>& GetViewPointParameter(const char* rswName, int& isExist);
	void GetItemBitmapFileName(const char* itemName, char* buf, int isIdentified);
	int GetNumItem();
	bool IsVisibleNPC(JOBTYPE jobType);
	ITEM_INFO GetItemInfoById(int __$ReturnUdt);
	ITEM_INFO GetItemInfoByIndex(int __$ReturnUdt);
	ITEM_INFO GetItemInfoBySequence(int __$ReturnUdt);
	char* GetJobActName(int job, int sex, char* buf);
	char* GetJobSprName(int job, int sex, char* buf);
	char* GetJobDyeActName(int job, int sex, char* buf);
	char* GetJobDyeSprName(int job, int sex, char* buf);
	char* GetImfName(int job, int head, int sex, char* buf);
	char* GetHeadActName(int job, int& head, int sex, char* buf);
	char* GetHeadSprName(int job, int& head, int sex, char* buf);
	char* GetShieldActName(int job, int head, int sex, char* buf);
	char* GetShieldSprName(int job, int head, int sex, char* buf);
	char* GetAccessoryActName(int job, int& head, int sex, int accessory, char* buf);
	char* GetAccessorySprName(int job, int& head, int sex, int accessory, char* buf);
	const char* GetItemName(int slot);
	void SetItem(ITEM_INFO* itemInfo);
	bool AddItem(ITEM_INFO* itemInfo);
	void DecItem(unsigned long index, int numItem);
	bool AddEquipItem(ITEM_INFO* itemInfo);
	bool AddItemNoRefresh(ITEM_INFO* itemInfo);
	bool IsValidItemIndexInEquiped(int index);
	unsigned char AddItemWithNoRefresh(ITEM_INFO* itemInfo);
	bool IsValidItemIndexInUnequiped(int index);
	bool MoveItemFromEquipWndToItemWnd(int index);
	void BitMaskToSlotNum(int wearLocation, int* slotNum);
	bool MoveItemFromItemWndToEquipWnd(ITEM_INFO* itemInfo);
	void ClearItem();
	void ClearShopItem();
	void ClearDealItems();
	void ClearEquipItem();
	void ClearMerchantDealItems();
	void ClearEquipItemInInventory();
	void ClearAntiEquipItemInInventory();
	void ClearMerchantShopItem();
	void ClearMerchantMyShopItem();
	int GetNumShopItem();
	int GetShopItemNum(int index);
	void DecShopItem(ITEM_INFO& itemInfo);
	bool IsExistInShopItemList(int index);
	ITEM_INFO GetShopItemInfoBySequence(int __$ReturnUdt);
	void AddShopItemFromSell(ITEM_INFO& itemInfo);
	void AddShopItem(SHOP_ITEM& shop_item, int isSellMode);
	int GetNumMerchantShopItem();
	int GetMerchantShopItemNum(int index);
	void AddMerchantShopItem(ITEM_INFO& itemInfo);
	void DecMerchantShopItem(ITEM_INFO& itemInfo);
	ITEM_INFO GetMerchantShopItemInfoBySequence(int __$ReturnUdt);
	int GetNumMerchantMyShopItem();
	int GetMerchantMyShopItemNum(int index);
	void AddMerchantMyShopItem(ITEM_INFO& itemInfo);
	void DecMerchantMyShopItem(ITEM_INFO& itemInfo);
	bool IsExistInMerchantMyShopItemList(int index);
	ITEM_INFO GetMerchantMyShopItemInfoBySequence(int __$ReturnUdt);
	void ClearPurchaseItem();
	int GetNumPurchaseItem();
	int GetPurchaseItemTotalPrice();
	void AddPurchaseItem(ITEM_INFO& itemInfo);
	void DecPurchaseItem(ITEM_INFO& itemInfo);
	int GetPurchaseItemNum(const char* itemName);
	ITEM_INFO GetPurchaseItemInfoBySequence(int __$ReturnUdt);
	ITEM_INFO GetPurchaseItemInfoByName(const char* __$ReturnUdt);
	void ClearMerchantPurchaseItem();
	int GetNumMerchantPurchaseItem();
	int GetMerchantPurchaseItemTotalPrice();
	ITEM_INFO GetMerchantPurchaseItemSaved(int __$ReturnUdt);
	int GetMerchantPurchaseItemNumByIndex(int index);
	void AddMerchantPurchaseItem(ITEM_INFO& itemInfo);
	void DecMerchantPurchaseItem(ITEM_INFO& itemInfo);
	int GetMerchantPurchaseItemNum(const char* itemName);
	ITEM_INFO GetMerchantPurchaseItemInfoBySequence(int __$ReturnUdt);
	void CloneMerchantPurchaseItemToMerchantPurchaseItemSaved();
	ITEM_INFO GetMerchantPurchaseItemInfoByName(const char* __$ReturnUdt);
	void ClearSellItem();
	int GetNumSellItem();
	int IsSellItemValid();
	int GetSellItemNum(int index);
	void AddSellItem(ITEM_INFO& itemInfo);
	void DecSellItem(ITEM_INFO& itemInfo);
	ITEM_INFO GetSellItemInfoBySequence(int __$ReturnUdt);
	ITEM_INFO GetSellItemInfoByIndex(int __$ReturnUdt);
	void ClearStoreItem();
	int GetNumStoreItem();
	void RefreshStoreItemWnd();
	int GetStoreItemNum(int index);
	bool AddStoreItem(ITEM_INFO& itemInfo);
	bool DecStoreItem(ITEM_INFO& itemInfo);
	ITEM_INFO GetStoreItemInfoByIndex(int __$ReturnUdt);
	ITEM_INFO GetStoreItemInfoBySequence(int __$ReturnUdt);
	bool AddMerchantItem(ITEM_INFO& itemInfo);
	bool DecMerchantItem(ITEM_INFO& itemInfo);
	void ClearMerchantItem();
	int GetNumMerchantItem();
	void RefreshMerchantItemWnd();
	int GetMerchantItemNum(int index);
	ITEM_INFO GetMerchantItemInfoByIndex(int __$ReturnUdt);
	ITEM_INFO GetMerchantItemInfoBySequence(int __$ReturnUdt);
	void ClearMerchantMirrorItem();
	int GetNumMerchantMirrorItem();
	void RefreshMerchantMirrorItemWnd();
	int GetMerchantMirrorItemNum(int index);
	void CloneMerchantItemToMerchantMirrorItem();
	bool AddMerchantMirrorItem(ITEM_INFO& itemInfo);
	bool DecMerchantMirrorItem(ITEM_INFO& itemInfo);
	ITEM_INFO GetMerchantMirrorItemInfoByIndex(int __$ReturnUdt);
	ITEM_INFO GetMerchantMirrorItemInfoBySequence(int __$ReturnUdt);
	void RefreshMerchantShopMakeWnd();
	void ClearMerchantHopeToSellItem();
	int GetNumMerchantHopeToSellItem();
	int GetMerchantHopeToSellItemNum(int index);
	bool AddMerchantHopeToSellItem(ITEM_INFO& itemInfo);
	bool DecMerchantHopeToSellItem(ITEM_INFO& itemInfo);
	ITEM_INFO GetMerchantHopeToSellItemInfoByIndex(int __$ReturnUdt);
	ITEM_INFO GetMerchantHopeToSellItemInfoBySequence(int __$ReturnUdt);
	void SetMerchantHopeToSellItemPrice(int sequence, int price);
	bool DeleteCommaInPrice(char* cPrice);
	void ClearExchangeItemList();
	int IsExchangeItemListFull();
	int GetFilledExchangeItemList();
	void AddToExchangeList(ITEM_INFO itemInfo);
	void SetToExchangeList(ITEM_INFO itemInfo);
	bool IsExistIndexInMyChangeList(int index);
	bool MoveItemToExchangeList(ITEM_INFO itemInfo);
	void CancelExchange();
	int GetNumOtherExchangeItems();
	int GetMyExchangeMoney() { return m_myExchangeMoney; }
	int GetOtherExchangeMoney() { return m_otherExchangeMoney; }
	void ClearOtherExchangeItemList();
	int IsOtherExchangeItemListFull();
	void SetMyExchangeMoney(int money);
	void SetOtherExchangeMoney(int money);
	void AddToOtherExchangeList(ITEM_INFO itemInfo);
	void SetToOtherExchangeList(ITEM_INFO itemInfo);
	void CancelExchangeNoRefresh();
	ITEM_INFO GetExchangeItemInfo(int __$ReturnUdt);
	ITEM_INFO GetOtherExchangeItemInfo(int __$ReturnUdt);
	void ClearFriend();
	int GetNumFriend();
	FRIEND_INFO GetFriendInfoBySequence(int __$ReturnUdt);
	void AddFriendToList(FRIEND_INFO& info);
	int SetFriendState(unsigned long aid, unsigned long gid, bool state);
	int DeleteFriendFromList(unsigned long gid);
	void RefreshFriendUI();
	int IsFriendName(const char* itemName);
	int IsCheatName(const char* itemName);
	void HiFriends(int money);
	int IsGuildMember(const char* itemName);
	int IsCheatGuildName(const char* itemName);
	int CompareTwoNames(const char* cName1, const char* cName2);
	void ClearParty();
	int GetNumParty();
	void RefreshPartyUI();
	FRIEND_INFO GetPartyInfoByAid(unsigned long __$ReturnUdt);
	void AddMemberToParty(FRIEND_INFO& info);
	void ChangeRoleFromParty(unsigned long index, int numItem);
	void DeleteMemberFromParty(const char* characterName);
	FRIEND_INFO GetPartyInfoBySequence(int __$ReturnUdt);
	FRIEND_INFO GetPartyInfoByName(const char* __$ReturnUdt);
	unsigned long GetMemberAidFromParty(const char* characterName);
	int GetWeaponType(int index);
	int MakeWeaponType(int head, int job);
	const char* GetShortenItemName(int slot);
	void SetShortenItemName(const char* shortenItemName, int slot);
	void ClearShortCutItemList();
	void SetShortCutItemInfo(char isSkill, int ID, int count, int slot);
	void ClearOtherUserEquipItem();
	bool AddOtherUserEquipItem(ITEM_INFO* itemInfo);
	ITEM_INFO GetOtherUserEquipedItems(int __$ReturnUdt);
	int GetSex();
	void SetSex(int money);
	void SetAid(unsigned long aid) { m_aid = aid; }
	void SetGid(unsigned long gid) { m_gid = gid; }
	unsigned long GetAid() { return m_aid; }
	unsigned long GetGid() { return m_gid; }
	int GetJob();
	int IsThisPC(int index);
	int GetExp() { return m_exp; }
	int GetStr() { return m_str; }
	int GetAgi() { return m_agi; }
	int GetVit() { return m_vit; }
	int GetInt() { return m_int; }
	int GetDex() { return m_dex; }
	int GetLuk() { return m_luk; }
	int GetJobexp();
	int GetWeight() { return m_weight; }
	int GetVirtue();
	int GetGold() { return m_gold; }
	int GetHonor() { return m_honor; }
	int GetLevel() { return m_level; }
	int GetPoint() { return m_point; }
	int GetSpeed() { return m_speed; }
	int GetNextExp() { return m_nextexp; }
	int GetPlusStr() { return m_plusStr; }
	int GetPlusAgi() { return m_plusAgi; }
	int GetPlusVit() { return m_plusVit; }
	int GetPlusInt() { return m_plusInt; }
	int GetPlusDex() { return m_plusDex; }
	int GetPlusLuk() { return m_plusLuk; }
	int GetJoblevel() { return m_joblevel; }
	int GetMaxWeight() { return m_maxWeight; }
	int GetJobNextexp();
	int GetSkillPoint() { return m_skillPoint; }
	int GetASPD();
	int GetPlusASPD();
	int GetAttPower();
	int GetMdefPower();
	int GetItemDefPower();
	int GetPlusdefPower();
	int GetEquipArrowIndex() { return m_equipArrowIndex; }
	int GetHitSuccessValue();
	int GetRefiningPower();
	int GetMax_mattPower();
	int GetMin_mattPower();
	int GetPlusmdefPower();
	int GetAvoidSuccessValue();
	int GetCriticalSuccessValue();
	int GetPlusAvoidSuccessValue();
	const char* GetCharName();
	int GetEffectIgnoreCnt(int index);
	void SetCharName(const char* characterName);
	void SetTextType(bool isShorten, bool isBold);
	char* GetStateFileName(int statusId, int priority);
	int Encode(int head, int job);
	int Decode(int head, int job);
	void ChangeXorValue();
	void SetHp(int newHp)
	{
		int e = Encode(newHp, m_xorValue[m_xorIndex]);
		if (e != m_hp[m_hpIndex]) {
			m_hp[m_hpIndex] = e;
			InvalidateBasicWnd();
		}
	}
	void SetMaxhp(int newMaxhp)
	{
		int e = Encode(newMaxhp, m_xorValue[m_xorIndex]);
		if (e != m_maxhp[m_maxhpIndex]) {
			m_maxhp[m_maxhpIndex] = e;
			InvalidateBasicWnd();
		}
	}
	void SetSp(int newSp)
	{
		int e = Encode(newSp, m_xorValue[m_xorIndex]);
		if (e != m_sp[m_spIndex]) {
			m_sp[m_spIndex] = e;
			InvalidateBasicWnd();
		}
	}
	void SetMaxsp(int newMaxsp)
	{
		int e = Encode(newMaxsp, m_xorValue[m_xorIndex]);
		if (e != m_maxsp[m_maxspIndex]) {
			m_maxsp[m_maxspIndex] = e;
			InvalidateBasicWnd();
		}
	}
	int GetMaxhp() { return Decode(m_maxhp[m_maxhpIndex], m_xorValue[m_xorIndex]); }
	int GetMaxsp() { return Decode(m_maxsp[m_maxspIndex], m_xorValue[m_xorIndex]); }
	int GetHp() { return Decode(m_hp[m_hpIndex], m_xorValue[m_xorIndex]); }
	int GetSp() { return Decode(m_sp[m_spIndex], m_xorValue[m_xorIndex]); }
	int GetHomunHP() { return m_homunHP; }
	int GetHomunSP() { return m_homunSP; }
	int GetHomunMaxHP() { return m_homunMaxHP; }
	int GetHomunMaxSP() { return m_homunMaxSP; }
	const int* GetHomunExpPointer();
	const int* GetHomunNextExpPointer();
	void SetHomunHP(int newHP) { m_homunHP = newHP; InvalidateHomunInfoWnd(); }
	void SetHomunSP(int newSP) { m_homunSP = newSP; InvalidateHomunInfoWnd(); }
	void SetHomunMaxHP(int newMaxHP) { m_homunMaxHP = newMaxHP; InvalidateHomunInfoWnd(); }
	void SetHomunMaxSP(int newMaxSP) { m_homunMaxSP = newMaxSP; InvalidateHomunInfoWnd(); }
	void SetHomunExp(int newExp) { m_homunExp = newExp; InvalidateHomunInfoWnd(); }
	void SetHomunNextExp(int newNextExp) { m_homunNextExp = newNextExp; InvalidateHomunInfoWnd(); }
	void SetHomunHungry(int newHungry) { m_homunHungry = newHungry; InvalidateHomunInfoWnd(); }
	void SetHomunSkillPoint(int value) { m_homunSkillPoint = value; InvalidateUI(); }
	void InvalidateHomunInfoWnd();
	void InitHomunTargetInfo();
	int GetMerHP() { return m_merHP; }
	int GetMerSP() { return m_merSP; }
	int GetMerMaxHP() { return m_merMaxHP; }
	int GetMerMaxSP() { return m_merMaxSP; }
	void SetMerHP(int hp) { m_merHP = hp; InvalidateMerInfoWnd(); }
	void SetMerSP(int sp) { m_merSP = sp; InvalidateMerInfoWnd(); }
	void SetMerMaxHP(int maxHp) { m_merMaxHP = maxHp; InvalidateMerInfoWnd(); }
	void SetMerMaxSP(int maxSp) { m_merMaxSP = maxSp; InvalidateMerInfoWnd(); }
	void InvalidateMerInfoWnd();
	void InitMerTargetInfo();
	const int* GetExpPointer() { return &m_exp; }
	const int* GetJobexpPointer() { return &m_jobexp; }
	const int* GetNextExpPointer() { return &m_nextexp; }
	const int* GetJobnextexpPointer() { return &m_jobnextexp; }
	const int* GetStandardStrPointer() { return &m_standardStr; }
	const int* GetStandardAgiPointer() { return &m_standardAgi; }
	const int* GetStandardVitPointer() { return &m_standardVit; }
	const int* GetStandardIntPointer() { return &m_standardInt; }
	const int* GetStandardDexPointer() { return &m_standardDex; }
	const int* GetStandardLukPointer() { return &m_standardLuk; }
	void InvalidateBasicWnd();
	void InvalidateUI();
	void InvalidateJob();
	void SetGoldNoRefresh(int newGold) { m_gold = newGold; }
	void SetGold(int newGold) { m_gold = newGold; InvalidateUI(); InvalidateBasicWnd(); }
	void SetJobexp(int newJobexp) { m_jobexp = newJobexp; InvalidateUI(); }
	void SetSpeed(int newSpeed) { m_speed = newSpeed; InvalidateUI(); }
	void SetHonor(int newHonor) { m_honor = newHonor; InvalidateUI(); }
	void SetWeight(int newWeight) { m_weight = newWeight; InvalidateUI(); }
	void SetVirtue(int newVirtue) { m_virtue = newVirtue; InvalidateUI(); }
	void SetJobnextexp(int value) { m_jobnextexp = value; InvalidateUI(); }
	void SetMaxWeight(int newMaxWeight) { m_maxWeight = newMaxWeight; InvalidateUI(); }
	void SetStr(int newStr) { m_str = newStr; InvalidateUI(); }
	void SetAgi(int newAgi) { m_agi = newAgi; InvalidateUI(); }
	void SetVit(int newVit) { m_vit = newVit; InvalidateUI(); }
	void SetInt(int newInt) { m_int = newInt; InvalidateUI(); }
	void SetDex(int newDex) { m_dex = newDex; InvalidateUI(); }
	void SetLuk(int newLuk) { m_luk = newLuk; InvalidateUI(); }
	void SetExp(int newExp) { m_exp = newExp; InvalidateUI(); }
	void SetLevel(int newLevel) { m_level = newLevel; InvalidateUI(); }
	void SetPoint(int newPoint) { m_point = newPoint; InvalidateUI(); }
	void SetPlusStr(int value) { m_plusStr = value; InvalidateUI(); }
	void SetPlusAgi(int value) { m_plusAgi = value; InvalidateUI(); }
	void SetPlusVit(int value) { m_plusVit = value; InvalidateUI(); }
	void SetPlusInt(int value) { m_plusInt = value; InvalidateUI(); }
	void SetPlusDex(int value) { m_plusDex = value; InvalidateUI(); }
	void SetPlusLuk(int value) { m_plusLuk = value; InvalidateUI(); }
	void SetSkillPoint(int newSkillPoint) { m_skillPoint = newSkillPoint; InvalidateUI(); }
	void SetNextexp(int value) { m_nextexp = value; InvalidateUI(); }
	void SetStandardStr(int value) { m_standardStr = value; InvalidateUI(); }
	void SetStandardAgi(int value) { m_standardAgi = value; InvalidateUI(); }
	void SetStandardVit(int value) { m_standardVit = value; InvalidateUI(); }
	void SetStandardInt(int value) { m_standardInt = value; InvalidateUI(); }
	void SetStandardDex(int value) { m_standardDex = value; InvalidateUI(); }
	void SetStandardLuk(int value) { m_standardLuk = value; InvalidateUI(); }
	void SetJob(int newJob) { m_job = newJob; InvalidateUI(); InvalidateJob(); }
	void SetJoblevel(int newJoblevel) { m_joblevel = newJoblevel; InvalidateUI(); }
	void SetGuildSkillPoint(int point) { m_guildSkillPoint = point; InvalidateUI(); }
	void SetAttPower(int value) { m_attPower = value; InvalidateUI(); }
	void SetMdefPower(int value) { m_mdefPower = value; InvalidateUI(); }
	void SetItemDefPower(int value) { m_itemDefPower = value; InvalidateUI(); }
	void SetPlusdefPower(int value) { m_plusdefPower = value; InvalidateUI(); }
	void SetRefiningPower(int value) { m_refiningPower = value; InvalidateUI(); }
	void SetMax_mattPower(int value) { m_max_mattPower = value; InvalidateUI(); }
	void SetMin_mattPower(int value) { m_min_mattPower = value; InvalidateUI(); }
	void SetASPD(int value) { m_ASPD = value; InvalidateUI(); }
	void SetPlusASPD(int value) { m_plusASPD = value; InvalidateUI(); }
	void SetPlusmdefPower(int value) { m_plusmdefPower = value; InvalidateUI(); }
	void SetHitSuccessValue(int value) { m_hitSuccessValue = value; InvalidateUI(); }
	void SetAvoidSuccessValue(int value) { m_avoidSuccessValue = value; InvalidateUI(); }
	void SetCriticalSuccessValue(int value) { m_criticalSuccessValue = value; InvalidateUI(); }
	void SetPlusAvoidSuccessValue(int value) { m_plusAvoidSuccessValue = value; InvalidateUI(); }
	unsigned long GetPartyColorBySeq(int seq);
	void SetEquipArrowIndwx(int money);
	ITEM_INFO GetEquipedItems(int __$ReturnUdt);
	TALKTYPE GetNoParamTalkType(const char* command);
	bool IsSecondAttack(int job, int sex, int weapon);
	float GetPCAttackMotion(int job, int sex, int weapon, int isSecondAttack);
	float GetShadowFactorWithJob(int job);
	bool IsProperWeapon(int job, int weapon);
	int IsItemSnap() { return m_isItemSnap; }
	int IsMonsterSnap() { return m_isMonsterSnap; }
	void RefreshMouseSnapState(int money);
	const char* GetRmaName(int slot);
	const char* GetW3DName(int slot);
	bool IsHaveAnotherPalette(int job, char* palName);
	const char* GetBodyPaletteName(int job, int sex, int palNum, char* buf);
	const char* GetHeadPaletteName(int head, int job, int sex, int palNum, char* buf);
	const char* GetJobName(int slot);
	void ReLoadAlwaysNeededRes();
	void PreLoadAlwaysNeededRes();
	int GetShieldType(int index);
	int IsProperShield(int head, int job);
	void RerecordProhibitLog();
	void PrintNumber(CFile* fp, unsigned long number);
	void InitChattingProhibitLog();
	void DeleteAssignAID(unsigned long pingTime);
	void EraseProhibit(unsigned long AID, unsigned long tick);
	void AppendProhibit(unsigned long AID, unsigned long tick);
	int GetChatProhibittedCnt(char* charName);
	int GetChatProhibittedCnt(unsigned long gid);
	void SetSelectedServerName(const char* serverName) { m_selectedServerName = serverName; }
	const char* GetSelectedServerName();
	const char* GetItemResName_WithId(const char* itemName, int isIdentified);
	unsigned long GetItemDisColor(ITEM_INFO* itemInfo);
	bool GetEmblemState() { return (bool&)m_emblemState; }
	void GetEmblemFileName(char* buffer, int gdid, int version);
	void SetEmblemState(unsigned char);
	void SetCompressedEmblemImage(const char* compressedEmblemImage, unsigned long emblemLen, int GDID, int emblemVersion);
	void InitShortCutTextFromReg();
	int IsExistEffectState(int index);
	int IsKorean();
	int IsPc(int index);
	int IsSiegeMode();
	int IsBattleFieldMode();
	int IsAttack(int index);
	int IsMyParty(int index);
	int IsMonster(CGameActor* MobActor);
	int IsMyGuild(int index);
	int IsGuardian(int index);
	int IsPetEgg(int index);
	int IsMasterAid(int index);
	int IsMerOrHomun(int index);
	int IsMer(int index);
	int IsHomun(int index);
	void InitPetTables();
	void InitPetTalkTable();
	void GetPetSpeak(int petJob, int hunger, int act, char* speak);
	void GetPetHungryStateText(int hungry, char* buf);
	void GetPetFriendlyStateText(int hungry, char* buf);
	void GetPetIllustName(int job, int IsAccEquiped, char* fName);
	void GetPetTalkString(char* srcStr, char* bufStr, int petGID, int isChange);
	void GetMonsterSkillInfo(char* Name, int skillId, int& act, int& loopType);
	void GetMonsterTalkInfo(int job, int stateId, int arg1, char* speak, unsigned long gid);
	char* GetPetName(int hunger);
	char* GetPetActText(int hunger);
	char* GetPetAccActName(int hunger);
	char* GetPetHungryText(int hunger);
	int GetPetHungryState(int index);
	int GetPetFriendlyState(int index);
	int GetPetTalkNumber(int job, int act, int hungry);
	int GetPetEmotion(int Hunger, int Friendly, int act) { return m_petEmotionTable[Hunger][Friendly][act]; }
	CGameActor* GetGameActorByAid(unsigned long aid);
	char* GetStateName(int hunger);
	unsigned long GetForceAnimEndTick(unsigned long AID);
	void SetForceAnimEndTick(unsigned long AID, unsigned long tick);
	int IsEFST_Berserk();
	int IsEFST_TrickDead();
	int m_curMapType;
	int m_mapInfoTable[30][15];
	std::list<SKILL_INFO> m_guildSkillList;
	std::list<SKILL_INFO> m_skillInfoList;
	std::list<SKILL_INFO> m_homunSkillList;
	std::list<SKILL_INFO> m_merSkillList;
	std::list<QUEST_INFO> m_QuestList;
	std::list<SKILL_INFO> m_tempSkillList;
	unsigned char m_shortcutSlotCnt;
	std::string m_shortenItemName[27];
	int m_shortenSkillUseLevel[27];
	ShortCutKey m_shortCutList[27];
	std::vector<int> m_skillUseLevelList;
	std::vector<int> m_skillAttackedEffectIdTable;
	std::vector<int> m_skillAttackEffectIdTable;
	std::vector<int> m_guildSkillUseLevelList;
	std::vector<int> m_homunSkillUseLevelList;
	std::vector<int> m_merSkillUseLevelList;
	unsigned long m_targetGID;
	std::vector<CHAT_PROHIBIT_LOG> m_chatProhibitLog;
	std::vector<REQ_ACCOUNT_AID> m_recievedAccountName;
	std::vector<REQ_ACCOUNTNAME_INFO> m_reqAccountInfo;
	std::string m_selectedServerName;
	std::vector<FORCE_ANIM_TICK> m_forceAnimEndTick;


	typedef	map<string,string>					mapDistributionTable;
	typedef	map<string,string>::iterator		mapDistributionTableIterator;
	typedef	map<string,string>::value_type		mapDistributionTableValueType;
	int m_diffTime;
	char m_curMap[20];
	int m_numLatePacket;
	int m_showType;
	unsigned long m_averagePingTime;
	unsigned long m_showDigitTick;
	unsigned long m_killTimeStartTick;
	int m_isShowTime;
	int m_isNeverDie;
	char m_cName[64];
	char m_aName[64];
	TAG_CHARACTER_BLOCK_INFO m_charBlockInfo2[9];
	int m_sex;
	int m_charNum;
	int m_authCode;
	unsigned long m_userLevel;
	unsigned long m_lastLoginIP;
	char m_lastLoginTime[28];
	int m_mkcount;
	int m_haircolor;
	int m_deadcount;
	int m_head;
	int m_weapon;
	int m_shield;
	int m_bodyPalette;
	int m_headPalette;
	int m_accessory;
	int m_accessory2;
	int m_accessory3;
	int m_bodyState;
	int m_healthState;
	int m_effectState;
	int m_posX;
	int m_posY;
	int m_dir;
	char m_camp;
	int m_camp_A;
	int m_camp_B;
	char m_charfont;
	int m_cartCurCount;
	int m_cartMaxCount;
	int m_cartCurWeight;
	int m_cartMaxWeight;
	int m_maxNumOfSellItemOfMerchant;
	int m_attackRange;
	int m_charSlot;
	int m_BgmVolume;
	float m_LoadingTime;
	int m_isShowWhisperWnd;
	int m_isPlayWhisperOpenSound;
	int m_isShowWhisperWnd_Friend;
	int m_isItemSnap;
	int m_isShowGameOver;
	int m_monsterSnapOn_Skill;
	int m_monsterSnapOn_NoSkill;
	int m_isShowTeamGravityPlanetLogo;
	std::vector<char const *> m_newPcJobNameTable;
	std::vector<char const *> m_newPcSexNameTable;
	std::vector<char const *> m_newPcHeadNameTable_M;
	std::vector<char const *> m_newPcHeadNameTable_F;
	std::vector<char const *> m_newPcJobImfNameTable;
	std::vector<char const *> m_newAccessoryNameTable;
	std::vector<char const *> m_pcWeaponNameTable;
	std::vector<char const *> m_pcSexImfNameTable;
	std::vector<char const *> m_pcJobNameTable;
	std::vector<char const *> m_pcSexNameTable;
	std::vector<char const *> m_jobNameTable;
	std::vector<char const *> m_sexNameTable;
	std::vector<char const *> m_shieldNameTable;
	std::list<unsigned long> m_pingTimeList;
	std::vector<char const *> m_palJobNameTable;
	std::vector<std::string> m_shortcutText;
	unsigned long m_petGID;
	int m_isPetNameModified;
	char m_petName[32];
	int m_petAcc;
	int m_petJob;
	int m_petLevel;
	int m_petHungry;
	int m_petFriendly;
	int m_petEggIndex;
	int m_petOldHungry;
	int m_petEmotionTable[5][5][7];
	std::string m_petTalkTable[5][5];
	XMLDocument m_petTalkXmlTable;
	XMLDocument m_monsterSkillInfo;
	XMLDocument m_monsterTalkTable;
	int m_guildDealZeny;
	int m_MaxItemIndex;
private:
	unsigned long m_aid;
	unsigned long m_gid;
	int m_isWeaponBow;
	float m_oneCellDist;
	int m_job;
	int m_exp;
	int m_level;
	int m_point;
	int m_nextexp;
	int m_joblevel;
	int m_skillPoint;
	int m_guildSkillPoint;
	int m_homunSkillPoint;
	int m_merSkillPoint;
	int m_plusStr;
	int m_plusAgi;
	int m_plusVit;
	int m_plusInt;
	int m_plusDex;
	int m_plusLuk;
	int m_str;
	int m_agi;
	int m_vit;
	int m_int;
	int m_dex;
	int m_luk;
	int m_standardStr;
	int m_standardAgi;
	int m_standardVit;
	int m_standardInt;
	int m_standardDex;
	int m_standardLuk;
	int m_ASPD;
	int m_attPower;
	int m_mdefPower;
	int m_plusASPD;
	int m_itemDefPower;
	int m_plusdefPower;
	int m_refiningPower;
	int m_max_mattPower;
	int m_min_mattPower;
	int m_plusmdefPower;
	int m_hitSuccessValue;
	int m_avoidSuccessValue;
	int m_criticalSuccessValue;
	int m_plusAvoidSuccessValue;
	int m_equipArrowIndex;
	int m_gold;
	int m_speed;
	int m_honor;
	int m_maxWeight;
	int m_jobnextexp;
	int m_jobexp;
	int m_weight;
	int m_virtue;
	int m_isMonsterSnap;
	int m_systemDiffTime;
	std::list<ITEM_INFO> m_itemList;
	std::list<ITEM_INFO> m_shopItemList;
	std::list<ITEM_INFO> m_purchaseItemList;
	std::list<ITEM_INFO> m_sellItemList;
	std::list<ITEM_INFO> m_storeItemList;
	std::list<ITEM_INFO> m_merchantItemList;
	std::list<ITEM_INFO> m_merchantShopItemList;
	std::list<ITEM_INFO> m_merchantMirrorItemList;
	std::list<ITEM_INFO> m_merchantMyShopItemList;
	std::list<ITEM_INFO> m_merchantPurchaseItemList;
	std::list<ITEM_INFO> m_merchantHopeToSellItemList;
	std::list<ITEM_INFO> m_merchantPurchaseItemSavedList;
	std::list<FRIEND_INFO> m_partyList;
	std::list<FRIEND_INFO> m_friendList;
	ITEM_INFO m_equipedItems[10];
	ITEM_INFO m_otherUserEquipedItems[10];
	ITEM_INFO m_exchangeItems[10];
	ITEM_INFO m_otherExchangeItems[10];
	int m_myExchangeMoney;
	int m_otherExchangeMoney;
	std::vector<TTPair> m_talkTypeTable;
	std::vector<std::pair<char const *, char const *> > m_mapNameTable;
	std::vector<std::pair<char const *, char const *> > m_mp3NameTable;
	std::vector<std::pair<std::string, std::vector<std::string> > > m_fogParameterTable;
	std::vector<std::pair<std::string, std::vector<std::string> > > m_ViewPointTable;
	std::vector<char const *> m_indoorRswTable;
	std::vector<float> m_shadowFactorTable;
	std::vector<std::string> m_msgStringTable;
	std::vector<std::string> m_weaponSwingWaveNameTable;
	std::vector<std::string> m_jobHitWaveNameTable;
	std::vector<std::string> m_weaponHitWaveNameTable;
	std::list<unsigned long> m_exAidList;
	std::list<std::string> m_exNameList;
	std::vector<std::pair<char const *, bool> > m_enableObjLightMapList;
public:
	std::vector<TTPair> m_cmdOnOffTable;
	std::vector<TTPair> m_cmdExeTable;
	std::vector<TTPair2> m_cmdOnOffSet;
	std::vector<WinPair> m_cmdWinTable;
	std::vector<char const *> m_keyWinTable;
	std::vector<MAIL_LIST> m_mailList;
	MAIL_OPEN m_mailOpen;
	MAIL_LIST m_mailNew;
	ITEM_INFO m_mailItem;
	int m_mailItemCnt;
	unsigned char m_waitForSendMail;
	std::vector<AUCTION_ITEM_SEARCH_INFO> m_auctionList;
	unsigned char m_waitForAddAuction;
	int m_auctionSeachOption;
	int m_auctionSeachOption2;
	std::map<std::string, MapPosInfo> m_mapPosTable;
	std::map<std::string, MapPosInfo>* m_divisionMapPosTable;
	std::map<int, MapPosInfo> m_divisionMapPosIndexTable;
	int m_initMapPos;
	int m_maxNum;
private:
	int m_hpIndex;
	int m_spIndex;
	int m_xorIndex;
	int m_maxhpIndex;
	int m_maxspIndex;
	int m_xorValue[12];
	int m_maxhp[12];
	int m_maxsp[12];
	int m_hp[12];
	int m_sp[12];
	CommonObject m_objParser;
public:
	int m_fogOn;
	int m_isAura;
	int m_isNoShift;
	int m_isNoCtrl;
	int m_isWindow;
	int m_isSkillFail;
	int m_isLogInOut;
	int m_isShopping;
	int m_isQuickSpell;
	int m_isQuickSpell2;
	int m_isMiniGame;
	int m_isQ2begin;
	int m_isNoTalkMsg;
	int m_isNoTalkMsg2;
	int m_isNoTrade;
	int m_isEffectOn;
	int m_isDA_illusionON;
	int m_bgmIsPaused;
	int m_isMinEffect;
	int m_isMinEffectOld;
	int m_isPropertyDisableLockOn;
	int m_isPropertyCountPK;
	int m_isPropertyNoParty;
	int m_amIPartyMaster;
	int m_amIParent;
	int m_isAttrIndicator;
	int m_bMakeMissEffect;
	int m_isLowSystemMemory;
	int m_bAutoOpenDetailWindowIfLowMemory;
	bool m_isShowNameTypeOld;
	unsigned char RandomScreenFlag;
	unsigned char m_isCheckToPcbang;
	int m_SelectAccountNumber;
	int m_isBattle;
	int m_isBattleChat;
	int m_isBmChat;
	int m_isStartBmChat;
	unsigned int m_timeCheckValue;
	unsigned long m_dwOldTime;
	int m_talkType;
	int m_langType;
	int m_expDivType;
	int m_itemCollectType;
	int m_itemDivType;
	std::string m_partyRequestName;
	std::string m_partyName;
	std::string m_accountAddress;
	std::string m_registrationUrl;
	std::string m_myShopName;
	std::string m_accountPort;
	std::string m_lastServerName;
	unsigned char m_emblemState;
	int m_isSaveChat;
	int m_sexCheck;
	int m_cNameCheck;
	unsigned long m_playingTime;
	short m_MightyGauge;
	char m_GaugePacket;
	std::string killName;
	std::string killedName;
	int m_winPoint;
	int m_losePoint;
	_SYSTEMTIME expireTime;
	unsigned long m_homunGID;
	char m_homunName[32];
	int m_homunAtk;
	int m_homunMatk;
	int m_homunHit;
	int m_homunCritical;
	int m_homunDef;
	int m_homunMdef;
	int m_homunFlee;
	int m_homunAspd;
	int m_homunJob;
	int m_homunLevel;
	int m_homunHP;
	int m_homunMaxHP;
	int m_homunSP;
	int m_homunMaxSP;
	int m_homunFriendly;
	int m_homunAcc;
	int m_homunExp;
	int m_homunNextExp;
	int m_homunHungry;
	int m_homunMaxHungry;
	int m_isHomunNameModified;
	unsigned long m_homunEnemyGID;
	int m_homunEnemyX;
	int m_homunEnemyY;
	int m_moveHomunEnemy;
	int m_selectHomunEnemy;
	int m_attackHomunEnemy;
	int m_homunAlive;
	int m_onTraceAI;
	int m_onHoUserAI;
	int m_homunStateId;
	int m_oldHomunHungry;
	int m_homunATKRange;
	unsigned long m_merGID;
	char m_merName[32];
	int m_merAtk;
	int m_merMatk;
	int m_merHit;
	int m_merCritical;
	int m_merDef;
	int m_merMdef;
	int m_merFlee;
	int m_merAspd;
	int m_merHP;
	int m_merMaxHP;
	int m_merJob;
	int m_merLevel;
	int m_merSP;
	int m_merMaxSP;
	int m_merFaith;
	int m_merCallNum;
	int m_merKillCounter;
	int m_merATKRange;
	long m_merExpireDate;
	unsigned long m_merEnemyGID;
	int m_merEnemyX;
	int m_merEnemyY;
	int m_moveMerEnemy;
	int m_onMerUserAI;
	int m_selectMerEnemy;
	int m_attackMerEnemy;
	int m_merAlive;
	int m_merStateId;
	unsigned char m_onAutoMSG;
	std::string m_strAutoMSG;
	unsigned long m_changeNameGID;
	char m_changeCharName[24];
	int m_cashPoint;
	int m_freePoint;
	unsigned char m_battleMsgOnOffSet[13];
	unsigned char m_bRefuseInviteParty;
	int m_onDunInfoWnd;
	int dunInfoWndX;
	int dunInfoWndY;
	int m_dunState;
	int m_tmpDunState;
	std::string m_dunName;
	int m_priority;
	long m_destroyTime;
	long m_enterTimeOut;
	int m_bOpenEquipedItem;
	int m_otherUserJob;
	int m_otherUserSex;
	int m_otherUserHead;
	int m_otherUserAccessory;
	int m_otherUserAccessory2;
	int m_otherUserAccessory3;
	int m_otherUserBodyPalette;
	int m_otherUserHeadPalette;
	char m_otherUserName[24];
	unsigned char m_bIsUAE;
};

extern CSession g_session;

bool IsEffectStateBird(DWORD effectState);
bool IsEffectStateSight(DWORD effectState);
bool IsEffectStateRuwach(DWORD effectState);
bool IsEffectStateBurrow(DWORD effectState);
bool IsEffectStateHiding(DWORD effectState);
bool IsEffectStateStar2(DWORD effectState);
bool IsEffectStateSanta(DWORD effectState);
bool IsEffectStateNothing(DWORD effectState);
bool IsEffectStateChicken(DWORD effectState);
bool IsEffectStateOrcFace(DWORD effectState);
bool IsEffectStateTransform(DWORD effectState);
bool IsEffectStateMarried(DWORD effectState);
bool IsEffectStatePushCart(DWORD effectState);
bool IsEffectStateSpecialHiding(DWORD effectState);
bool IsEffectStateFootPrint(DWORD effectState);
bool IsEffectStateSummer(DWORD effectState);

bool IsEffectStateBirdChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateSpecialHidingChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateSightChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateRuwachChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateBurrowChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateHidingChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateChickenChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateOrcFaceChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStatePushCartChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateStar2Changed(DWORD effectState, DWORD oldEffectState);


#endif
void SetEffectStateSpecialHiding(DWORD *effectState);
void ClearEffectStateSpecialHiding(DWORD *effectState);
void InitTimer(DWORD rate);
