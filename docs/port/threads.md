# Threads

Step M8.5 of the [implementation plan](implementation-plan.md): "Worker threads that call into the renderer and effects without locks (far-model loading) are made safe." This file lists every thread the client runs, what each touches while the main thread runs, what was done about it and why, and the sanitizer runs that show the result.

## The rule

The client is a single-threaded program with a few helper threads. The main thread runs `WinMain`'s loop, the modes, the world, the UI, the renderer and the d3d/ddraw adapter (`port/compat/d3d`) on top of `bf_gfx`, and drains the window events and the control socket's inbox when the client pumps. Nothing in `src/`, in the d3d adapter, the texture manager (`CTexMgr`), the resource manager's lists (`CResMgr`, apart from `Get`), the scene graph or the effects is safe to call from another thread, and the port does not try to make it so. A helper thread is acceptable only when what it shares with the main thread is behind a lock or an atomic.

Per thread, the 2008 design is kept where a lock makes it safe; where the thread existed only to hide loading time on 2008 hardware, its work runs on the main thread under `BF_PORT`, which the plan allows.

## Inventory

`_beginthreadex`, `CreateThread`, and the port's own `std::thread`, `pthread_create`, dispatch queues and framework callbacks, over `src/` (the units `sources.mk` builds) and `port/`:

| Thread | Started by | Touches while the main thread runs | Before M8.5 | Now |
|---|---|---|---|---|
| Far-model loader `LoadFarModels` | `CWorld::OnEnterFrame` (`GameWorld.cpp`), once per map, for the models more than 200 units from the player | `CResMgr::Get` (locked by `m_GetResSection`), and inside it `C3dModelRes::Load` → `CTexMgr::GetTexture`/`CreateTexture` (the texture map, unlocked) → `CTexture::Update` → the ddraw adapter's surface set and `bf_gfx` texture upload into the main thread's open command buffer (unlocked); `C3dActor` construction; `m_bgObjList.push_back` and `InterlockedIncrement(&m_bgObjThread)`, which the main thread reads plainly in `CWorld::ProcessActors` and then reads the actor's bounds in `SceneGraphNode::InsertObject` | Races, seen by TSan (below): the texture map, the adapter's surface set, `bf_gfx`'s command buffer, the counter and the actors it publishes. On arm64 the plain read of the counter does not order the reads of the actor behind it, which x86 did | **Runs on the main thread** under `BF_PORT`, at the end of `OnEnterFrame`, before the first frame. No thread is started, `g_farModelThread` stays NULL, `WaitForThreadEnd` has nothing to wait for. The models still reach the scene graph through `m_bgObjThread` in the first `ProcessActors`. `SetThreadPriority(HIGHEST)` is skipped, since it would now apply to the main thread |
| Music fade `FadeBgm` | `CMode::ProcessFadeIn` and `CMode::RunFadeOut` (`Mode.cpp`), at every fade (login, map change) | The stream functions in `device/Sound.cpp` (`SetStreamVolume`, `GetStreamVolume`, `PauseStream`), `FadeCnt`, `g_session.m_BgmVolume` | M7.3: the stream functions under one recursive lock (`g_streamLock`), `FadeCnt` atomic. Left: the thread read `m_BgmVolume`, which the options window writes on the main thread; and when it was 0 the thread returned after `FadeCnt++` without taking it back, so `FadeCnt` never returned to 0 and every later fade thread spun in `while (FadeCnt != 1) Sleep(5)` forever | **Kept as a thread** (the fade runs while the main thread draws the fade, as in 2008). Under `BF_PORT` the volume is tested by the main thread before the thread is started, and the thread no longer reads it. The thread's handle is never closed, as in 2008: a few bytes per fade |
| `FinalStrike` | `UIWindowMgr::OnKeyDown` ('P' with Ctrl), `UIWindowMgr.cpp` | `CPlayer::LaunchEffect` and `SendMsg` from the thread: the effects and the actor, unlocked | Unreachable: it needs `g_session.IsMasterAid(8)`, and `IsMasterAid` returns TRUE only for levels 7 (Gravity's own accounts), 5, 2 and 1 | Unchanged: never started. If it ever became reachable, it would have to post its effects to the main thread |
| GameGuard (`NPGameLib`, `CommPipe`) | `SpeedCheckEventThread`, `CommPipeThread` | | Not built (`CMakeLists.txt`: not game units) | Not built |
| Control socket I/O | `bf_control_start` (`port/native/control`), `std::thread` | Its `Io` record: inbox and outbox under `g_io.lock`, counters atomic; the main thread parses and carries out commands in `DrainInbox` when the client pumps | Safe by design | No TSan report |
| `bf_gfx` command buffer completion | Metal, on a GCD worker, after each `commit` (`port/native/gfx/src/bf_gfx.mm`, `submit`) | The free chunk list under `Shared::m`, `completed` atomic, the in-flight semaphore | Safe by design; TSan reports the block's captures (see "Reports left") | Unchanged |
| Audio render | AVAudioEngine's render thread, the voices' render blocks (`port/native/audio`) | Lock-free hand-over: atomics for commands, retired plays and gains; no locks, allocation or messages on the render thread | Safe by design | No TSan report |
| Audio queue | `dispatch_queue_create("bf_audio")` and the stream player's completion handler | `stream_copy_done` under the module's recursive lock (taken with `try_lock`, else deferred to the queue) | Safe by design | No TSan report |
| Single-instance message port | `CFMessagePort` source on the main run loop (`port/native/app`) | Runs on the main thread | Not a thread | |

The technical plan's row "Time and threads" names `device/Timer.cpp`: it holds only the frame timer (`timeGetTime` arithmetic on the main thread) and starts no thread. The one critical section it names is `CResMgr::m_GetResSection`; it stays, now taken by the main thread only (recursive, so harmless).

## Why the far models load on the main thread

- The thread existed for loading speed: the world appears while the far models trickle in. On this machine the release client loads Prontera's 1250 far models in 90 ms and prt_in's 456 in 30 ms (trace lines "Load Model :: N far models" and "Lazy Model Load Complete", `warp.py` with `BF_TRACE=...,game`), against roughly 1 s for the whole map change.
- Making the thread safe would need locks in the texture manager, the ddraw adapter's surface registry, `bf_gfx` (a texture upload encodes into the main thread's command buffer), the actor publication, and every place the main thread uses them: a new lock discipline through code the 2008 client never locked, for 90 ms.
- Frames after a map change now show every model from the first frame, which makes captured frames repeatable.

