# prefs: game-folder chooser and settings window

The native UI the application needs once it starts from Finder (step
M12.2): a chooser for the game folder when none usable is configured, and a
settings window for what a player changes. Built on `sys` (settings, game
folder, Keychain) and `app` (the window and the pump). The contract is
[`include/bf_prefs.h`](include/bf_prefs.h).

## Build and test

```
cmake -S port/native/prefs -B build/native/prefs -G Ninja
cmake --build build/native/prefs
build/native/prefs/bifrost-prefs-test                 # the checks, one line each (about 1 s)
build/native/prefs/bifrost-prefs-test --interactive   # the real chooser panel, then the window; Cmd+Q ends
```

The CMake project adds `sys` and `app` as subdirectories unless the parent
already has the targets. The test needs no person and no game data:

- `BF_HOME` is pointed at `build/native/prefs/prefs-test/home`, so the
  settings file is written there and never in the real Application Support.
- The folders it validates are made under `prefs-test/folders`.
- The chooser runs with `BF_NO_ALERTS`, so its panel is never shown.
- The window's controls are set as a click would (`performClick:`,
  `setSelectedSegment:` + `sendAction:`, `setIntegerValue:` + `sendAction:`).
  Each value is then read back from the plist file on disk, and the change
  callback is checked.
- It renders the window, title bar included, to `prefs-test/settings-light.png`
  and `settings-dark.png`. The copies in `test/reference/` show the window
  as a new user sees it: defaults, the default game folder, the account
  name "player".

## Start-up order for the client

```
bf_app_init(...)                       app module; the chooser and the window need NSApp
bf_app_set_clock(bf_clock_ms)
bf_prefs_set_archives(list, n)         optional: the client's archive list, once read
switch (bf_prefs_choose_game_folder(folder, sizeof folder)) {
    case 1:  break;                    valid (configured, or chosen and stored)
    case 0:  quit cleanly;             the user cancelled
    case -1: quit with the trace line; no folder and nobody to ask
}
... first file access (bf_file_*, the archives) ...
bf_prefs_install_menu_item()           "Settings..." (Cmd+,) in the application menu
bf_prefs_set_change_callback(fn, ctx)  apply UI scale and volume live
ui_scale = bf_prefs_ui_scale()         for the first bf_gfx_resize
loop: bf_app_pump(0); bf_app_poll_event(...) ...
```

The chooser must run before the first file access. When it picks a
folder, it calls `bf_file_set_game_folder`, so the chosen folder wins over
`$BF_CLIENT_DIR` for this run. A scripted run that sets `BF_CLIENT_DIR` to
a valid folder never sees the chooser.

## What it does

- **Validation** (`bf_prefs_game_folder_valid`): the path must be a readable
  folder. It must hold one of the archives (`bf_prefs_set_archives`; until
  the client names its list, any `*.grf`), compared without regard to case,
  whose first 15 bytes are `Master of Magic`. The reason for a refusal is one
  English sentence, shown to the user. Only the folder listing and 16 bytes
  per archive are read.
- **The chooser** (`bf_prefs_choose_game_folder`): when the configured folder
  (setter, `BF_CLIENT_DIR`, `Paths/GameFolder`, `~/Games/<name>`) is valid,
  it returns at once. Otherwise it activates the app and runs an
  `NSOpenPanel` for folders modally. The panel's message gives the reason
  and what is wanted, and it starts in `~/Games` when that exists. The
  panel's delegate refuses an invalid folder while the panel is still open,
  and shows the reason. The choice is stored in `Paths/GameFolder`, saved,
  and set for this process. It never runs without a window-server session or
  with `BF_NO_ALERTS` (returns -1), the same rule `bf_sys_alert` follows.
