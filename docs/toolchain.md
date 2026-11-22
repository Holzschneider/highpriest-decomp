# Toolchain

The build reads the tools and SDK files below. Only files a build opens are kept. This page says what each one is and where it came from, so it can be obtained again and checked. `sources.mk` says which compiler each source gets.

All of it is Microsoft's except the runner, and none of it is covered by this repository's licence ([THIRD-PARTY.md](../THIRD-PARTY.md)).

## Visual C++ 6.0 SP6: `tools/msvc6`

The compiler of the game and of most library sources, the linker, the librarian and the resource compiler.

| File | What | Size | SHA-256 |
|---|---|---:|---|
| `CL.EXE` | driver, 12.00.8804 | 51,200 | `1bf99f206271ecdbd13da2829192ea2c02e2a44c740b8d72935d5d9cb753b156` |
| `C1.DLL` | C frontend, 12.00.9782 (SP6) | 696,369 | `fc6771ed73524417877f1263604bf8b9b9cd403c8cfb72b2e25737bdc81d592e` |
| `C1XX.DLL` | C++ frontend, 12.00.9782 (SP6) | 1,220,659 | `ab4610ad56f7278a9c89d055e4189bd8037c7340a21bac5434fe24c7d5a7526d` |
| `c2.dll` | backend, 12.00.9782 | 737,329 | `3f2b5f43e3172d38651886bd2599957298d9fa42d06684a752e381c17572b12f` |
| `mspdb60.dll` | program database writer, 6.00.8168 | 180,276 | `7c90b80d42914562332e28c253415e72a80c6651003d08a9ffdc4ba6e27fc37c` |
| `LINK.EXE` | linker and librarian, 6.00.8447 | 462,901 | `9672e578fdfaa43bdb8e9c16071682988665cb90bba2904bf02f6a5576d8ffbc` |
| `CVTRES.EXE` | resource converter, 5.00.1736.1 | 15,632 | `83b602ed8e69e979fc9557f482a4a4c6c9a97b4ad67b879aedeacd2b09e5b20b` |
| `RC.EXE` | resource compiler, 5.00.1641.1 | 7,440 | `c1eaea71c0d862b08715f7c992f090c0e15b4b07a99386719fa32732aca49c8e` |
| `RCDLL.DLL` | resource compiler, 5.00.1641.1 | 107,792 | `9dbe97d6f641f34470cd8ac4eba361915a53b317fe5ff3c11b94fc8cec35ce68` |

