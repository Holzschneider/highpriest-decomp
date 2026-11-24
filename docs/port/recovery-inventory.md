# Recovery inventory

Every place outside the single-player files that refers to the single-player game *HighPriest*, with a verdict per site: **added** (deleted in M1.2) or **replaced** (restored in M1.3 to M1.5, with the evidence used). Steps M1.1 to M1.5 of the [implementation plan](implementation-plan.md); the method is in the [technical plan](technical-plan.md), section 3.2.

The list was taken from the source before the removal (`macos-port` at 5b896c6) with

```bash
port/tools/inventory.py                      # one line per site: file:line: [enclosing function] names: text
port/tools/inventory.py --names              # the derived name lists
port/tools/inventory.py --markdown --verdicts port/tools/inventory-verdicts.tsv   # the table below
```

The tool derives its names from the five files as they are on `main` (`--names-from`), so it keeps working after the files are gone; on the recovered tree it reports nothing and exits 0.

## How the names are derived

All mechanically, from the sources; `inventory.py` documents each rule.

| Kind | Rule | Count |
|---|---|---:|
| functions | every function defined at file scope in `HP_Battle.cpp`, `HP_BuySell.cpp`, `HP_RestTravell.cpp`, `HP_SaveLoad.cpp`, `HP_Tetris.cpp` (the method name) and the class of `HP_Tetris.h`; names also defined outside the five files are left out | 55 (45 `UIWindowMgr` methods, the `Tetris` class with its constructor, destructor and 7 methods, `EndingStrike`) |
| globals | every variable the five files declare at file scope, with or without `extern`, that at most four other files name | 84 (left out as online: `g_session g_modeMgr g_windowMgr g_mouse g_renderer g_isSkipFrame`) |
| tagged | identifiers the five files use that carry the game's tag (`HP_…`, `…_HP`) and are defined outside them, and every function, type or file-scope constant defined with such a name | 9: `HP_AddItem HP_EquipItemById HP_MakingItem HP_GetLocation HP_TRAVEL_AVAILABLE HP_Damage_Moment HP_ST_SLEEP Zc_Notify_Entry_HP Zc_BroadCast_HP` |
| read | online-class functions whose whole body is single-player, found by reading; they only seed the next rule | 2: `CGameMode::ProcessDamageSituation`, `CGameMode::GainExp` |
| closure | functions defined outside the five files, class members and file-scope variables that the single-player code uses and nothing else does (every other line naming them is a declaration, their own definition, or inside a single-player body); repeated to a fixed point. Inline accessors in class bodies and type names are not candidates | 11: `DeleteBroadcastBalloon DoPCHaveItem DrawBackGround2D GetWeaponCost MakeSkill Mob_SkillReturn RemoveSkillbyJT SetDamageSituation Zc_Notify_Entry_Mob m_DayCount g_DM_List` |
| patterns | `GetGameActorByAid(<literal>)` and `GetGameActorByAID(<literal>)`: the single-player game gives its actors fixed ids (1 the acolyte, 2 the blacksmith, 100 the boy); an online client knows only ids the server sent | 11 sites |
| strings | string literals naming the game: `HighPriest` in any case, `data_hp`, a name ending in `_HP` | 7 sites |

