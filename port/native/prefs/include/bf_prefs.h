/*
 * bf_prefs: the game-folder chooser and the settings window.
 *
 * The application started from Finder has no environment and no command
 * line, so it needs two pieces of native UI before and beside the game:
 *
 *   - the game-folder chooser, run once at start-up before the game loop:
 *     when the configured folder (bf_file_location(BF_LOCATION_GAME): the
 *     setter, $BF_CLIENT_DIR, the setting Paths/GameFolder, ~/Games/<name>)
 *     holds no usable archive, it explains why and asks for a folder with an
 *     NSOpenPanel run modally;
 *   - the settings window, one plain window opened from the application
 *     menu ("Settings...", Cmd+,): game folder, UI scale, the four
 *     convenience options, music and effect volume, the remembered account
 *     name and "Forget Saved Password".
 *
 * Every change in the window is written to bf_settings_shared() at once and
 * saved, and the change callback names the value that changed, so that the
 * client can apply UI scale and volume live. The window is an ordinary
 * NSWindow: nothing here calls [NSApp run]; it is driven by bf_app_pump()
 * in the client's loop. bf_app_init() must have run first.
 *
 * The game folder is only read: validation lists it and reads the first
 * 16 bytes of its archives.
 *
 * Threads: every function runs on the main thread.
 *
 * C header, usable from C and C++14. No AppKit types cross it; the window
 * crosses as void *.
 */
#ifndef BF_PREFS_H
#define BF_PREFS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Names of the values the window writes -------------------------------
 *
 * Besides the BF_OPT_* options, Account/Name and Paths/GameFolder of
 * bf_settings.h, the window writes these. The volumes are the client's own
 * registry values (CSession::InitOptionWithRegistry), 0..127 like its
 * option window and /bv, /v.
 */
#define BF_PREFS_DISPLAY         "Display"
#define BF_PREFS_UI_SCALE        "UIScale"        /* integer 1, 2 or 3; absent: bf_prefs_ui_scale() decides */
#define BF_PREFS_CLIENT_OPTION   "Client/Option"
#define BF_PREFS_MUSIC_VOLUME    "streamVolume"   /* integer 0..127, default 100 */
#define BF_PREFS_EFFECT_VOLUME   "soundVolume"    /* integer 0..127, default 100 */
#define BF_PREFS_VOLUME_MAX      127
/* Not a stored value: the name reported in group Account when the saved password was deleted. */
#define BF_PREFS_SAVED_PASSWORD  "SavedPassword"

/* ---- The game folder -------------------------------------------------- */

/*
 * The archives the check opens in place of the client's own list, in its
 * search order (file names, compared without regard to case; copied). With
 * names NULL / count 0 (the default), the check uses the list the client
 * builds (WinMain.cpp, AddPaksFromDataIni): the numbered entries of [Data]
 * in the folder's DATA.INI, else data.grf alone.
 */
void bf_prefs_set_archives(const char *const *names, int count);

/*
 * 1 if path (absolute UTF-8) is a usable game folder:
 *  - a path of at most BF_FILE_FOLDER_MAX bytes (bf_file.h; checked first,
 *    the reason names the length and the limit), and a full one (the
 *    client is started from anywhere: Finder starts it in /);
 *  - a readable folder;
 *  - at least one of the archives above is there, and each one there opens
 *    as the client opens it: the GRF header, a version 0x1xx or 0x2xx, and
 *    for 0x2xx a file table that unpacks;
 *  - it holds, loose below the folder or in one of those archives, the
 *    files the start cannot do without: data/sclientinfo.xml (the service
 *    and the login server; without it the client is Korean and dials
 *    Gravity's server), data/msgstringtable.txt (every interface text),
 *    and data/sprite/몬스터/poring.spr and .act (the default sprite, which
 *    CResMgr::Get draws for every missing one). Names are compared without
 *    regard to case or Unicode normalisation; a link counts as what it
 *    points to. The names of a 0x1xx archive are encrypted and not read:
 *    such an archive may hold them.
 * Everything else may be missing: the client draws placeholders.
 * 0 if not, with the reason as one English sentence in reason (may be
 * NULL), for showing to the user; it names what is missing. Reads the
 * folder listing, DATA.INI, and each archive's header and file table
 * (about 0.1 s for the 2021 data.grf).
 */
