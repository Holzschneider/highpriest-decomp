# Reference screenshots (step M4.8)

The milestone verifiers judge captured frames against public screenshots of 2008-era clients (spec section 7). The images live outside git in `~/Games/Bifrost/test/reference/` next to an `index.md` that adds era evidence per file. This page is the rebuildable record: file name, SHA-256 and source URL. No image is committed.

## How the folder was assembled

Four public sources, all fetched with `curl` and each file looked at before it was kept:

1. **StrategyWiki** (`sw-*`): the Ragnarok Online interface captures, uploaded 2007-04. Window crops at 1:1 pixels in the classic blue skin (Basic Info, Equipment, Inventory, Status, Skill Tree, Option, Friend, Make a Room, minimap). Listed through the wiki's `list=categorymembers` API for `Category:Ragnarok_Online_images`; the web pages themselves sit behind a bot check.
2. **Official iRO beginner guide, 2008/2009** (`iro-guide-*`, `iro-official-*`): `iro.ragnarokonline.com/game/*.asp` as archived by the Wayback Machine (page headers show Pacific time 2008-12-30 to 2009-01-05); images from `img.ragnarokonline.com/game/image/` and `/game/screenshot/`. Gives the login window, the service (server) select list (Iris, Loki, Chaos), character select, character creation with the stat hexagon and the health-warning screen, plus small combat crops.
3. **iRO Wiki** (`irowiki-*`): files uploaded 2007-09 to 2009-08, found with the MediaWiki `list=allimages` API (`https://irowiki.org/w/api.php`, 6486 files, filtered by upload date and size, then viewed). Quest screenshots with dialogue boxes, a guild window, indoor and field maps. Mostly crops.
4. **Turkish Wikipedia** (`euro-ro-*`): three 1024x768 euRO screenshots uploaded 2006-05-22 (footer dates 2005-12 to 2006-04). They carry an euro-ro.net watermark and a date footer, and are older than the target; they are kept because they are the only full-frame captures with damage numbers, the minimap, the chat box and the Basic Info window together.

Tried and rejected: the RPGFan gallery (130 images, 2002 to 2004 UI with a title bar and a RagnarokOnline.com watermark), pRO "Level Up Games" screenshots with a date footer (2003 to 2005), the iRO Wiki gallery (full-size images were not archived), Fandom, MobyGames, Giant Bomb and ModDB (bot checks), Renewal-era material (2010+: new login screen, skill tree and status window), and the 2025 `Character_Select.png` of irowiki (modern client).

The `iro-official-2005-*` images are 2005 scenery with a URL watermark; use them for scenery and layout only. Items 5 (`irowiki-qwindow`) and all irowiki crops are partial windows or map snippets, not full frames.

## Coverage

| item | covered by |
|---|---|
| 1 login, licence | iro-guide-logon, iro-guide-health-warning (reduced thumbnail) |
| 2 server selection | iro-guide-serverselect |
| 3 character select and creation | iro-guide-charselect, iro-guide-charmake |
| 4 basic info, chat, minimap | sw-basic-info-window, sw-mini-map, sw-make-a-room-window, euro-ro-* (full frames) |
| 5 inventory, equipment, status, skills, options | sw-* windows, irowiki-qwindow |
| 6 NPC dialogue, shop | dialogue: irowiki-cowsong, -inputnumbers, -bookread; shop: only a fragment (irowiki-vending-info), no NPC buy/sell window found |
| 7 combat, damage, ground items | iro-guide-combat-damage, -mvp-damage, euro-ro-*; no clear capture of items on the ground |
| 8 field map, indoor map | euro-ro-2006-field-damage, irowiki-aragham-jr, -hermaton, iro-official-2005-*, irowiki-ninja-quest-1, -bed; no prt_fild08 capture |
| 9 party, guild, trade | irowiki-guildwindow, sw-friend-window; no party or trade window found |

No image is a full 800x600 frame of the plain 2008 client with an empty chat box and no overlay; the closest are the euRO frames (overlay) and the official 2005 scenery (watermark). Verifiers should compare layout, fonts and sprite quality, not exact pixels.

## Files

