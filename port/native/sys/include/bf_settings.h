/*
 * bf_settings: the one settings object.
 *
 * Every setting of the client lives here: what the Windows client kept in
 * the registry (written there by Setup_HP.exe and by the client itself),
 * the port's own options, the remembered account name and the game folder.
 * It is persisted as one property list,
 *
 *     ~/Library/Application Support/<name>/Settings.plist
 *
 * (under $BF_HOME when that is set). Only values that were set are written;
 * built-in defaults are layered underneath and never stored, so a later
 * change of a default reaches every user who did not change the value.
 *
 * Shape. Settings are grouped like registry keys. A group is a dictionary;
 * inside it, a dictionary is a subgroup and anything else is a value:
 *
 *     <dict>
 *       FormatVersion   integer 1
 *       Options         dict    SkipIntroScreens, FreeCamera, LaunchArguments,
 *                               AttackWithoutCtrl (integer 0/1)
 *       Account         dict    Name (string)
 *       Paths           dict    GameFolder (string, absent by default)
 *       Client          dict    the client's registry root, HKLM\<g_regPath>:
 *                               its values (WIDTH, SOUNDMODE, ...) and its
 *                               subkeys as subgroups (Option, UIRectInfo,
 *                               ShortcutItem, BookMark/<server>/<char>, ...)
 *       Registry        dict    any other registry key the client opens,
 *                               as Registry/<HKLM|HKCU|...>/<subkey path>
 *     </dict>
 *
 * Value types: integer (REG_DWORD, stored as a signed 32-bit value),
 * string (REG_SZ, UTF-8) and data (REG_BINARY).
 *
 * Addressing. A value is named by a group path and a value name. The group
 * path separates groups with "/" (the registry adapter turns "\" into "/");
 * "" is the top level. The value name is used as it is. Both are UTF-8;
 * names are compared without regard to case, as in the registry, and keep
 * the case they were first written with. The registry adapter converts the
 * client's code-page bytes in names and REG_SZ data to UTF-8 and back.
 *
 * The object. bf_settings_shared() is the one object of the process. It is
 * loaded on first use. Writes mark it changed; bf_settings_save writes the
 * file atomically when something changed (the registry adapter calls it on
 * RegCloseKey, the client at exit). A later profile switch gets its own
 * object from bf_settings_open.
 *
 * Every function may be called from any thread; the object locks itself.
 */
#ifndef BF_SETTINGS_H
#define BF_SETTINGS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bf_settings bf_settings;

/* Group and value names the port defines. */
#define BF_SETTINGS_OPTIONS        "Options"
#define BF_OPT_SKIP_INTRO          "SkipIntroScreens"   /* default 1: skip the licence and intro screens */
#define BF_OPT_FREE_CAMERA         "FreeCamera"         /* default 1: no limits on zoom and vertical rotation */
#define BF_OPT_LAUNCH_ARGUMENTS    "LaunchArguments"    /* default 1: server/account/character arguments go straight in */
#define BF_OPT_ATTACK_WITHOUT_CTRL "AttackWithoutCtrl"  /* default 0: as the original (/noctrl still works) */
#define BF_SETTINGS_ACCOUNT        "Account"
#define BF_SETTING_ACCOUNT_NAME    "Name"
#define BF_SETTINGS_PATHS          "Paths"
#define BF_SETTING_GAME_FOLDER     "GameFolder"
#define BF_SETTINGS_CLIENT         "Client"             /* the client's registry root */
#define BF_SETTINGS_REGISTRY       "Registry"           /* every other registry key */

typedef enum bf_setting_type {
	BF_SETTING_NONE   = 0,
	BF_SETTING_INT    = 1,
	BF_SETTING_STRING = 2,
	BF_SETTING_DATA   = 3
} bf_setting_type;

/* The settings of the process (default profile). Never NULL. */
bf_settings *bf_settings_shared(void);

/*
 * The settings of a named profile, stored as Settings-<profile>.plist next
 * to the default file; NULL or "" is the shared object. Objects live until
 * the process ends.
 */
