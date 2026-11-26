/*
 * bf_setting_key.h: the client's settings keys on bf_settings (M11.4).
 *
 * The 2008 client kept its settings in the registry below
 * HKEY_LOCAL_MACHINE\<g_regPath>; it still names them that way: a key is a
 * path in the client's code page, "\" between the parts, such as
 * g_regPath + g_uiPath. These calls take that path and map it onto the one
 * settings object (port/native/sys/include/bf_settings.h), exactly as the
 * registry adapter of stage A did, so that every Settings.plist written
 * before keeps its keys and value types:
 *
 *     Software\<name>[\sub\key]   group Client[/sub/key]   (g_regPath)
 *     any other path              group Registry/HKLM/<path>
 *
 * Path parts, value names and string values are converted between the
 * client's code page (bf_file_user_codepage) and UTF-8. Two values of the
 * client's root are not stored there:
 *
 *     ID         the login window's remembered account: Account/Name
 *     PASSWORD   the Keychain item of that account (bf_sys_keychain_*)
 *
 * and CHECKSAVE ("keep") belongs to the build that set it: writing 1 records
 * the build's identity (bf_sys_code_identity) as Account/SavedBy, writing 0
 * removes it. Another build (every rebuild of an ad hoc signed client) reads
 * CHECKSAVE as 0 and PASSWORD as absent without asking the Keychain, and
 * does not delete the other build's item; the Keychain's access prompt can
 * then come only from storing the password after "keep" is ticked again.
 *
 * Values keep the registry's typing: a 4-byte integer write is an integer,
 * a string write a string (the text up to its first NUL within the size),
 * anything else data. A read copies the value into the caller's buffer as
 * RegQueryValueEx did: an integer as 4 bytes, a string in the code page
 * with its NUL, data as stored; it fails, copying nothing, when the value is
 * absent or does not fit, and then *size holds the size needed.
 *
 * bf_setting_close saves the settings file when something changed (the
 * client also saves at exit). C++ only: the client's units include it.
 */
#ifndef BF_SETTING_KEY_H
#define BF_SETTING_KEY_H

#include <string>

#include "../../native/sys/include/bf_settings.h"

/* An open key (what the client kept in an HKEY). */
struct bf_setting_key {
	std::string group;   /* the bf_settings group, UTF-8 */
	bool root;           /* the client's root: ID and PASSWORD live elsewhere */
	bool open;
	bf_setting_key() : root(false), open(false) {}
};

/* Opens the key at path; false (key not open) when the group does not exist. */
bool bf_setting_open(bf_setting_key &key, const char *path);
/* Opens the key at path, creating it and its parents when absent. */
bool bf_setting_create(bf_setting_key &key, const char *path);
/* Closes the key and saves the settings when something changed. A key that is not open is left alone. */
void bf_setting_close(bf_setting_key &key);
/* Removes the key at path with everything below it; false when absent. */
bool bf_setting_remove(const char *path);

/*
 * Reads a value into data (*size bytes); on return *size is the value's
 * size. data NULL asks for the size only. False when the key is not open,
 * the value is absent, or it does not fit (nothing copied).
 */
bool bf_setting_read(const bf_setting_key &key, const char *name, void *data, unsigned int *size);
/* Writes a value: BF_SETTING_INT with size 4 an integer, BF_SETTING_STRING a string, else data. */
bool bf_setting_write(const bf_setting_key &key, const char *name, bf_setting_type type, const void *data,
                      unsigned int size);

/* A 4-byte value (DWORD, int, float: its bytes): read with a 4-byte buffer. */
template <class T>
inline bool bf_setting_get(const bf_setting_key &key, const char *name, T &value)
{
	static_assert(sizeof(T) == 4, "bf_setting_get reads 4 bytes");
	unsigned int size = 4;
	return bf_setting_read(key, name, &value, &size);
}

/* A 4-byte value written as an integer (its bytes, as REG_DWORD). */
template <class T>
inline bool bf_setting_set(const bf_setting_key &key, const char *name, const T &value)
{
	static_assert(sizeof(T) == 4, "bf_setting_set writes 4 bytes");
	return bf_setting_write(key, name, BF_SETTING_INT, &value, 4);
}

/* A string: the text up to its first NUL within size bytes. */
inline bool bf_setting_set_string(const bf_setting_key &key, const char *name, const char *text, unsigned int size)
{
	return bf_setting_write(key, name, BF_SETTING_STRING, text, size);
}

#endif