Globals (84): `g_HelpNextActTime g_MainNextActTime g_MobNextOutTime g_curStance g_curScreen g_BattleEvent g_NextSkill map_BNPC_ID g_ScreenConv_Alpha g_FieldConv_Alpha g_AcoPosition g_curStory map_Number g_used_Skill g_Mob_used_Skill map_firstway map_BNPC_Pos map_BNPC_done map_BNPC_event Boy_talked map_secondstart map_secondway map_thirdstart map_thirdway map_BOSS_done map_Heal_Used map_ComboF_Used g_Frame g_PoringCurX g_PoringCurY g_PoringTarY g_PoringTarX g_menuSelect g_curRectTop g_curStance_B g_SpellSeek g_SpellSeekTime g_curHardness g_curSave g_menuFreeze g_selected_item g_BS_phase g_BS_NextActTime g_curEnding g_MobNextTimeFixed g_PrevMob Boy_frame bat_ChookGi bat_STUN_turn bat_mobskill_turn bat_mouse_pos g_Aco_Job Boy_friendly Boy_Job g_BoyAttackedTime g_used_Skill_Time g_WordDisplay TT g_savedata g_Name game_TetrisPlayed game_ShopOpenNumber g_selected_item_ID BuyerID g_TravelMap_Count HP_TravelMap game_ItemRentEvent g_BS_Job g_curEXPmode g_scroll_Y g_ClearedGame Ealpha Ealpha2 g_TimeStorage g_TIME_hr g_TIME_min g_Aco_Head g_Aco_HeadPal g_Aco_BodyPal g_Aco_Name g_Aco_Hair g_BS_Hair g_Aco_Hair_Color g_BS_Hair_Color`.

The technical plan estimated 54 functions, 58 globals and about 450 lines from a narrower count; the rules above find 557 sites in 54 groups.

## Totals

| Verdict | Groups | Sites |
|---|---:|---:|
| added, lines in a function that stays | 27 | 249 |
| added, whole function (the function and its declaration go) | 17 | 232 |
| added, with one replaced line in the group | 3 | 15 |
| replaced | 5 | 58 |
| replaced, partly open | 2 | 3 |
| **all** | **54** | **557** |

## Found by reading, outside the name lists

Single-player changes that mention none of the names above, found while reading the sites. They are part of the recovery and are handled with the step named.

| Where | What | Verdict |
|---|---|---|
| `UIWindowMgr::ProcessPushButton`, head | key remapping (Enter, W, X and the scan codes of A, D, W, X turned into A, D, up, down) that makes the online `VK_RETURN`, Alt+X and Alt+W cases unreachable | added (M1.2) |
| `UIWindowMgr::ProcessPushButton`, case `'D'` | leaves single-player screens | added (M1.2) |
| `CGameMode::SendMsg` (`GameMode2.cpp`), `MM_WEAR_EQUIP`, `MM_TAKE_OFF_EQUIP` | move items between inventory and equipment locally instead of asking the server; `MM_WEAR_EQUIP_ARROW` beside them still sends `CZ_REQ_WEAR_EQUIP`, and `Zc_Req_Wear_Equip_Ack`/`Zc_Req_Takeoff_Equip_Ack` do the move when the server answers | replaced (M1.4): send `CZ_REQ_WEAR_EQUIP` / `CZ_REQ_TAKEOFF_EQUIP` and wait for the ack (protocol, code) |
| `CSession::SetStatusData`, `CGameMode::GetNextExpValue`, `CGameMode::GetWeaponPower` | copy the single-player actor's stats into the session, the single-player experience table, a weapon attack table; used only by deleted code and by the single-player status display | added (M1.2) |
| `g_BackTexture` (`GameMode.cpp`) | set in the single-player tail of `Initialize`, read nowhere | added (M1.2) |
| `UIMakeCharWnd` constructor and `MakeButton` | the preview is an acolyte (`job = 4`), a second preview (`m_charInfo2`, a blacksmith) with its own head buttons, a difficulty button; stats left uninitialised | replaced, open (M1.4): novice preview, stats 5 each (the value the hexagon buttons reset a pair to), the second preview and the difficulty button go |
| `ReadRegistry` (`WinMain.cpp`) | gives up when the key is absent (the caller then ran `Setup_HP.exe`); after reading, it forces windowed 1024×768, no tips at start-up and sound on | replaced (M1.5): built-in defaults when the key or a value is absent. The forced values stay and are marked open (`registry-forced`): nothing shows whether the 2008 client had them, and M4 decides the window size anyway |

## Open sites