| file | SHA-256 | items | what it shows | source |
|---|---|---|---|---|
| euro-ro-2006-field-damage-1024.jpg | 482831cdb494c47425d0022e95bc95ae17c4d41c48b71a8531afe68b5df7f6f9 | 7, 8, 5 | 1024x768 field map: damage numbers, minimap, inventory ("Stok") window. Overlay: euro-ro.net mark and footer (1024x768) | https://upload.wikimedia.org/wikipedia/tr/8/8b/ROlvlup.jpg |
| euro-ro-2006-prontera-pvp-1024.jpg | cc8a41df680ebdad914a906ca11642866a60af9cd244dea141015b2af228dec4 | 4, 7, 5 | 1024x768 Prontera, Turkish client: Basic Info (Temel bilgi) window, minimap, chat window, damage numbers, status icons. Overlay: euro-ro.net mark and date footer (1024x768) | https://upload.wikimedia.org/wikipedia/tr/e/e3/ROPvP.jpg |
| euro-ro-2006-woe-combat-1024.jpg | 8962a01abc045384ddaa16d86d9d295b26de6a1567a97f754cdccb18dc07abb4 | 7, 4 | 1024x768 guild war: heavy damage numbers, skill icons bar, chat. Overlay: euro-ro.net mark and footer (1024x768) | https://upload.wikimedia.org/wikipedia/tr/2/2f/ROWoE.jpg |
| iro-guide-basicinfo-ingame.gif | 8f52e84613a667fafa2674cda8d40ed1f74fcee75819f5a286c9108b730f5153 | 4 | 160x100 crop: Basic Info window over a dual-throne room, chat bubble and HP bar (160x100) | http://web.archive.org/web/2008im_/http://img.ragnarokonline.com/game/image/about_4.gif |
| iro-guide-charmake.gif | 6ab47a7b489403ed22cb3b014836babc9ddab1f24e5140fd9b9a41a9fefbb238 | 3 | Character creation: sprite preview, name, stat hexagon (STR/AGI/VIT/INT/DEX/LUK = 5), OK/cancel (578x344) | http://web.archive.org/web/2008im_/http://img.ragnarokonline.com/game/image/game_3.gif |
| iro-guide-charselect.gif | 1dbaabb6529caa6a5695d9e233689cb0081f4eac84f2d65e1d0725e2b32ddc7f | 3 | Character Select window, three empty slots, stat table, make/cancel; full-width crop (578x353) | http://web.archive.org/web/2008im_/http://img.ragnarokonline.com/game/image/game_2.gif |
| iro-guide-combat-damage.gif | 92a0d2a891e5e7a582e77d46ab1cd661fecba2c024078938293f737c5c16b627 | 7 | 160x355 crop: combat, "Miss" and damage number 19 (160x355) | http://web.archive.org/web/2008im_/http://img.ragnarokonline.com/game/image/exp_2.gif |
| iro-guide-field-poring.gif | 4d08133d4fbe20f4b1f31f63f857db5717714794b7b6393f49355e8992e9aaa6 | 7, 8 | 160x355 crop: field map with Poring and two characters, HP bars (160x355) | http://web.archive.org/web/2008im_/http://img.ragnarokonline.com/game/image/exp_1.gif |
| iro-guide-health-warning.gif | 129fb9145cacd59cef44e37fc078fd0404c4ad8a541e53ab50a00efe4dff87df | 1 (licence/warning screen) | "WARNING: READ BEFORE PLAYING" epilepsy notice with agree/exit, reduced thumbnail (160x120) | http://web.archive.org/web/2008im_/http://img.ragnarokonline.com/game/image/server_4.gif |
| iro-guide-logon.gif | 6e01fa90a7af66d10e0d4c2100801e67144612156e89be52aed7f778d13834f8 | 1 | Login window (ID, Password, keep checkbox, login/exit), window crop, English iRO (282x122) | http://web.archive.org/web/2008im_/http://img.ragnarokonline.com/game/image/game_1.gif (page iro.ragnarokonline.com/game/startgame.asp) |
| iro-guide-mvp-damage.gif | 6e0a3f611f055bc7ab734d6e990a6f26c2bd2eb16b3b72e95d6cc5cd93662163 | 7 | 160x165 crop: MVP banner and damage number 10 (160x165) | http://web.archive.org/web/2008im_/http://img.ragnarokonline.com/game/image/exp_3.gif |
| iro-guide-serverselect.gif | 13839b9b41e69f832c7830497fe4d04541d1e318a366c36ddb287e6b83ea886d | 2 | "Service Select" list Iris/Loki/Chaos with OK/cancel, window crop (292x209) | http://web.archive.org/web/2008im_/http://img.ragnarokonline.com/game/image/server01.gif |
| iro-official-2005-ayothaya-dock.jpg | 4b442bc33bb73a5098474871da83c7756af28fafd1ab88c4b3b807acd7b82c92 | 8 | 800x600 field/town scenery (Ayothaya docks), no windows. Official site scenery shot with URL watermark and date footer (800x600) | http://web.archive.org/web/2008im_/http://img.ragnarokonline.com/game/screenshot/ayo_boatmarket.jpg |
| iro-official-2005-indoor-room.jpg | 250b02cf6e604755758d3f20b9c5f962bc3543e68e838d78fa8f2f7b3b883192 | 8 (indoor) | 800x600 indoor room (Ayothaya), no windows. URL watermark and date footer (800x600) | http://web.archive.org/web/2008im_/http://img.ragnarokonline.com/game/screenshot/ayo_party.jpg |
| irowiki-aragham-jr.jpg | ecf583bfecffb970f795802d4dc0066ab73755eba94305c1b7971ffd080e9bba | 8, 4 | 400x250 field/town crop with minimap and character (400x250) | https://irowiki.org/w/images/4/4e/Aragham_Jr.jpg |
| irowiki-bed.jpg | 9590c9ec3795d249a19b584db59d872d4ce419a2304294a4b26da8d51fb06a86 | 8 (indoor) | 288x258 indoor map with chat bubble and object label (288x258) | https://irowiki.org/w/images/3/30/Bed.jpg |
| irowiki-bookread.png | f3d760a8a024a3aa818ddd5ed36d92d0ffc8ef1de61aca847af69b480d6517c4 | 6 | 603x419 book text window (NPC/item text) (603x419) | https://irowiki.org/w/images/a/a6/Bookread.png |
| irowiki-cowsong.jpg | 6743bcd024afae0cd9add92ce11d4d2852e47b822230d358c6aa2fa29ea25dd3 | 6 | 278x178 NPC dialogue box with next button (278x178) | https://irowiki.org/w/images/9/9a/Cowsong.jpg |
| irowiki-guildwindow.jpg | 63db7a5faedf81a773bc89bb2b42b8c097f289c54fe15da6936b6351dd6c6329 | 9 | 402x319 Guild Info window (tabs, tendency, alliance) (402x319) | https://irowiki.org/w/images/a/aa/Guildwindow.jpg |
| irowiki-hermaton.jpg | 15523c280ebd8f922e5fb1a69c53e7decf6ca528c92cf4a8f44f722634102cd7 | 8, 4 | 432x336 town crop with minimap (432x336) | https://irowiki.org/w/images/5/5c/Hermaton.JPG |
| irowiki-inputnumbers.jpg | 765c09784d06cb4207a7c444a95fa317935fac9e56e9a47330d2846cf0214929 | 6 | 387x235 NPC dialogue with number input box (387x235) | https://irowiki.org/w/images/8/8f/InputNumbers.jpg |
| irowiki-ninja-quest-1.jpg | c970c3d8d4e6f9e40da13780cd5cfdb255da579db03fe9df6d125ce4b31010cf | 8 (indoor) | 637x468 indoor map, character with name label (637x468) | https://irowiki.org/w/images/b/b7/Ninja_quest_1.JPG |
| irowiki-qwindow.jpg | 4b36a3c4a0cd7a99a2e64d465849356d58833d1743510e81797e3f80671689c6 | 5 (quest list) | 279x203 Quest List window (279x203) | https://irowiki.org/w/images/8/80/QWindow.jpg |
| irowiki-spring-brandos.jpg | fa3ee80983787fb1883c7c461682a0b6202a2070a999debb02d6b7e2dee800ca | 6, 8 | 429x325 NPCs with chat bubbles and minimap overlay (429x325) | https://irowiki.org/w/images/5/53/Spring_Brandos.jpg |
| irowiki-vending-info.gif | 01afd35563bb1de007b5de6605031cce64d90d9f37d879108f7702e12114266a | 6 (partial) | 160x100 fragment of the "Vend a Shop" window (160x100) | https://irowiki.org/w/images/a/a5/Vending_Info.gif |
| sw-basic-info-window.gif | 2f2bdc561de1b314f68936873a05a40bef0d0d4abade591d4f912174b41c017e | 4 | Basic Info window (job, HP/SP, base/job level, weight, zeny, status/option/items/equip/skill/map/chat/friend buttons), 280x120 crop (280x120) | https://cdn.wikimg.net/en/strategywiki/images/0/04/Ragnarok_Online_Basic_Info_Window.gif |
| sw-equipment-window.gif | 67e38e275a83c2ff15b0167921dc838d213a7bc4e2b18e745a5e57231c1c2246 | 5 | Equipment window with paper doll, 280x147 crop (280x147) | https://cdn.wikimg.net/en/strategywiki/images/2/2c/Ragnarok_Online_Equipment_Window.gif |
| sw-friend-window.gif | 4ac612a838de0556485e41aa46672e6dce9a55bc99ad514ad29d950a845b1e0b | 9 | Friend window (0/40), Friend/Party radio, 140x155 crop (140x155) | https://cdn.wikimg.net/en/strategywiki/images/4/49/Ragnarok_Online_Friend_Window.gif |
| sw-inventory-window.gif | a83ce4e9f44ab96e1649b196b530ab01c0278b100624763a9cac876f4c3f687d | 5 | Inventory window item tab, 280x102 crop (280x102) | https://cdn.wikimg.net/en/strategywiki/images/6/6b/Ragnarok_Online_Inventory_Window.gif |
| sw-make-a-room-window.gif | 46ab742a0b8455b349bd89c2e0716f670df0c049081fff0190fc88d75b0ee958 | 4 (chat) | Make a Room (chat room) dialog, 280x120 crop (280x120) | https://cdn.wikimg.net/en/strategywiki/images/e/ec/Ragnarok_Online_Make_a_Room_Window.gif |
| sw-mini-map.gif | 14b5de05193eea2d8415a67f8738d4b6fab65e8cf92e34c1d5f34c64028bc824 | 4 | Minimap, 129x129 crop (129x129) | https://cdn.wikimg.net/en/strategywiki/images/d/d3/Ragnarok_Online_Mini-map.gif |
| sw-options-window.gif | f598a13374de1a056c2d18f678847edda01daf2aa934ef1b29d6e8a88993c605 | 5 | Option window (BGM/effect sliders, skin, snap), 280x120 crop (280x120) | https://cdn.wikimg.net/en/strategywiki/images/0/07/Ragnarok_Online_Options_Window.gif |
| sw-skill-tree-window.gif | aee45947f32d8424c9212951ba45d7bba52bf5fffe86898075dc759fc41680e4 | 5 | Skill Tree window (Thief), 280x216 crop (280x216) | https://cdn.wikimg.net/en/strategywiki/images/0/0a/Ragnarok_Online_Skill_Tree_Window.gif |
| sw-status-window.gif | 5cb561f8cb5f9b8422f6abdb170b5c52fde8ce0118a86acc665cf978105fd718 | 5 | Status window (stats, ATK/DEF, Status Point), 280x120 crop (280x120) | https://cdn.wikimg.net/en/strategywiki/images/3/3d/Ragnarok_Online_Status_Window.gif |

## Check

```
ls -la ~/Games/Bifrost/test/reference/
cd ~/Games/Bifrost/test/reference && shasum -a 256 $(ls | grep -v '^index.md$') | sort -k2
```

The hashes must equal the SHA-256 column above. To rebuild, download each source URL into the folder under the file name given (the Wayback URLs redirect to the nearest capture, the content is stable); the iRO Wiki and StrategyWiki files are plain downloads.