- **The settings window**: one non-resizable window titled "Settings", laid
  out with an `NSGridView`. Every change is written with `bf_settings_*` and
  saved at once. While a volume slider is dragged, the value is set and
  reported at every step, and saved when the drag ends.

  | Row | Control (identifier) | Writes |
  |---|---|---|
  | Game folder | label `gameFolder` (path with `~`, full path in the tooltip), button `chooseGameFolder` (a sheet with the same validating panel) | `Paths/GameFolder`, used from the next start |
  | UI scale | segmented `uiScale` 1× 2× 3× | `Display/UIScale` |
  | Options | checkboxes `SkipIntroScreens`, `FreeCamera`, `LaunchArguments`, `AttackWithoutCtrl` | `Options/<name>` |
  | Music volume | slider `musicVolume` 0–127, label `musicVolumeValue` | `Client/Option/streamVolume` |
  | Effects volume | slider `effectVolume` 0–127, label `effectVolumeValue` | `Client/Option/soundVolume` |
  | Account | text field `accountName` (trimmed; written on Return or when focus leaves) | `Account/Name` |
  | | button `forgetPassword` | deletes the Keychain item of that name; reports `Account/SavedPassword` |
  | | label `status` | the outcome of Choose and Forget |

  The window re-reads every control when it is opened. A value the client
  changed in the meantime (its own option window, `/bv`) therefore shows
  correctly.
- **The menu item** (`bf_prefs_install_menu_item`): "Settings…" with Cmd+,
  at the top of the application menu that `bf_app_init` built, followed by a
  separator.
- **UI scale default**: with no `Display/UIScale` set, `bf_prefs_ui_scale`
  returns the main screen's backing scale, rounded and clamped to 1–3. On a
  Retina display that is 2, so the original 1024×768 UI keeps its size in
  points.

## Notes for the client (M4.3, M7, M12.1)

- The volumes are the client's own registry values, on the client's own
  0–127 scale. They are the values its option window and `/v` and `/bv`
  use, and that `CSession::WriteOptionToRegistry` writes at exit. On the
  callback for `Client/Option/streamVolume`, the client should set
  `g_session.m_BgmVolume` and call `SetStreamVolume`. For `soundVolume`, it
  should call `Set2DVolume`/`Set3DVolume`. Otherwise the client writes its
  old value back at exit.
- The callback for `Display/UIScale` should run the resize path with the
  new scale.
- The options take effect where the client reads them. A change during play
  may only take effect at the next start (for example `SkipIntroScreens`).
- `Display/UIScale` and the volume keys are defined in this header. They
  are not in `bf_settings.h`, which this package did not change.

## Untested here

- **The chooser's panel itself**: this Mac's screen is locked during the
  run and nobody can answer a modal panel. The checks cover the fast path,
  the refusal without UI, and validation. The panel's delegate calls the
  same validation function. `--interactive` shows the real panel.
- **The "Choose…" sheet**, for the same reason. Storing a folder
  (`bf_prefs_set_game_folder`, which the sheet calls) is checked.
- **Forget Saved Password with a real item**: the login keychain is locked
  in this background session, so the check is skipped. The check stores an
  item under a test account, presses the button and reads the item back.
  It runs where `bf_sys_keychain_available()` is 1.
- **Typing** into the account field: the test sets its text and sends its
  action, as Return does.
- In the light reference picture, the inactive window's title is drawn too
  faint to see. That comes from the offscreen render; the dark one shows it.

## For `port/README.md`

> **Preferences module** (`port/native/prefs`, M12.2). The game-folder
> chooser (validates for a GRF archive, `NSOpenPanel` at start-up when the
> configured folder is unusable) and the settings window (game folder, UI
> scale, the four options, volumes, account name, Forget Saved Password;
> Cmd+,). Test: `cmake -S port/native/prefs -B build/native/prefs -G Ninja
> && cmake --build build/native/prefs && build/native/prefs/bifrost-prefs-test`
> (about 1 s, opens a window briefly, no game data, settings under the build
> folder); `--interactive` shows the real panel and window.