Sites that reading did not settle. Each is marked in the source with `// BF_RECOVER_OPEN(<id>): <what is unclear>` and is carried to M6 and M8, where it can be observed against the server.

| Id | File | What is unclear |
|---|---|---|
| `wheel-step` | `GameMode.cpp`, `CGameMode::ProcessWheel` | the zoom step per wheel notch |
| `status-layout` | `UIFrameWnd2.cpp`, `UIStatusWnd::OnDraw` | exact positions of the restored values and of the stat costs |
| `makechar-layout` | `window/UIFrameWnd.cpp`, `UIMakeCharWnd` | the 2008 layout of `win_make.bmp`: preview, hexagon, stat values and buttons |
| `registry-forced` | `WinMain.cpp`, `ReadRegistry` | whether the forced windowed 1024×768, tips off and sound on are the 2008 client's or the single-player game's |

## Sites

One row per enclosing function; the last column gives the lines per name. `(file scope)` rows are declarations and definitions outside functions; a function's own definition line is counted in its row.

| file | enclosing function | sites | verdict | lines and names |
|---|---|---:|---|---|
| `HP_Battle.cpp` | `(file)` | 1 | added The single-player battle, field and menu code. | file: 0 |
| `HP_BuySell.cpp` | `(file)` | 1 | added The blacksmith shop and smithing mini-game. | file: 0 |
| `HP_RestTravell.cpp` | `(file)` | 1 | added Resting, day count and the travel map. | file: 0 |
| `HP_SaveLoad.cpp` | `(file)` | 1 | added Save slots and the ending. | file: 0 |
| `HP_Tetris.cpp` | `(file)` | 1 | added The block-puzzle mini-game. | file: 0 |
| `HP_Tetris.h` | `(file)` | 1 | added Its class. | file: 0 |
| `GameActor.cpp` | `(file scope)` | 3 | added; HP_ST_SLEEP replaced Externs of single-player globals. `HP_ST_SLEEP = 10` duplicates the online state `ST_SLEEP = 10` (GameActor.h); the constant goes and its use takes `ST_SLEEP`. | HP_ST_SLEEP: 45; g_curStance: 3469; g_curScreen: 3470 |
| `GameActor.cpp` | `CGameActor::ProcessState` | 1 | added The `m_gid == 3` branch turns the single-player boy to face the field after a move; the `else` (`SetState(ST_STAND)`) is the online path and stays. | g_curScreen: 3614; g_curStance: 3614 |
| `GameActor.cpp` | `CGameActor::SendMsg` | 1 | replaced `SetState(HP_ST_SLEEP)` becomes `SetState(ST_SLEEP)`: same value, the online name (code). | HP_ST_SLEEP: 4204 |
| `GameMode.cpp` | `(file scope)` | 24 | added Externs and definitions of single-player globals and the damage queue (`g_DM_List`, `g_DamageMoment`). | game_TetrisPlayed: 115; HP_Damage_Moment: 116,174,179; g_NextSkill: 132; g_SpellSeek: 133; g_SpellSeekTime: 134; g_used_Skill: 137; g_used_Skill_Time: 138; g_BoyAttackedTime: 139; g_curScreen: 140; g_curStance: 141; map_Number: 142; g_curStory: 143; g_curEXPmode: 144; g_curEnding: 145; g_Aco_Job: 146; g_FieldConv_Alpha: 147; g_AcoPosition: 148; g_MainNextActTime: 149; g_MobNextOutTime: 150; g_DM_List: 174; bat_STUN_turn: 235; g_PrevMob: 236; g_BattleEvent: 237 |
| `GameMode.cpp` | `CGameMode::Initialize` | 9 | added; tail replaced Lines 608-633 are the single-player set-up: player placed at cell 13,0, starter items, the two companions, story state, skill 0x107, screen 100, damage queue reset, two instruction lines. Online, the player is placed from `g_session.m_posX/Y` in `CWorld::OnEnterFrame` (GameWorld.cpp) and items, skills and status arrive by packet. `g_BackTexture` is set here and read nowhere. The function ends after the cloud/sakura weather set-up. | HP_AddItem: 613,614,615,616; Zc_Notify_Entry_HP: 617,618; game_TetrisPlayed: 620; g_curStory: 621; SetcurScreen: 623 |
| `GameMode.cpp` | `CGameMode::OnUpdate` | 3 | added; replaced `ProcessDamageSituation()`, `RenderBackGround2D()` and `RenderMenu()` are single-player. `PollNetworkStatus()` is restored at the top, after `CheckSystemMessage()`, as in `CLoginMode::OnUpdate` (code: the function exists and nothing calls it). | ProcessDamageSituation: 692; RenderBackGround2D: 699; RenderMenu: 701 |
| `GameMode.cpp` | `CGameMode::ProcessActorPick` | 6 | added The `m_lastPcGid == 1` block queues single-player skills on clicks on the acolyte; `ProcessPcPick` after it is the online path. | g_NextSkill: 3589,3600; g_SpellSeek: 3593,3604; g_SpellSeekTime: 3594,3605 |
| `GameMode.cpp` | `CGameMode::ProcessWheel` | 24 | replaced The whole body steers the single-player battle (skill queue, field walk). Restored as camera zoom: the wheel changes the view distance with the clamps of the Ctrl+right-drag zoom in `ProcessRBtn`, unless the pointer is over a window (`processType == 1`) or a skill is being aimed (then `ProcessInput` uses the wheel for the skill level). `CView::AddDistance` exists and nothing calls it (code). Step size open. | fixed actor id: 4876,4877; g_used_Skill: 4881; g_used_Skill_Time: 4882; g_NextSkill: 4884,4893,4898,4906; g_BoyAttackedTime: 4897; g_curScreen: 4910; g_curStance: 4910,4919,4930,4936,4940,4948; g_curStory: 4923,4924; map_Number: 4923,4924; g_AcoPosition: 4929,4938,4942,4944; g_FieldConv_Alpha: 4937; g_MainNextActTime: 4949; g_MobNextOutTime: 4951 |
| `GameMode.cpp` | `CGameMode::ProcessDamageSituation` | 18 | added (whole function) Plays the queue of damage the single-player battle computes locally; online, damage arrives as `ZC_NOTIFY_ACT` and is shown by the actor messages. | ProcessDamageSituation: 7763; HP_Damage_Moment: 7766; g_DM_List: 7766,7767,7912,7913; fixed actor id: 7773,7786; bat_STUN_turn: 7776; g_used_Skill: 7836; g_BattleEvent: 7842,7849,7857; g_PrevMob: 7851; g_curStance: 7873; g_MainNextActTime: 7874; BS_StartFasterTetris: 7879; GainExp: 7881; EndFight_Status: 7883 |
| `GameMode.cpp` | `CGameMode::GainExp` | 58 | added (whole function) Client-side experience and level-up of the single-player acolyte; online, `ZC_LONGPAR_CHANGE`/`ZC_PAR_CHANGE` carry them. `GetNextExpValue` and `GetWeaponPower` serve only it and the single-player status display and go with it. | GainExp: 7921; fixed actor id: 7923; g_curEXPmode: 7929; g_Aco_Job: 7949,7952; g_curStory: 7980,7984,7988,7992,7996,8009,8010,8019,8021,8032,8034,8045,...; map_Number: 7980,7984,7988,7992,7996; g_curEnding: 8104; DoPCHaveItem: 8109,8114,8119,8124,8129,8134,8139,8144; HP_AddItem: 8110,8112,8115,8117,8120,8122,8125,8127,8130,8132,8135,8137,... |
| `GameMode.cpp` | `CGameMode::GetWeaponCost` | 1 | added (whole function) Used only by the shop of `HP_BuySell.cpp`. | GetWeaponCost: 8189 |
| `GameMode.cpp` | `CGameMode::DeleteBroadcastBalloon` | 1 | added (whole function) Called only by the single-player code (closure rule); dead once it goes. | DeleteBroadcastBalloon: 8226 |
| `GameMode.h` | `CGameMode (declarations)` | 7 | added Declarations of the above. | DeleteBroadcastBalloon: 241; ProcessDamageSituation: 287; GainExp: 288; GetWeaponCost: 291; Zc_BroadCast_HP: 330; Zc_Notify_Entry_HP: 590; Zc_Notify_Entry_Mob: 591 |
| `GameModePacket.cpp` | `(file scope)` | 9 | added Externs of single-player globals. | g_Aco_Job: 31; g_BS_Job: 32; Boy_Job: 33; g_Aco_Hair: 34; g_BS_Hair: 35; g_Aco_Hair_Color: 36; g_BS_Hair_Color: 37; g_curStory: 54; bat_mobskill_turn: 55 |
| `GameModePacket.cpp` | `CGameMode::Zc_Notify_Entry_HP` | 15 | added (whole function) Spawns the single-player companions locally (no packet: an int id and two shorts); online actors enter with `ZC_NOTIFY_STANDENTRY` and friends. | Zc_Notify_Entry_HP: 11668; g_Aco_Job: 11678; g_Aco_Hair: 11680; g_Aco_Hair_Color: 11686; g_BS_Job: 11693; g_BS_Hair: 11695,11742; g_BS_Hair_Color: 11701,11748; Boy_Job: 11740; HP_EquipItemById: 11827,11828,11829,11830; m_DayCount: 11831 |
| `GameModePacket.cpp` | `CGameMode::Zc_Notify_Entry_Mob` | 40 | added (whole function) Spawns the single-player monsters locally. | Zc_Notify_Entry_Mob: 11845; g_curStory: 11900,11917,11927,11938,11948,11958,11969,11986,11996,12006,12016,12026,...; bat_mobskill_turn: 12338 |
| `GameModePacket.cpp` | `CGameMode::Zc_BroadCast_HP` | 1 | added (whole function) A copy of the broadcast balloon fed with single-player text instead of a packet; `Zc_Broadcast` is the online handler. | Zc_BroadCast_HP: 16556 |
| `GameWorld.cpp` | `CWorld::MakeSkill` | 1 | added (whole function) Used only by the single-player battle. | MakeSkill: 1378 |
| `GameWorld.cpp` | `CWorld::RemoveSkillbyJT` | 1 | added (whole function) Used only by the single-player battle. | RemoveSkillbyJT: 1403 |
| `GameWorld.h` | `CWorld (declarations)` | 2 | added Declarations of the above. | MakeSkill: 84; RemoveSkillbyJT: 85 |
| `Mode.cpp` | `CMode::DrawBackGround2D` | 1 | added (whole function) Draws the single-player 2D backgrounds; called only from `HP_Battle.cpp`/`HP_RestTravell.cpp`. | DrawBackGround2D: 214 |
| `Mode.h` | `CMode (declarations)` | 1 | added Declaration of the above. | DrawBackGround2D: 335 |
| `Session.cpp` | `(file scope)` | 3 | added Externs of single-player globals. | g_used_Skill: 69; g_Mob_used_Skill: 70; g_BattleEvent: 71 |
| `Session.cpp` | `CSession::MoveItemFromItemWndToEquipWnd` | 1 | added The block on actor 1 (weapon and headgear sprites, `SetStatusData`) dresses the single-player acolyte; online the server sends `ZC_SPRITE_CHANGE` and the status packets after the equip ack. `SetStatusData` serves only the single-player code and goes with it. | fixed actor id: 3660 |
| `Session.cpp` | `CSession::MoveItemFromEquipWndToItemWnd` | 1 | added The same block for taking off. | fixed actor id: 3700 |
| `Session.cpp` | `CSession::HP_AddItem` | 2 | added (whole function) Creates inventory items locally. | HP_AddItem: 3891; HP_GetLocation: 3902 |
| `Session.cpp` | `CSession::HP_MakingItem` | 19 | added (whole function) The smithing reward. | HP_MakingItem: 3975; HP_AddItem: 3981,3983,3988,3990,3995,3997,4002,4004,4009,4011,4016,4018,... |
| `Session.cpp` | `CSession::HP_GetLocation` | 1 | added (whole function) Equip location table for `HP_AddItem`. | HP_GetLocation: 4045 |
| `Session.cpp` | `CSession::Mob_SkillReturn` | 68 | added (whole function) Monster skill choice of the single-player battle. | Mob_SkillReturn: 4123; g_used_Skill: 4132,4166,4215,4259,4343,4353,4365,4374,4404,4420,4431,4435,...; g_BattleEvent: 4154,4156,4158,4160,4162,4225,4227,4229,4231,4233,4235,4307,...; g_Mob_used_Skill: 4167,4169,4172,4174,4197,4214,4246,4254,4261,4274,4339,4367,... |
| `Session.cpp` | `CSession::HP_EquipItemById` | 1 | added (whole function) Local equip. | HP_EquipItemById: 4696 |
| `Session.cpp` | `CSession::DoPCHaveItem` | 1 | added (whole function) Used only by `GainExp`. | DoPCHaveItem: 4711 |
| `Session.h` | `CSession (declarations)` | 8 | added Declarations of the above and `m_DayCount`. | DoPCHaveItem: 917; HP_EquipItemById: 918; HP_AddItem: 1089; HP_GetLocation: 1090; HP_MakingItem: 1091; Mob_SkillReturn: 1180; SetDamageSituation: 1342; m_DayCount: 1416 |
| `Session2.cpp` | `(file scope)` | 1 | added Extern of the damage queue. | HP_Damage_Moment: 23; g_DM_List: 23 |
| `Session2.cpp` | `CSession::SetDamageSituation` | 3 | added (whole function) Fills the damage queue. | SetDamageSituation: 5578; HP_Damage_Moment: 5580; g_DM_List: 5585 |
| `UIFrameWnd2.cpp` | `UIStatusWnd::OnDraw` | 1 | replaced, open The single-player version draws only ATK (`Str*10 + weapon power`) and DEF (`Vit*5`) of actor 1. Restored: the sixteen values the window's own hover table (`UIStatusWnd::OnMouseHover`) names, from the session getters the status packets fill (ATK, MATK, HIT, CRI, status points, guild; DEF, MDEF, FLEE, ASPD), right-aligned where the single-player code drew ATK and DEF, and the stat costs. Exact text positions open (`status-layout`). | fixed actor id: 1803 |
| `UIWindowMgr.cpp` | `(file scope)` | 83 | added Externs and definitions of single-player globals. | g_menuSelect: 55,72; g_curScreen: 56,112; g_curStance_B: 57,115; TT: 58; Tetris: 58; g_curRectTop: 73; g_AcoPosition: 74; g_PrevMob: 75; map_Number: 76; map_firstway: 77; map_secondstart: 78; map_secondway: 79; map_thirdstart: 80; map_thirdway: 81; map_BNPC_ID: 82; Boy_Job: 83; g_Aco_Job: 84; g_Aco_Head: 85; g_BS_Job: 86; g_Aco_Name: 87; g_Aco_Hair: 88; g_BS_Hair: 89; g_curSave: 90; g_curHardness: 91; g_curEXPmode: 92; g_savedata: 106; HP_TRAVEL_AVAILABLE: 107; HP_TravelMap: 107; g_PoringCurX: 108; g_PoringCurY: 109; g_PoringTarY: 110; g_PoringTarX: 111; g_curStory: 113; g_curStance: 114; g_BS_phase: 116; g_BS_NextActTime: 117; g_WordDisplay: 118; g_menuFreeze: 120; g_ScreenConv_Alpha: 122; g_FieldConv_Alpha: 123; g_MainNextActTime: 124; g_MobNextOutTime: 125; g_HelpNextActTime: 126; g_BoyAttackedTime: 127; g_Frame: 128; g_MobNextTimeFixed: 129; g_scroll_Y: 130; bat_STUN_turn: 131; bat_mouse_pos: 132; bat_ChookGi: 133; bat_mobskill_turn: 134; map_BOSS_done: 135; map_Heal_Used: 136; map_ComboF_Used: 137; map_BNPC_Pos: 138; map_BNPC_done: 139; map_BNPC_event: 140; game_TetrisPlayed: 141; game_ShopOpenNumber: 142; game_ItemRentEvent: 143; g_selected_item: 144; g_selected_item_ID: 145; g_used_Skill: 146; g_used_Skill_Time: 147; g_Mob_used_Skill: 148; g_BattleEvent: 149; g_NextSkill: 150; g_SpellSeek: 151; g_SpellSeekTime: 152; g_TravelMap_Count: 153; Boy_friendly: 154; Boy_talked: 155; Boy_frame: 156; g_Aco_HeadPal: 157; g_Aco_BodyPal: 158; g_TIME_hr: 159; g_TIME_min: 160; g_Aco_Hair_Color: 161; g_BS_Hair_Color: 162; g_curEnding: 163; g_ClearedGame: 164; g_TimeStorage: 165 |
| `UIWindowMgr.cpp` | `UIWindowMgr::ProcessPushButton` | 34 | added Tetris moves and `g_menuSelect` in the arrow keys, `ExeMenu()` on A, `SetcurScreen` on D, the no-op `GetGameActorByAid(1)` on S. Also, found by reading: the key remapping at the top (Enter, scan codes 0x1e/0x20/0x11/0x2d, W, X turned into A/D/up/down) is single-player; it makes the online `VK_RETURN`, Alt+X and Alt+W cases unreachable, which shows it was put in front of them. | g_menuSelect: 4725,4726,4727,4749,4750,4751; g_curStance_B: 4728,4752,4780,4797; TT: 4729,4730,4731,4732,4733,4734,4756,4757,4763,4765,4784,4785,...; BlockSetting: 4732; ExeMenu: 4862; fixed actor id: 4873; g_curScreen: 4877,4879,4881; SetcurScreen: 4878,4880,4882 |
| `WinMain.cpp` | `SearchProcessIn9X` | 1 | added `HIGHPRIEST.EXE` added to the list of accepted process names. | "HIGHPRIEST.EXE": 187 |
| `WinMain.cpp` | `SearchProcessInNT` | 1 | added The same. | "HIGHPRIEST.EXE": 262 |
| `WinMain.cpp` | `WinMain` | 6 | replaced `data_hp.grf` and the `g_serviceType` test: the archive list is read from `DATA.INI` before the first file is opened (M1.3). `Setup_HP.exe` loop: built-in defaults (M1.5). `g_Aco_Name` and the HP/SP/level/zeny set-up: single-player session, deleted. `MT_GAME, "World_HP.rsw"`: `MT_LOGIN, "login.rsw"` (M1.3). The process-name check goes (M1.5). | "data_hp.grf": 892; "Setup_HP.exe": 928; "Setup\\Setup_HP.exe": 929; g_Aco_Name: 1000,1001; "World_HP.rsw": 1016 |
| `pdb_shared.h` | `HP_Damage_Moment (declarations)` | 1 | added Type of the damage queue. | HP_Damage_Moment: 10 |
| `pdb_shared.h` | `HP_TRAVEL_AVAILABLE (declarations)` | 1 | added Type of the travel map. | HP_TRAVEL_AVAILABLE: 19 |
| `framework/Locale.cpp` | `InitClientInfo` | 2 | replaced Registry key and window title name HighPriest; they become `Software\` + `BF_APP_NAME` and `BF_APP_NAME` (M1.5). | "Software\\Gravity Soft\\HighPriest": 197; "HighPriest    Version 0.96": 198 |
| `window/UIFrameWnd.cpp` | `(file scope)` | 12 | added Externs of single-player globals. | g_curHardness: 45; g_curSave: 46; g_curEXPmode: 47; g_TIME_hr: 48; g_TIME_min: 49; g_TimeStorage: 50; game_ItemRentEvent: 51; g_Aco_Name: 52; g_Aco_Hair: 53; g_BS_Hair: 54; g_Aco_Hair_Color: 55; g_BS_Hair_Color: 56 |
| `window/UIFrameWnd.cpp` | `UIMakeCharWnd::OnDraw` | 2 | replaced, open Draws `win_make_HP.bmp`, a second (blacksmith) preview and the difficulty marker. Restored: `win_make.bmp`, the one preview, the stat hexagon `DrawHexagon()` (defined and never called) and the six stat values. Layout open (`makechar-layout`). | "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4": 2551; g_curHardness: 2669 |
| `window/UIFrameWnd.cpp` | `UIMakeCharWnd::SendMsg` | 25 | replaced OK sets up the single-player save, Cancel goes to a single-player screen. Restored from the login mode: OK stores the character in `g_charInfo` and sends `LMM_SENDCHARINFO` (handled in `CLoginMode::SendMsg`, sent by nobody, which then sends `CH_MAKE_CHAR`); Cancel sends `MM_COMMAND, LMM_SELECTCHARACTER`. The difficulty button and the second preview's head buttons go. | fixed actor id: 2816,2817; g_Aco_Name: 2840,2841; g_Aco_Hair: 2843,2845,2847; g_Aco_Hair_Color: 2844,2846,2848; g_BS_Hair: 2850,2852; g_BS_Hair_Color: 2851,2853; SetcurScreen: 2858,2884; g_TIME_hr: 2859; g_TIME_min: 2860; g_TimeStorage: 2861; MakeSaveFileName_IN: 2862; g_curSave: 2862; game_ItemRentEvent: 2863; g_curEXPmode: 2864; g_curHardness: 3060,3061,3063 |
| `window/UISys.h` | `UIWindowMgr (declarations)` | 45 | added The 45 single-player methods of `UIWindowMgr`. | RenderBackGround2D: 1692; RenderBackGround2D_House: 1693; RenderBackGround2D_Battle: 1694; RenderBackGround2D_Travel: 1695; RenderAcoPosition: 1696; RenderTravelMap: 1697; RenderEnding: 1698; RenderNextSkill: 1699; BS_Tetris: 1700; BS_StartFasterTetris: 1701; BS_Selling: 1702; BS_Buying: 1703; BS_Buying_Weapon: 1704; BS_NotBuying: 1705; BS_NotAnswering: 1706; BS_Making: 1707; Aco_ActinField: 1708; Mob_ActinField: 1709; Boy_ActinField: 1710; Mob_WhichOne: 1711; Aco_Travel: 1712; All_CheckPos: 1713; All_CheckInput: 1714; Aco_Battle: 1715; Mob_Battle: 1716; Help_Battle: 1717; RenderMenu: 1718; Aco_Revive: 1719; RenderThreeWay: 1720; Aco_DecideinField: 1721; Aco_MoveByDec: 1722; RenderWhiteBlueRect: 1723; GetitemName_HP: 1724; GetmapName_HP: 1725; SetcurScreen: 1792; SetcurStance_B: 1793; EndFight: 1794; EndFight_Status: 1795; ExeMenu: 1796; HP_Resting: 1797; HP_DayPassing: 1798; HP_SaveGame: 1799; HP_LoadGame: 1800; MakeSaveFileName: 1801; MakeSaveFileName_IN: 1802 |
| `sources.mk` | `(build list)` | 1 | added The five units leave the list of game units (read by `census.sh` and by the Windows `Makefile`, which is left as it is; the branch gives up the byte-identical build). | HP_*: 24 |
