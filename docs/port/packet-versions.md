# Adding a packet version

The client speaks one protocol: the zone packets of rAthena's `PACKETVER` 20080401 to 20080820 (technical plan 2.4). Decision 3.1.3 asks for the seams that make a later version an addition rather than a rewrite; M10 put them in place. None is implemented for another version. This note says where they are and what a version would take, with 20080910 as the worked example.

## The seams

| What | Where | Rule |
|---|---|---|
| Packet names and ids | the `HEADER_` enumeration of `src/packet.h` | Every handler and sender names a packet by its enumerator. `port/tools/packetids.py` finds any packet id written as a number in `src/` (rules in its docstring, `--self-test` for the rules) and exits 1 if there is one. |
| Lengths | `InitPacketLenWithClient` in `src/packet.cpp`, which fills `g_packetLenMap` | The only place that decides a length. `GetPacketSize`, `IsFixedPacket` and, under `BF_PORT`, `GetRecvPacketSize` (a length the server sends under a request's header, the auction answer of M8.4) are packet.cpp's; `CRagConnection` asks them. `bifrost-packetlen` prints the table. |
| Requests whose layout changes | `src/PaddingPacket.h` | One `BuildCzX(buf, fields...)` per request next to its layout: header, padding, fields, and the length from the table. Senders pass fields and a `PADDING_PACKET_BUFSIZE` buffer and never see the layout. `port/tools/packetbuild_test.sh` holds every builder to the bytes the 2008 construction gave. |
| Server packets | one handler per header in the dispatch of `CGameMode::PollNetworkStatus` (`GameModePacket.cpp`) and `CLoginMode::PollNetworkStatus` | A received layout is read in its handler and nowhere else. |

## What a version takes

1. **Compare.** `port/tools/packetdiff.py --rathena <checkout> -v <PACKETVER>` against the client's table. Each mismatch is one of three kinds: a request the client sends (rAthena names a `clif_parse_` handler), a packet the server sends (the client parses it; check `clif.cpp` for whether rAthena sends it at that version), or a length only (`nullptr`, neither side uses it). The struct pass (`354 rAthena structures, 0 differ`) checks the server's layouts against the client's.
2. **Choose the version at build time.** The enumerators are compile-time constants (the dispatch is a `switch`), so the version is a build switch, `BF_PACKETVER`, defaulting to 20080401; `port/tools/flags.sh` would pass it. A version that only moves fields could be chosen at run time instead, but not one that renumbers.
3. **Requests.** For each flagged request, a layout of that version in `PaddingPacket.h` (for example `PACKET_CZ_USE_SKILL_20080910`, with padding so that every field lands on the offset rAthena's packet db gives), and in its builder a `#if BF_PACKETVER >= ...` that fills that layout instead. The senders do not change. A request that is renumbered (sent under another id) gets its enumerator's value from the same switch in `packet.h`; a new request gets a new enumerator, a table row and a builder.
4. **Lengths.** In `InitPacketLenWithClient`, after the base rows, a block `#if BF_PACKETVER >= ...` that sets the version's lengths (`g_packetLenMap[HEADER_CZ_USE_SKILL] = sizeof(PACKET_CZ_USE_SKILL_20080910);`). `bifrost-packetlen` then prints the version's table and `packetdiff.py` at that version shows 0 request mismatches.
5. **Server packets.** A changed layout gets a struct of that version and a branch in its one handler; a renumbered one an enumerator value under the switch, so the dispatch picks it up.
6. **Check.** `packetids.py` exits 0; `packetbuild_test.sh` gets the version's expected bytes (built from rAthena's offsets) beside the 2008 ones; `packetdiff.py` at the version shows only length-only rows; `layoutcheck.py` passes; and the gate test passes against a server built at that `PACKETVER` (the run's server stays at 20080401; another `PACKETVER` is the owner's decision).

## Worked example: 20080910

`packetdiff.py --rathena <rAthena of 2026-11-23> -v 20080910` against the client's table (unchanged by M10):

```
client: 722 zone packets, highest header 0x035b
PACKETVER 20080910: 514 in common, 22 length mismatches (15 of them client requests), 6 server-parsed requests the client lacks
    0x0072 client   25  rathena   22  clif_parse_UseSkillToId
    0x007c client   42  rathena   44  nullptr  named in map/packets_struct.hpp
    0x007e client  102  rathena  105  clif_parse_UseSkillToPosMoreInfo
    0x0085 client   11  rathena   10  clif_parse_ChangeDir
    0x0089 client    8  rathena   11  clif_parse_TickSend
    0x008b client   23  rathena    2  nullptr  length only: no rAthena source names it
    0x008c client   11  rathena   14  clif_parse_GetCharNameRequest
    0x0094 client   14  rathena   19  clif_parse_MoveToKafra
    0x009b client   26  rathena   34  clif_parse_WantToConnection
    0x009f client   14  rathena   20  clif_parse_UseItem
    0x00a2 client   15  rathena   14  clif_parse_SolveCharName
    0x00a7 client    8  rathena    9  clif_parse_WalkToXY
    0x00f5 client    8  rathena   11  clif_parse_TakeItem
    0x00f7 client   22  rathena   17  clif_parse_MoveFromKafra
    0x0113 client   22  rathena   25  clif_parse_UseSkillToPos
    0x0116 client   10  rathena   17  clif_parse_DropItem
    0x0190 client   19  rathena   23  clif_parse_ActionRequest
    0x023d client    6  rathena   -1  nullptr  length only: no rAthena source names it
    0x02e2 client   14  rathena   20  nullptr  length only: no rAthena source names it
    0x02e3 client   25  rathena   22  nullptr  length only: no rAthena source names it
    0x02e4 client    8  rathena   11  nullptr  length only: no rAthena source names it
    0x02e5 client    8  rathena    9  nullptr  length only: no rAthena source names it
PACKETVER 20080910: 354 rAthena structures, 0 differ from the client
```

**The 15 requests** are exactly the builders of `PaddingPacket.h`. Their ids stay; their lengths and field offsets change. From rAthena's packet db (`clif_packetdb.hpp`, offsets of the fields its parser reads, in the parser's order):