- The base tree is the `VC98` directory of Visual Studio 6.0 Enterprise, Disc 1. `mspdb60.dll`, `RC.EXE` and `RCDLL.DLL` are from the disc's `COMMON/MSDEV98/BIN`.
- The frontends and the driver are from Service Pack 6 (`vc98/bin` in `VS6sp61.cab` of the installer).
- `c2.dll` is the Professional and Enterprise backend, which the Service Pack 6 installer stores as `msvcep.dll`. The same file is `VC98/Bin/C2.DLL` in [itsmattkc/MSVC600](https://github.com/itsmattkc/MSVC600) at commit `001c4bafdcf2ef4b474d693acccd35a91e848f40`.
- `mspdb60.dll` is the unbound image: no bound import table and a stored PE checksum of 0. An installed copy is the same image with its imports bound.
- `LINK.EXE` 6.00.8447 is the linker build the original's Rich header names. `CVTRES.EXE` writes the producer string `Microsoft CVTRES 5.00.1735.1`, which the original's resource object carries.
- `RC.EXE` does not run under wibo (it needs `user32!CharNextA`), so the build runs it under Wine.

`Include/` holds the 136 headers the compiles open, `MFC/Include/` the two the resource script includes. The digest of the headers' digests (`cd tools/msvc6/Include && find . ! -type d | LC_ALL=C sort | xargs sha256sum | sha256sum`) is `1696e9d82bc6df6a47bf4e51432837626d463610d6b948800dc5ca5a80cd293e`. `Include/strstream` is absent from the disc and comes from the MSVC600 commit above (3,747 bytes, SHA-256 `9be64c4536f1f2173ba14e729629a430670828965d27b6e59034139e866d04de`).

`Lib/` holds the import libraries the link reads (`ADVAPI32`, `GDI32`, `IMM32`, `KERNEL32`, `NETAPI32`, `OLE32`, `SHELL32`, `USER32`, `WINMM`, `WS2_32`), `OLDNAMES.LIB`, `UUID.LIB`, and `LIBCPMT.LIB` (735,302 bytes, SHA-256 `08cd1b3ba2eab087c2d2acabf7a6919362aff59ed87141f167cbe12bee6a732c`), from which the build takes the one member it does not compile, `wlocale.obj`.

The other files of `Lib/` and `MFC/Include/`:

| File | Size | SHA-256 |
|---|---:|---|
| `Lib/ADVAPI32.LIB` | 117,982 | `88e77c6590be2a9139c0015c1f07965d7c261cc552de4209d6e8e8d86c4c11c4` |
| `Lib/GDI32.LIB` | 81,080 | `dc99cf3da3e3e6b2efeee7724a6cb400a615f8e2dde9ebf6df6b3bf3eb3d7c1b` |
| `Lib/IMM32.LIB` | 19,684 | `eb5e8621914af650a8717b2d1789084cdc8d795b8c9a1a811969476c5436f161` |
| `Lib/KERNEL32.LIB` | 175,226 | `cd4a546f71481319aeee455aa0fb51698a094f9d3e6d8339acf445489a263c68` |
| `Lib/NETAPI32.LIB` | 69,536 | `e6e4b1620c5386d7a17e652df38e8a7282ec38ea4bf0d57948b73964fd960aed` |
| `Lib/OLDNAMES.LIB` | 65,372 | `348a220e94f90bd9f530514ae59ff9b044b76e2056f52b45df406c12d2156afb` |
| `Lib/OLE32.LIB` | 68,152 | `f31f06ecaf4d8c5e9990cd65f752e9cc7466bf3694555c3914219cbfaf2026a9` |
| `Lib/SHELL32.LIB` | 97,768 | `53150052405bbce4f264226a7bd86d3496bafe8c4bc2252835415826d996bd53` |
| `Lib/USER32.LIB` | 135,444 | `0a3c34c4daec09c1b9fd163d5d0b2ee9a6cbe0b3e863ae81a1fae3b9f936fd19` |
| `Lib/UUID.LIB` | 1,110,586 | `118b8c80a457e4dbc19bb8a54d49e193f16caa3287ab516427f52165a9caa0bf` |
| `Lib/WINMM.LIB` | 43,982 | `bb098679a802afdbc6fdaac03dfe1ea81453a36f6d4768b1c1cd092b1dbe9342` |
| `Lib/WS2_32.LIB` | 24,954 | `dbe9de991aea6662d8857da13e572729b5379377be8e13bdd86e57a51354988e` |
| `MFC/Include/AFXRES.H` | 32,525 | `e6a3b06901272e0f83eedecf03b3e324fc335996d4642be29581ebf628a799ad` |
| `MFC/Include/WINRES.H` | 1,151 | `e3bb99dea5759d68acaf1cfa8996144685f86503c7db12905f01dc43dda59fc1` |

## Visual C++ 12.00.8047: `tools/msvc6_8047`

A pre-release build of Visual C++ 6.0. The members of the stock `LIBCPMT.LIB` are stamped with this compiler build, and the 16 sources of the C++ library that the build compiles use it.

| File | What | Size | SHA-256 |
|---|---|---:|---|
| `CL.EXE` | driver, 12.00.8047.2 | 51,200 | `b542a0b1f585b0fd3b402cf33ff87912b2b2d0b65397449959176c963cc798c8` |
| `C1.DLL` | C frontend | 647,212 | `19fb7eb500e4fa884fe170434ddb42a5ddc5e065f497c2dd233e58e27f7052a6` |
| `C1XX.DLL` | C++ frontend | 1,161,774 | `2e9047a7d14d766037b8a628629f423c110a04d57e6f57ba463086f1bb4f201b` |
| `C2.DLL` | backend | 733,228 | `10234624da584d13db1ff985dc4cf3582482cdd3626ede2e82c8200185932ac7` |
| `MSPDB60.DLL` | program database writer | 180,224 | `7d0fd77349d6aee593baf34f3ae920f7ffc6a7aa4cba1590b1d7a9cbab443dde` |

The files are `VC98/BIN/CL.EXE`, `C1.DLL`, `C1XX.DLL`, `C2.DLL` and `COMMON/MSDEV98/BIN/MSPDB60.DLL` of a Visual Studio 98 Beta 2 distribution: archive.org item `vs-6.0-build-8056-beta-2`, file `VS6.0 build 8056 (Beta2).zip` (117,632,536 bytes, SHA-256 `208a88dad6e616f50dd7381897df4cee53679ad8e7513b15017fb1ad4bd03c46`). The zip holds 40 inner zips, each with one volume of a RAR set:

```bash
7z x 'VS6.0 build 8056 (Beta2).zip' -oouter
for z in outer/LNDVS*.ZIP; do 7z x "$z" -ovol; done
cd vol && unrar x -y vs98b2.rar 'VC98/BIN/*' 'COMMON/MSDEV98/BIN/MSPDB60.DLL' ../out/
```

Volume `vs98b2.r26` is missing from the distribution; nothing the compiler needs is in or after it. The distribution is not an authenticated Microsoft release. The four DLLs store a PE checksum that equals the recomputed one, so they were not changed after the checksum was written, and the objects the compiler produces equal the stock library's members.

## Visual C++ backend 12.00.8943: `tools/msvc6_8943`

The backend of the Processor Pack for Visual Studio 6.0 SP4. The one object the stock Miles library holds, `cleanup.c`, compiles with it. C only: there is no C++ frontend.

| File | What | Size | SHA-256 |
|---|---|---:|---|
| `CL.EXE` | driver, 12.00.8804 (the file in `tools/msvc6`) | 51,200 | `1bf99f206271ecdbd13da2829192ea2c02e2a44c740b8d72935d5d9cb753b156` |
| `C1.DLL` | C frontend, 12.00.8867 | 682,545 | `0792033fc3d2fec262ff95478c6a9e70a06e836ed55f0a9710200fa06c204754` |
| `C2.DLL` | backend, banner 12.00.8943 | 860,209 | `24dce657c10d80da39679ccd03192c00377c1a646794495be8f555eaddd816c3` |
| `MSPDB60.DLL` | the file in `tools/msvc6` | 180,276 | `7c90b80d42914562332e28c253415e72a80c6651003d08a9ffdc4ba6e27fc37c` |

- `C2.DLL` is `c2.dll` of the Processor Pack installer `vcpp.exe` (1,186,960 bytes, SHA-256 `a9c019a8c7e2d1d320b6bfdf570c156caffe198b27a1d6a86ce07cbb838d2598`), once at `http://msdn.microsoft.com/vstudio/downloads/ppack/vcpp.exe` and kept by the Wayback Machine (capture `20010416025530`). `7z e vcpp.exe c2.dll` extracts it.
- The Processor Pack replaces only the backend. `C1.DLL` and `CL.EXE` are those of Service Pack 4 and of Service Pack 5, which are the same files; these are from `vc98/bin` of the SP5 installer `vs6sp5.exe` (SHA-256 `10a834041223ad21181c0e87c0a90930d976498556af2e8f959a044101e6a604`).

## Windows DDK compiler 13.00.8830: `tools/ddk2266`

The compiler of a pre-release Windows XP DDK (build 2266.1). The two DirectX objects built from source, `DxGuid.cpp` and `dilib1.c`, compile with it.

| File | What | Size | SHA-256 |
|---|---|---:|---|
| `CL.EXE` | driver, 13.00.8827 | 81,920 | `d877dad2ec0d38e76de6aa908bbfd490bfca7389d125d2664a7071446ccea189` |
| `C1.DLL` | C frontend, 13.00.8831 | 843,776 | `a22a69ad1ccb1e1f731b00da8bd5dd5437d4a378691d9a25c3af7559432d154c` |
| `C1XX.DLL` | C++ frontend, 13.00.8830 | 1,994,752 | `5121250e02013cf3d6c88bb84c65ca35fc7cb7ad376d614d49f62019160f7800` |
| `C2.DLL` | backend, 13.00.8830 | 1,253,376 | `a1097f90eb9c3ef9864e4421dbc66e81eb4aeb57ecf9dcd8066df042cd74c3e9` |
| `MSPDB60.DLL` | program database writer 6.20.8687, loaded with `/Zvc6` | 192,557 | `cdbe6f922e08282d8b531c89b1b16140397fade98cd5734a05707c85e806d147` |
| `MSPDB70.DLL` | program database writer 7.00.8827 | 217,088 | `6d8a3b8f1e979fdfba3f9d443a3435747ee182a9512016d2f2e84dad903b6b0d` |

The files are members of `common/X86dBINS.cab` (3,861,352 bytes, SHA-256 `e151bf05c3e96568c00df5e6b30f7bfd49c5e4c6ec347d34bf161b2c4a9be39d`) of the ISO `Microsoft Windows Driver Development Kit For Windows Codename Whistler Build 2266.1 (DbgBuild.000906-2246).iso`, in archive.org item `osx86microsoftwindowswhistlerprofessionaleditionbuild2267prebeta1checkeddebugen`. `7z x X86dBINS.cab` extracts them, and `common/X86dBINS.inf` maps each member (`X86dBINS_FILE_1` to `_4`, `_18`, `_20`) to its file name. The DDK's own build settings (`i386mk.inc` in `common/CMNBINS.cab`) pass `/Zvc6` to every compile.

## MASM 6.13.7299: `tools/masm613`

The assembler of the runtime's 41 assembly sources.

| File | What | Size | SHA-256 |
|---|---|---:|---|
| `ml.exe` | ML 6.13.7299 | 351,744 | `3159512d1fb4f8b9e4c9ec4125d06c3e9d9adc5a675715670f4ea705f5472705` |

`ml.exe` is the result of Microsoft's ML 6.13 update, an RTPatch patch, applied to the ML 6.11d of the Windows 98 DDK.

| Input | Origin | Size | SHA-256 |
|---|---|---:|---|
| `MASM 6.13.zip` | archive.org item `masm-6.13` | 6,500,778 | `61ab1f2e14622a3721097e32aaadc436f8ebd385d361c5944063d7b1fa76c8ce` |
| `update.613.exe` | in the zip, `MASM 6.13/UPDATE.613/`; a self-extractor holding `ML613/` | 2,650,077 | `273f9f000a6f3241855b94572ad26abdd6fffe04192c24bc858a3a5c1d5d84b3` |
| `ML613/PATCH.EXE` | RTPatch 1.11, a 16-bit DOS program | 76,981 | `a80d0e2c8b6a8655e52030278966501a303cc1dfbe2caf4b8500d710062670f6` |
| `ML613/PATCH.RTP` | the patch data | 596,271 | `97272b9ce40186071cb200fc3aca8b248fba75c7a5ec06245d37fd14b9d5c343` |
| `OCT98MSDN.iso` | archive.org item `msdn-disc6-october-1998-x03-86662` (Windows 98 DDK) | 57,147,392 | `228e077afe807e5344d879bd801ae85abfe67078a895e983ca9016deed4c899d` |
| `BIN_WIN98_ML.EXE` | in the ISO's `CABS/I386/BINS_DDK.CAB`; ML 6.11d | 388,096 | `33455c7e38348ffda7b73eb66b818f0c00e5d8d52f08eae665616e3a5c9c9b8b` |
| `BIN_WIN98_ML.ERR` | in the same CAB | 9,461 | `6d8ca8b5e41c601531639814bf8fa3b6c7db289c83bcfeee5c9fdab79905431f` |

The patcher runs under [emu2](https://github.com/dmsc/emu2) (commit `9d8698d06e67359bc8b230c2d19d5b1503e60819`):

```bash
7z x 'MASM 6.13.zip' -oA
7z x 'A/MASM 6.13/UPDATE.613/update.613.exe' 'ML613/*' -oB
7z x OCT98MSDN.iso CABS/I386/BINS_DDK.CAB -oC
7z x C/CABS/I386/BINS_DDK.CAB BIN_WIN98_ML.EXE BIN_WIN98_ML.ERR -oD
mkdir run && cp D/BIN_WIN98_ML.EXE run/ML.EXE && cp D/BIN_WIN98_ML.ERR run/ML.ERR && cp B/ML613/PATCH.EXE B/ML613/PATCH.RTP B/ML613/PATCH.RTD run/
cd run && emu2 PATCH.EXE </dev/null    # writes ml.exe and ml.err
sha256sum ml.exe
```

The patcher checks its base: with one byte of `ML.EXE` changed it stops with `error ept0036: Invalid copy of file found`.

## DirectX 8.0 SDK: `tools/dx8sdk`

Headers the compiles search ahead of the compiler's own, and two import libraries. The files are Git blobs of [edgeforce/directx8](https://github.com/edgeforce/directx8) at commit `e6fa10f5f92bf523c15fdb068206ce25c8d2121d`, unchanged, from its `include/` and `lib/`.

| File | What | Size | SHA-256 |
|---|---|---:|---|
| `Include/basetsd.h` | header | 6,971 | `8b773005c8e432e4780eda02d0bdd6f079396f4203d080fff5aec93badd5ae03` |
| `Include/d3d.h` | header | 88,007 | `acbee82f353ede18b33ffd98d034610edd4c278f69deeb7f40a254323c5ac357` |
| `Include/d3dcaps.h` | header | 26,001 | `ca9d464596654cabbbcf1a0ee73d29c77aa93230d6d0cbed68660f77435a6e8f` |
| `Include/d3dtypes.h` | header | 79,885 | `e1b807f582d51edd50e06a2edb0392a05581b8f03151eea1c381b623e981f707` |
| `Include/d3dvec.inl` | header | 5,476 | `f041f307b41aa6caaa0a5f5ad2bbbd2e9690b73c0677f29ddfe7d30933589f04` |
| `Include/ddraw.h` | header | 240,099 | `0a02fd4be23ceef5c8e59518179b3784d314c272ee68b74a3cf7775dff25c48b` |
| `Include/dinput.h` | header | 226,315 | `abbe366581495567abd7bd2377ad5e9cdc846402688d2d52b65b0a56b71c7213` |
| `Include/dsound.h` | header | 109,997 | `a8fc649d12ee17415335b52fc700122c5660d8ed6b6121f7a545aeed84e5a309` |
| `Lib/ddraw.lib` | import library | 4,540 | `5d09dfd37c308e2b78a213e8ddaf547d1e785d9a56b6843e49cbfc015a8c9149` |
| `Lib/dinput.lib` | import library and five objects | 19,104 | `be0e10078acc1bee14a37344755f474442e290c3f1aab8e02eb0c8162e1c93ad` |
| `Lib/dsound.lib` | import library | 7,226 | `f8b660968270801ecf4b0cf0ff2d00f0687cf1fdbc3936c8e6d82ad09bec085c` |

The build takes the import members of `dinput.lib` and replaces its objects with `src/library/dinput/dilib1.c`, the only one the original links. The SDK's `dxguid.lib` is not kept: `src/library/DxGuid.cpp` gives its one object.

## Runtime sources: `vendor/`

Sources of the Visual C++ 6.0 runtime that the units under `src/library/msvc6/` include.

- `vendor/msvc6/crt`: the runtime sources Visual Studio 6.0 Enterprise ships on Disc 1, with the source and header updates of Service Pack 6, unchanged.
- `vendor/msvc-internals/nt5` and `opennt`: sources of runtime modules whose source Visual C++ 6.0 does not ship (conversion and transcendental routines and some helpers), unchanged from mirrors of the Windows 2000 (NT5) and OpenNT source trees. They are not an authenticated Visual C++ 6.0 source release; each was chosen by comparing its object with the stock library's member.

## Runner: `tools/bin/wibo`

[wibo](https://github.com/decompals/wibo) at commit `74517de90dc92855571c8fb76941b1051bc41d8e` with `tools/wibo/fixed-time.patch` applied; 5,490,696 bytes, SHA-256 `ba3c33555f750859ec8a29890a4f512969b6a5c6f80be192a1160e3e47b4295a`. It is dynamically linked and needs glibc 2.38 and the libstdc++ of GCC 13, or newer. It embeds mimalloc, liburing and Wine's C runtime DLLs, which its build fetches ([THIRD-PARTY.md](../THIRD-PARTY.md)).

The patch adds `--fixed-time SECONDS`, which freezes the wall clock the guest sees, for the process and its wibo children. It covers `GetSystemTimeAsFileTime`, `GetSystemTime`, `GetLocalTime`, `NtQuerySystemTime` and the current-time lookup of `GetTimeZoneInformation`. File times, monotonic clocks and waits are unchanged. The compilers write the time into their objects, so the `Makefile` passes the same value to every run.

To rebuild it:

```bash
git clone https://github.com/decompals/wibo.git wibo-source
git -C wibo-source checkout --detach 74517de90dc92855571c8fb76941b1051bc41d8e
git -C wibo-source apply "$PWD/tools/wibo/fixed-time.patch"
python3.12 -m venv wibo-venv
wibo-venv/bin/python -m pip install clang==17.0.6
export SOURCE_DATE_EPOCH=$(git -C wibo-source log -1 --format=%ct HEAD)
cmake -S wibo-source -B wibo-build -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$PWD/wibo-source/cmake/toolchains/x86_64-linux-gcc.cmake" -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld -DWIBO_ENABLE_LIBURING=ON -DWIBO_ENABLE_LTO=OFF -DPython3_EXECUTABLE="$PWD/wibo-venv/bin/python"
cmake --build wibo-build -j8
```

The runner embeds its build time, which `SOURCE_DATE_EPOCH` fixes. The binary's identity also depends on the host compiler and libraries.
