# Files the login screen asks for and does not find

Implementation plan M4, last "Done when" criterion: every interface file the login screen asked for and did not find is listed, with a replacement or a decision.

## How the list was taken

The M4 frame script ([`port/harness/scripts/m4_frames.py`](../../port/harness/scripts/m4_frames.py)) was run with the file and resource traces on, from start-up to the login screen and through five resizes:

```bash
BF_TRACE=err,stub,gfx,mode,file,res,ui BF_CLIENT=build/cmake/debug/bifrost python3 port/harness/run.py m4_frames
```

Every lookup of `CFileMgr` is traced with where it was found (`folder` for the game's loose `data/` folder, `archive` for `rdata.grf` or `data.grf`) or as `MISSING`; a resource or texture that does not load is counted as `res.missing` (M3.5). At the end of the run the counters were `file.missing 3`, and there was no `res.missing` and no placeholder. The game data is kRO's of June 2021 (`rdata.grf` before `data.grf`) with the loose tables of `data/`.

## Interface files: none missing

The login screen (service type America, sub-mode `LSM_IDENTRY`, licence and intro skipped by `SkipIntroScreens`) asked for these interface files, and every one was found in an archive:

| File (under `data\texture\유저인터페이스\` unless a path is given) | Used by |
|---|---|
| `bgi_temp.bmp` | `CLoginMode` wallpaper (`TITLE_FILE`), drawn only in `LSM_WARNING` for this service type |
| `t2_배경1-1.bmp` … `t2_배경3-4.bmp` (12) | `UIWindowMgr::RenderTitleGraphic`, the background tiles |
| `t_테입1.tga` … `t_테입4.tga`, `t_12세.tga`, `t_중력성인.tga`, `t_r1.tga` … `t_r4.tga` | `RenderTitleGraphic`: tapes, age rating, developer's mark, logo |
| `login_interface\win_login.bmp` | `UILoginWnd`, the window |
| `login_interface\btn_connect{,_a,_b}.bmp`, `login_interface\btn_exit{,_a,_b}.bmp`, `btn_cancel{,_a,_b}.bmp` | `UILoginWnd`'s buttons |
| `login_interface\chk_saveon.bmp`, `chk_saveoff.bmp` | the "save ID" check box |
| `data\sprite\cursors.spr`, `.act` | the cursor (`CMode::DrawMouseCursor`) |
| `data\sprite\shadow.spr`, `.act`, `data\sprite\이팩트\숫자.spr`, `.act` | loaded at start-up for game mode |

Not asked on this path, checked to exist so that the screens the defaults skip still find their art: `login_interface\warning.bmp` (licence/warning wallpaper, `LSM_WARNING`), `login_interface\btn_intro{,_a,_b}.bmp` and `btn_request{,_a,_b}.bmp`. The intro button is made only when `..\openning.bik` exists, which it does not in our data (progress log, M12.4): decision, the button stays hidden until the intro movie is found.

**The labels are Korean** because kRO's interface bitmaps carry their text (`win_login.bmp`, the button bitmaps). That is not a missing file: the English bitmaps are M9's business (translation of the interface), not a replacement here.

## The other files the login screen asked for and did not find

| File | Asked by | Decision |
|---|---|---|
| `data\..\ftwrk.lex` | `framework/ftwbrk.cpp`, the Thai word-break lexicon (`fLoadLexicon`) | none needed: it is used only to break Thai text into lines; the port's service type is not Thai, and nothing else reads it |
| `data\MonsterTalkTable.xml` | `CSession` start-up (`Session2.cpp`), the lines monsters say | none: the session skips the table when it is absent and monsters then say nothing. Neither archive has it; ROenglishRE's `Translation/Renewal/data/monstertalktable.xml` is an empty table (`<enemy_monster_talk_table>` with no entries), so copying it would change nothing but the trace line. Revisit with M8 (monster chat is not a gate criterion) |
| `<game folder>\ProhibitionLog.gd` | `CSession`, the player's whisper block list, read at start-up | none: the file is written when the player blocks someone and is absent on a first run. Note for the system adapter: the path is built as `<game folder>` + `\ProhibitionLog.gd` (a backslash after an absolute macOS path); that it resolves when the file is written is to be checked when the block list is tested (M8) |