| Request | Builder | 20080401: length, offsets | 20080910: length, offsets |
|---|---|---|---|
| 0x0072 CZ_USE_SKILL | `BuildCzUseSkill` | 25: 6, 10, 21 | 22: 9, 15, 18 |
| 0x007e CZ_USE_SKILL_TOGROUND_WITHTALKBOX | `BuildCzUseSkillTogroundWithTalkbox` | 102: 5, 9, 12, 20, 22 | 105: 10, 14, 18, 23, 25 |
| 0x0085 CZ_CHANGE_DIRECTION | `BuildCzChangeDirection` | 11: 7, 10 | 10: 4, 9 |
| 0x0089 CZ_REQUEST_TIME | `BuildCzRequestTime` | 8: 4 | 11: 7 |
| 0x008c CZ_REQNAME | `BuildCzReqname` | 11: 7 | 14: 10 |
| 0x0094 CZ_MOVE_ITEM_FROM_BODY_TO_STORE | `BuildCzMoveItemFromBodyToStore` | 14: 7, 10 | 19: 3, 15 |
| 0x009b CZ_ENTER | `BuildCzEnter` | 26: 4, 9, 17, 21, 25 | 34: 7, 15, 25, 29, 33 |
| 0x009f CZ_USE_ITEM | `BuildCzUseItem` | 14: 4, 10 | 20: 7, 20 |
| 0x00a2 CZ_REQNAME_BYGID | `BuildCzReqnameBygid` | 15: 11 | 14: 10 |
| 0x00a7 CZ_REQUEST_MOVE | `BuildCzRequestMove` | 8: 5 | 9: 6 |
| 0x00f5 CZ_ITEM_PICKUP | `BuildCzItemPickup` | 8: 4 | 11: 7 |
| 0x00f7 CZ_MOVE_ITEM_FROM_STORE_TO_BODY | `BuildCzMoveItemFromStoreToBody` | 22: 14, 18 | 17: 3, 13 |
| 0x0113 CZ_USE_SKILL_TOGROUND | `BuildCzUseSkillToground` | 22: 5, 9, 12, 20 | 25: 10, 14, 18, 23 |
| 0x0116 CZ_ITEM_THROW | `BuildCzItemThrow` | 10: 5, 8 | 17: 6, 15 |
| 0x0190 CZ_REQUEST_ACT | `BuildCzRequestAct` | 19: 5, 18 | 23: 9, 22 |

The 20080401 column is the client's own layouts (CZ_ENTER: AID at 4, GID 9, AuthCode 17, clientTime 21, Sex 25). So for 20080910, CZ_USE_SKILL becomes `{ short PacketType; char pad[7]; short selectedLevel; char pad2[4]; WORD SKID; char pad3[1]; DWORD targetID; }` (22 bytes: level at 9, SKID at 15, target at 18), `BuildCzUseSkill` fills it under `BF_PACKETVER >= 20080910`, and its table row says 22. The same for the other fourteen: 15 layouts, 15 builder branches, 15 table rows, no sender touched. One caution in this example: rAthena's second offset for 0x009f at 20080910 is 20, the packet's end, so its parser cannot read an AID there (clif_parse_UseItem reads the index only); a 20080910 layout would put the AID in the last four bytes (offset 16) and be checked against `clif.cpp`. The offsets are a guide to read with the parser, not to copy unread.

**The 7 other mismatches** are length only: 0x007c (ZC_NOTIFY_STANDENTRY_NPC; the structure pass finds rAthena's layout equal to the client's, and its packet db row has no handler), 0x008b, 0x023d and 0x02e2–0x02e5 (the Japanese forms of the requests above, which this client never sends). Nothing to do beyond checking, at the version, that rAthena does not send 0x007c at 44 bytes.

**The 6 server-parsed requests the client lacks** are 0x0436–0x0439 (the RE client's unpadded CZ_ENTER, CZ_REQUEST_ACT, CZ_USE_SKILL and CZ_USE_ITEM) and 0x0802/0x0806 (party booking, also in the 20080401 list). The client need not send them: rAthena still parses the padded forms at 20080910.

Note on the technical plan 2.4: its "rAthena expects renumbered requests" from 2008-09-10 is, by this output, re-laid-out requests under the same ids, plus the new 0x0436–0x0439 forms.