int bf_prefs_game_folder_valid(const char *path, char *reason, size_t reason_size);

/*
 * Validates path and stores it as Paths/GameFolder (saved at once), and
 * tells the change callback. The folder in use by this process does not
 * change: the client reads it at start-up only. 1 if stored, 0 if the
 * folder is not valid (reason as above).
 */
int bf_prefs_set_game_folder(const char *path, char *reason, size_t reason_size);

/*
 * The start-up chooser. If the configured game folder is valid, copies it
 * to out and returns 1 at once, showing nothing. Otherwise it shows the
 * reason and an open panel for folders (modal, before the game loop),
 * refuses folders that are not valid while the panel is open, stores the
 * choice in Paths/GameFolder, makes it this process's game folder
 * (bf_file_set_game_folder, so that it also wins over $BF_CLIENT_DIR), and
 * returns 1 with the path in out.
 *
 * 0: the user cancelled; the client should quit cleanly.
 * -1: no folder and no way to ask (no window-server session or
 *     $BF_NO_ALERTS set, as for bf_sys_alert; or out too small), with a
 *     BFT_ERR trace line naming the reason.
 */
int bf_prefs_choose_game_folder(char *out, size_t out_size);

/*
 * The chooser's panel on its own, whatever the configured folder: reason
 * (UTF-8, may be NULL) is shown above the explanation. Same results as
 * bf_prefs_choose_game_folder, which calls it.
 */
int bf_prefs_ask_game_folder(const char *reason, char *out, size_t out_size);

/* ---- UI scale ---------------------------------------------------------- */

/*
 * The UI scale to use: Display/UIScale when it is set (clamped to 1..3),
 * else the main screen's backing scale rounded (2 on a Retina display, so
 * that the original 1024x768 UI keeps its size in points), clamped to 1..3.
 */
int bf_prefs_ui_scale(void);

/* ---- The settings window ------------------------------------------------ */

/*
 * Called after the window wrote a value: group and name as in bf_settings
 * (BF_SETTINGS_OPTIONS / BF_OPT_*, BF_PREFS_DISPLAY / BF_PREFS_UI_SCALE,
 * BF_PREFS_CLIENT_OPTION / BF_PREFS_MUSIC_VOLUME or _EFFECT_VOLUME,
 * BF_SETTINGS_ACCOUNT / BF_SETTING_ACCOUNT_NAME or BF_PREFS_SAVED_PASSWORD,
 * BF_SETTINGS_PATHS / BF_SETTING_GAME_FOLDER). The value is already in
 * bf_settings_shared(). A volume slider reports every step while it is
 * dragged. NULL removes the callback.
 */
typedef void (*bf_prefs_changed_fn)(void *context, const char *group, const char *name);
void bf_prefs_set_change_callback(bf_prefs_changed_fn fn, void *context);

/*
 * Adds "Settings..." with Cmd+, at the top of the application menu that
 * bf_app_init built (the first item of NSApp.mainMenu), followed by a
 * separator. Calling it again does nothing. 1, or 0 when there is no main
 * menu yet.
 */
int bf_prefs_install_menu_item(void);

/*
 * Opens the settings window (creating it on first use), with every control
 * read again from the settings, in front and key. Returns 1, or 0 if
 * bf_app_init has not run.
 */
int bf_prefs_show_settings(void);

/* Closes the window if it is open; pending text edits are written first. */
void bf_prefs_close_settings(void);

/* Non-zero while the settings window is on screen. */
int bf_prefs_settings_visible(void);

/*
 * The settings window (NSWindow *), created if needed and refreshed from the
 * settings, but not shown: for tests and for rendering it offscreen. Its
 * controls carry identifiers (README.md lists them).
 */
void *bf_prefs_settings_window(void);

#ifdef __cplusplus
}
#endif

#endif
