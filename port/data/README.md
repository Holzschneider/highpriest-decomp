# Game folder

The client reads `~/Games/Bifrost` (or `BF_CLIENT_DIR`): `rdata.grf` over `data.grf`, `BGM/`, and a loose `data/` folder that it searches first. None of it is in git.

```bash
BF_DATA_HOST=user@host BF_DATA_PATH=/path/to/ro BF_ROE_PATH=/path/to/ROenglishRE port/data/fetch.sh
port/data/assemble.py              # builds data/ from ROenglishRE; --check only reports
port/tools/tablecheck.py           # the client's text tables against its parsers; exit 1 on a fault
port/data/server-maps.sh           # build/maps.txt and port/server/import/maps_enterable.conf
```

`fetch.sh` copies the archives, the music and the ROenglishRE checkout with rsync, which only reads on the source side and skips what is already there. The source machine is named by the environment or the arguments, never in a committed file.

`assemble.py` writes into `data/`:

| Files | Source | Why |
|---|---|---|
| `sclientinfo.xml` | generated | English service (`america`), the local server, `<readfolder>` |
| 7 item tables | ROenglishRE `Translation/Compatibility/2012-04-10/Pre-Renewal/data` | missing from the 2021 archive |
| `skillnametable.txt`, `skilldesctable.txt` | ROenglishRE `Translation/Pre-Renewal/data` | missing from the archive |
| `msgstringtable.txt` | ROenglishRE `Translation/Renewal/data` (no pre-renewal version exists) | missing; its first 1,387 lines match the client's `MSI_` ids one to one |
| `leveluseskillspamount.txt` | ROenglishRE `Additions/data` | missing |
| 8 maps (`prontera`, `prt_fild05`, `prt_fild08`, `prt_in`, `prt_church`, `izlude`, `alberta`, `morocc`) | ROenglishRE `Translation/Pre-Renewal/data` | the archive holds their renewal layout; the pre-renewal server walks the older one (`db/pre-re/map_cache.dat`), which these files match cell for cell |

Files are copied unchanged, with two repairs that `tablecheck.py` asks for: in `skilldesctable.txt` and `leveluseskillspamount.txt` the blocks of skills this client does not know (third jobs and later) are removed, because its loader files every line after an unknown skill name under the skill before it. Missing message ids would get `?msg N` placeholders; the current table needs none.

`server-maps.sh` reads the server's map list and map caches (from the running container, read only, or from rAthena at the pinned commit) and has `grfaudit.py` write the maps the client can enter and the `delmap:` lines for the rest. A map is enterable when its world, ground and attribute files are readable (through `ResNameTable.txt` aliases, as the resource manager resolves them) and at most 10% of the models it places are missing; it is also removed from the server when more than 1% of its cells differ from the server's map cache. `port/server/import/map_conf.txt` imports the result; it takes effect when the server restarts.