The 2008 path (`CreateThread`, its fallback) is kept under `#else`.

## Evidence

Builds: `cmake --preset tsan && cmake --build --preset tsan --target bifrost probes` (new preset: `-fsanitize=thread` through the cache flags, `CMakeLists.txt` unchanged). For ASan, the `asan` preset, and a recoverable address-only build of the same tree so that one run lists every report:

```bash
F="-fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer"
cmake --preset debug -B build/cmake/asan-only -DCMAKE_C_FLAGS="$F" -DCMAKE_CXX_FLAGS="$F" \
  -DCMAKE_OBJC_FLAGS="$F" -DCMAKE_OBJCXX_FLAGS="$F" -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address
```

Runs (world replay, no server; live walk through prt06 to prt_in and back with a fresh account):

```bash
export BF_TRACE=err,mode,res,game
BF_CLIENT=$PWD/build/cmake/tsan/bifrost TSAN_OPTIONS="log_path=$PWD/build/tsan/tsan halt_on_error=0 exitcode=0" \
  python3 port/harness/run.py world_replay
BF_CLIENT=... python3 port/harness/run.py warp --param fresh=1
```

| Build | Run | Result | Reports on the client's data | Reports in the Metal completion handler |
|---|---|---|---|---|
| tsan, before (macos-port 8a419ae) | world_replay | PASS but for the exit status (TSan's 66) | 16, all with `LoadFarModels` on one side: `CTexMgr`'s texture map, the ddraw adapter's `std::set<Surface*>`, `bf_gfx` `ensure_cb`/`upload`, `CWorld::ProcessActors` (counter and list), `TestAABBvsOBBIntersection`/`vector3d` (the actor's bounds) | 9 |
| tsan, after | world_replay (two runs) | PASS | 0 | 11, 12 |
| tsan, after | warp --param fresh=1 (two runs) | PASS | 0 | 5, 13 |
| asan-only, after | world_replay (two runs) | PASS | 0 | |
| asan-only, after | warp --param fresh=1 (two runs) | PASS | none about threads; in one run the effect-table read below (`RagEffect2.cpp:5899`, `:5900`) | |
| asan (preset), after | world_replay | PASS | 0 | |

The fade thread ran in every run (a fresh settings home has the music at 100), at the login fade and twice per map change.

## Reports left

**Metal completion handler** (`bf_gfx.mm`, `submit`): every remaining TSan report has the block passed to `addCompletedHandler:` on one side, run by `MTLDispatchListApply` on a GCD worker, reading its own captured variables (`std::shared_ptr<Shared>`, the `used` array, `NSMutableArray` enumeration), and on the other side the main thread writing those captures when the block is copied inside `addCompletedHandler:`, before `commit`. The handler runs after the GPU finishes, so the copy happens before it; the ordering goes through Metal and the kernel, which TSan does not see. These are reports about Apple's synchronisation, not about the client's data. If the gfx package wants a clean TSan log, a global mutex locked and unlocked by the main thread after `addCompletedHandler:` and taken first in the handler would make the edge visible to TSan at no real cost.

**Not about threads**, found by the same runs, for M8.6:

- UBSan (asan preset), `GameModePacket.cpp` `Zc_Notify_Standentry*`: `m_partyPosList.find(p->GID)` binds a reference to a misaligned packet field; fixed (the six sites pass `(DWORD)p->GID`, a copy).
- UBSan (asan preset), `GameActor.cpp:6120` `SetAttrState`: on the second map load, `CPlayer::OnInit` reaches `m_player->ReLaunchBlurEffects()` while `CWorld::m_player` is still NULL (`g_FirstEffect == 2`). Not fixed; it stops the `asan` preset's warp run at the second map.
- ASan, `RagEffect2.cpp:5899` `CEffectPrim::Render3DCasting`: `g_cosTable[m_GI[ec].rise_angle]` reads index -1. Main thread only; intermittent (three of four address-sanitizer warp runs, one of them with the 2008 loader thread restored, so not caused by this change).
