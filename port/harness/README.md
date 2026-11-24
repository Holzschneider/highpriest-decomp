# Test harness

Scripted play for the milestones that need playing (spec sections 6 and 7): a script drives the client through its control socket ([PROTOCOL.md](../native/control/PROTOCOL.md)), and each step is confirmed in the test server's SQL logs and by captured frames. Python 3, standard library only.

```bash
python3 port/harness/run.py --self-test                      # harness tests: mock client + one read-only server check
python3 port/harness/run.py port/harness/scripts/login.py --mock
BF_CLIENT=<client binary> python3 port/harness/run.py login  # the real test (server must be online)
python3 port/harness/run.py --server-status
```

A run prints one line per step and writes `build/harness/<script>[-mock]-<time>/` with `report.json`, `report.md`, the client's trace, packet log, stdout and the control transcript; the latest report is also copied to `build/harness/<script>[-mock]-latest.{json,md}`. The exit status is 0 when every step passed.

## Layout

| File | What it does |
|---|---|
| `run.py` | command line: `--self-test`, `--server-status`, a script with `--mock`, `--param k=v`, `--client-arg`, `--timeout` |
| `bfharness/server.py` | the test server through `docker compose`: `status`, `online` (container running, "Map Server is now online" since it started, ports open), `ensure_up`, and a guarded `restart` |
| `bfharness/sql.py` | `Sql` (MariaDB via `docker compose exec -T db mariadb --batch`), `MockSql`, `Marker`, and the `Logs` readers |
| `bfharness/accounts.py` | test accounts, offline set-up in SQL, `GM` atcommands typed by the client |
| `bfharness/client.py` | launches `BF_CLIENT` with `--control` and the `BF_*` environment, collects trace and exit code, kills on timeout |
| `bfharness/control.py` | the socket client: requests by id, the event stream, helpers (`type_text`, `say`, `click`, `click_control`, `wait_mode`, `capture` ...) |
| `bfharness/frames.py` | captures to `<game folder>/test/frames/<script>/<name>.png`, checked against the reply's SHA-256; the checksum manifest |
| `bfharness/runner.py` | steps with evidence, the reports |
| `bfharness/play.py` | shared steps: log in (through the character server list, which the 2008 client shows even for one server), enter the game, walk to a clicked cell, say a line and find it in chatlog; for M5: wait for `CZ_ENTER` in the packet log, compare the login with `bifrost-netprobe`'s, record the exit after zone entry |
| `bfharness/pktlog.py` | reads `BF_PKT_LOG` files; the login sequence (account and character connections, up to the first zone packet sent) and its comparison: same packets sent in the same order, same packets received per connection (the character server's `HC_BLOCK_CHARACTER` comes before or after `CH_SELECT_CHAR` by timing). Also a command: `pktlog.py <client log> <netprobe log>` |
| `mock_client.py` | serves PROTOCOL.md with a fake game, and writes what the server would log to a JSON-lines file in the shape of rAthena's tables |
| `scripts/` | `login.py`, `create_char.py` (M5), `walk.py` (M6), `gate.py` (M8): the shapes the real tests take; all pass against the mock. Real client only (they note that and pass with `--mock`): `login_edit.py` (M5.3: typing, Backspace, Tab, Cmd+V, masking, "save" with the account in the settings and the password in the Keychain), `charselect.py` (M5.4: select by click or `--param via=enter`, `CZ_ENTER`, packets equal to `bifrost-netprobe`'s), `autologin.py` (M5.5: the same from launch arguments, `--param via=env` for the environment), `m4_frames.py`, `replay.py`, `english.py` (M9.1: `CA_LOGIN` and a Windows-1252 name on the wire, stored by the server; needs `char_name_option: 0` and `default_codepage: latin1`, set in `port/server/import`), `texttest.py` (M9.4: the client with `BF_TEXTTEST=1` opens the status, equipment, item, skill and option windows over the login screen; judged by eye, no server) |
| `tests/` | the self-tests |

## Rules the harness keeps

- **The server is shared.** `ensure_up` starts it only when it is not running, from the main checkout's `port/server` (or `BF_SERVER_DIR`), since a worktree's copy would remount the fixed compose project. `restart` restarts the `rathena` service only and refuses unless called with `allow=True` or `BF_SERVER_RESTART=1`: it drops every connected session, so it is for the lead after a configuration change.
- **Frames stay out of git.** Real runs capture under `<game folder>/test/frames/<script>/` (`BF_FRAMES_DIR` overrides); the script's `MANIFEST` path in the repository gets `<sha256>  <w>x<h>  <name>.png` lines, nothing else. Mock runs write their synthetic frames under `build/harness/frames-mock/` and their manifest into the run directory.
- **Evidence windows.** `t.logs.mark()` before an action records the database clock and the highest id of every log table; readers take `after=marker` and select by id, which is exact. `loginlog` has no id and is selected by time (one-second granularity).
- **Set-up versus play.** SQL set-up (`Accounts.create_account`, `Accounts.set_char`) is for the scene before the client logs in: it refuses an online character, because the map server overwrites the row at every save. Once in game, set-up is by atcommand, typed by the client (`t.gm.run(GM.warp(...))`) and so logged in `atcommandlog`; it needs a group-99 account (`bfadmin`, or `create_account(..., group_id=99)`). The action under test is always played through the client.
- **Socket path.** macOS limits a socket path to 103 bytes; the socket lives in `build/harness/sock/`, or in the system temporary directory when that path is too long.

## Environment

| Variable | Used for |
|---|---|
| `BF_CLIENT` | the client binary (or command line) under test |
| `BF_TRACE` | trace categories passed to the client (default `err,net,mode,stub`) |
| `BF_CLIENT_DIR` | game folder, passed through; default `~/Games/<name>` |
| `BF_FRAMES_DIR` | where frames go instead of `<game folder>/test/frames` |
| `BF_SERVER_DIR` | compose project of the test server |
| `BF_SERVER_RESTART=1` | allows `Server.restart` |

The client also receives `BF_TRACE_FILE`, `BF_PKT_LOG` (both in the run directory) and `BF_CONTROL` (the socket path, in addition to `--control`).

## SQL evidence per criterion

| Criterion | Evidence |
|---|---|
| M5 login reaches character selection | `loginlog` row `user`=account, `rcode`=100 after the marker; mode `charselect`; slots equal `char` rows of the account |
| M5 a script creates a character | `charlog` `char_msg`='make new char' with the name; `char` row on the account |
| M5 same login from launch arguments | `autologin.py`: `loginlog` 'login ok' and `charlog` 'char select' after the marker, `CZ_ENTER` in `client-packets.txt`, no `key` or `pointer` request in the transcript |
| M5 packet log matches `bifrost-netprobe` | `charselect.py`, `autologin.py`: `client-packets.txt` against `netprobe-packets.txt` of the same run (`bfharness/pktlog.py`; not SQL) |
| M5 selection enters the zone | `charselect.py`: `CZ_ENTER` (0x009b) on the zone connection; `charlog` 'char select' |
| M6 walk: client and server agree | `chatlog` row of a line said on arrival: `src_map`, `src_map_x`, `src_map_y` are the map server's position, compared with `state.self`; after quitting, `char.last_map/last_x/last_y` |
| M6 resize keeps picking correct | the same, after `resize`, for the cell `state.pointer.cell` named before the click |
| M6 map change by warp portal | `map` event and `state.map`; `chatlog.src_map` of a line said after it; `char.last_map` after quitting |
| M8 gate: kill a Poring | `picklog` `type`='M', `char_id`=1002 (rAthena logs monster drops under the mob id), map `prt_fild08` |
| M8 gate: pick up the drop | `picklog` `type`='P', `char_id`, `nameid`=909, `amount` > 0 |
| M8 gate: buy a potion | `picklog` `type`='S', `nameid`=501, `amount` > 0; `zenylog` `type`='S', `amount` < 0 |
| M8 gate: sell the loot | `picklog` `type`='S', `nameid`=909, `amount` < 0; `zenylog` `type`='S', `amount` > 0 |
| M8 gate: change job | `char.class` after logout (job changes are not in a log table) |
| M8 gate: chat | `chatlog` `type`='O' with the line |
| set-up steps | `atcommandlog` row per command |

## For the client's control module

The scripts use only what PROTOCOL.md specifies. Where they click a control they use these `controls` ids, which the control module has to report (or the scripts change with it): `LOGINWND` `id_edit`, `pw_edit`, `ok`; `SELECTCHARWND` `slot:<n>` (text = character name), `ok`, `make`; `MAKECHARWND` `name_edit`, `ok`; `CHOOSESELLBUYWND` `buy`, `sell`; `ITEMSHOPWND` and `ITEMSELLWND` `item:<item id>`, `ok`; `CHOOSEWND` `choice:<label>`; `RESTARTWND` `charselect`, `exit`. Login, character selection and creation go by keyboard only (Tab, Enter, arrows), so the M5 scripts work even while `controls` is empty. The real client reports these and more (all controls of a window, named after their control ids; the list is in [compat/stubs/README.md](../compat/stubs/README.md), `bf_client_state.cpp`).

## Open

- The map load after `CZ_ENTER` belongs to M6.1; `charselect.py` and `autologin.py` record the client's exit after zone entry instead of judging it (`stop_after_zone_entry`); `--param strict_exit=1` judges it again.
- "Save" with the Keychain is untested while the login keychain is locked (unattended run): the client then stores the account only and traces `sys: keychain not available`.

- The gate test's Job Master is rAthena's `npc/custom/jobmaster.txt`, which the stock configuration does not load: `port/server/import/map_conf.txt` needs `npc: npc/custom/jobmaster.txt`.
- The map server saves a character on logout, map-server change and every `autosave_time` (300 s in `map_athena.conf`); the scripts read positions from `chatlog` while playing and from `char` only after quitting.
- The mock's picking is a flat grid; it checks the harness, not the client's camera.
