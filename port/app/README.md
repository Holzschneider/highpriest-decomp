# app: the application bundle

The client as a macOS application (step M12.1): `<name>.app`, ad-hoc signed,
built by the target `app` of the top-level `CMakeLists.txt` (part of `all`).
`<name>` and the bundle identifier come from `port/name.cmake`, the minimum
macOS from `port/tools/flags.sh`; no path or file here spells the name out.

```
cmake --preset debug && cmake --build --preset debug       # build/cmake/debug/<name>.app, among the rest
cmake --build --preset debug --target app                   # the bundle alone
open build/cmake/debug/<name>.app                           # what Finder does
codesign --verify --deep --strict build/cmake/debug/<name>.app
```

## What is in it

```
<name>.app/Contents/Info.plist              from Info.plist.in
<name>.app/Contents/PkgInfo                 APPL????
<name>.app/Contents/MacOS/<exe>             the client executable, <exe> = the name in lower case
<name>.app/Contents/Resources/<name>.icns   drawn by bifrost-icon (bf_icon.cpp)
<name>.app/Contents/_CodeSignature/         the ad-hoc signature
```

- **The executable** is a copy of `build/cmake/<preset>/<exe>`, the bare
  client the fast check and `BF_CLIENT` use. It is linked once. The bare
  executable stays, so that the fast check, the harness and the sanitizer
  presets work as before. Each preset makes its own bundle from its own
  executable (an `asan` bundle finds the sanitizer runtime through the
  executable's absolute `LC_RPATH`, as the bare one does).
- **Info.plist**: identifier, executable, name, version (`BF_APP_VERSION`,
  `BF_APP_BUILD`, cache variables), `LSMinimumSystemVersion` 14.0,
  `LSArchitecturePriority` arm64, `NSHighResolutionCapable`,
  `NSPrincipalClass` NSApplication, `LSApplicationCategoryType`
  role-playing games (a games category), and no automatic or sudden
  termination, since the client quits by itself (`port/native/app`).
  `NSLocalNetworkUsageDescription` gives the reason macOS shows when the
  client connects to a server on the local network; loopback (the test
  server) needs no permission.
- **The icon** is drawn from shapes in code (a rainbow bridge over a night
  sky); no game artwork is in it.
- **No game data** is in the bundle. The game folder is chosen at run time:
  `Paths/GameFolder` in the settings, `BF_CLIENT_DIR`, `~/Games/<name>`, or
  the chooser of `port/native/prefs` when none of them is usable.
- **Signature**: `codesign --force --sign - --timestamp=none --identifier
  <bundle id>` over the bundle, then `codesign --verify --deep --strict`;
  both run in the build step, so a bundle that does not verify fails the
  build. No entitlements, no hardened runtime: nothing needs them, and an
  ad-hoc signature cannot be notarised anyway. The fast check (`--client`)
  verifies the signature again.

## Started from Finder

LaunchServices starts the executable with launchd's environment (no
`BF_*`, the system `PATH`), `/` as working directory, stdout and stderr on
`/dev/null`, and no `-psn_` argument on current macOS (`bf_main.cpp` drops
one if it comes). Nothing in the client depends on the working directory or
on the executable's location: game files go through `bf_file` relative to
the game folder, writes go to the folders under the home, the settings live
in `~/Library/Application Support/<name>/Settings.plist`, the
single-instance lock and message port are keyed by the bundle identifier
compiled in (`port/native/app`), and the control socket opens only when
`--control` or `BF_CONTROL` names one. The client calls no `NSBundle` API.

- **First start, no usable folder**: the chooser panel opens (seen in a
  bundle launch with `Paths/GameFolder` set to an empty folder: the trace
  says `prefs: configured game folder is not usable: ... holds no .grf
  archive.` and the main thread waits in `-[NSSavePanel runModal]`). The
  choice is stored in `Paths/GameFolder`.
- **Later starts**: the stored folder is used; the trace's first line
  names it (`start: ..., game folder <folder>`).
- **AppKit's own defaults**: the open panel remembers its size in
  `~/Library/Preferences/<bundle id>.plist`, as every application's does.

## Tested by

`BF_APP=<build>/<name>.app python3 port/harness/run.py <script>`: the
harness launches the bundle with `open` from a Finder-like environment and
stores the game folder in the run's settings (`port/harness/README.md`,
"Launching the bundle"). `gate`, `intro` and `granny` pass that way.

## For `port/README.md`

> **The application bundle** (`port/app`, M12.1). `cmake --build --preset
> debug` also makes `build/cmake/debug/<name>.app` (target `app`): the
> client executable, an Info.plist from `port/app/Info.plist.in` (bundle id
> and name from `port/name.cmake`, macOS 14, arm64), an icon drawn by
> `bifrost-icon`, no game data, ad-hoc signed and verified in the build. The
> bare executable `build/cmake/debug/<exe>` stays for the fast check and
> `BF_CLIENT`. `open build/cmake/debug/<name>.app` starts it as Finder does;
> the game folder comes from the settings or the chooser.
> `BF_APP=<path>.app python3 port/harness/run.py gate` runs a harness script
> against the bundle launched through LaunchServices, with the game folder
> stored in the run's settings (`BF_APP_FOLDER=links` stores a folder of
> links, so that the trace proves the setting was used). The fast check
> (`--client`) builds the bundle and verifies its signature.