bf_settings *bf_settings_open(const char *profile);

/*
 * A settings object on an explicit property-list path (tests, tools). The
 * file need not exist. Free with bf_settings_close; the shared object must
 * not be closed.
 */
bf_settings *bf_settings_open_file(const char *plist_path);
void bf_settings_close(bf_settings *settings);

/* The file the object is stored in. */
const char *bf_settings_path(bf_settings *settings);

/* Rereads the file, dropping unsaved changes. 1 if read, 0 if absent (empty settings), -1 if unreadable. */
int bf_settings_load(bf_settings *settings);
/* Writes the file if anything changed since the last load or save. 1 written, 0 nothing to do, -1 error. */
int bf_settings_save(bf_settings *settings);
/* Forgets every stored value; the defaults remain. */
void bf_settings_reset(bf_settings *settings);

/* ---- Groups (registry keys) ---------------------------------------------- */

/* 1 if the group exists in the stored values or the defaults (RegOpenKeyEx). */
int bf_settings_has_group(bf_settings *settings, const char *group);
/* Creates the group and its parents if absent (RegCreateKeyEx). 1, or 0 if a value is in the way. */
int bf_settings_create_group(bf_settings *settings, const char *group);

/*
 * Removes a value, or with name NULL the group and everything in it
 * (RegDeleteValue, RegDeleteKey). Defaults cannot be removed: a removed value
 * that has a default reads as the default again. 1 if something was removed.
 */
int bf_settings_remove(bf_settings *settings, const char *group, const char *name);

/*
 * The index-th value of a group in stored order, defaults included
 * (RegEnumValue). Copies its name (UTF-8, NUL-terminated) and returns its
 * type, or BF_SETTING_NONE past the end or if name_size is too small.
 */
bf_setting_type bf_settings_value_at(bf_settings *settings, const char *group, int index,
                                     char *name, size_t name_size);

/* ---- Values --------------------------------------------------------------- */

/* The type of a value (stored, else default), BF_SETTING_NONE if absent. */
bf_setting_type bf_settings_type(bf_settings *settings, const char *group, const char *name);

/* 1 and *out set if present as an integer, else 0 and *out untouched. */
int bf_settings_get_int(bf_settings *settings, const char *group, const char *name, int32_t *out);
/* The value or fallback. */
int32_t bf_settings_int(bf_settings *settings, const char *group, const char *name, int32_t fallback);

/*
 * A string value as UTF-8. Returns 1 and copies it NUL-terminated; 0 if
 * absent or not a string; -1 if out_size is too small (nothing copied).
 * *length (may be NULL) gets the byte length without the NUL in every
 * case where the value exists.
 */
int bf_settings_get_string(bf_settings *settings, const char *group, const char *name,
                           char *out, size_t out_size, size_t *length);

/* A data value: as bf_settings_get_string without the NUL. out may be NULL to ask for the length. */
int bf_settings_get_data(bf_settings *settings, const char *group, const char *name,
                         void *out, size_t out_size, size_t *length);

/*
 * Setters create the group path as needed and replace a value of any type.
 * They return 1, or 0 if the string is not UTF-8 or a group of that name is
 * in the way (with a trace line).
 */
int bf_settings_set_int(bf_settings *settings, const char *group, const char *name, int32_t value);
int bf_settings_set_string(bf_settings *settings, const char *group, const char *name, const char *utf8);
int bf_settings_set_data(bf_settings *settings, const char *group, const char *name,
                         const void *data, size_t size);

/* ---- Conveniences ---------------------------------------------------------- */

/* An option of the Options group as 0/1 (BF_OPT_*), with its default. */
int bf_settings_option(bf_settings *settings, const char *option);
int bf_settings_set_option(bf_settings *settings, const char *option, int on);

/* The remembered account name (UTF-8); "" when none. Same return values as bf_settings_get_string. */
int bf_settings_account_name(bf_settings *settings, char *out, size_t out_size);
int bf_settings_set_account_name(bf_settings *settings, const char *utf8);

#ifdef __cplusplus
}
#endif

#endif
