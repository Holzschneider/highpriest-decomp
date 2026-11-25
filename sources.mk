# What is compiled how, and the order of everything the link depends on.
# Objects are named by their source path under src/, without the extension.

# The game objects, in the order of the link command line.
GAME := \
  3dlib/Anim 3dlib/D3dutil 3dlib/DC 3dlib/Device 3dlib/DrawUtil 3dlib/mesh 3dlib/Prim \
  3dlib/Renderer 3dlib/Texture device/Connection device/Control device/EncClient \
  device/GronPacket device/md5 packet device/PacketQueue Padding device/SequenceRandomGenerator \
  device/Sound device/Timer VariablePacket resource/Action resource/Attr resource/Bitmap \
  resource/Emblem resource/EZeffect resource/Ground resource/ImfRes resource/Model \
  resource/PaletteRes resource/Skybox resource/Sprite resource/Wave resource/World skin SnapMgr \
  UIControl UIControl2 window/UIEmotionListWnd window/UIEmotionWnd window/UIFrameWnd UIFrameWnd2 \
  UIFrameWnd3 UIGronMessengerWnd UIGuildWnd window/UIIllustWnd window/UIImeWnd UIItemWnd \
  UIMessengerWnd window/UIMetalWorkWnd UIRectInfo window/UISys UIWindowMgr 3dActor 3dGround \
  game/Common ExceptionHandler framework/ftwbrk GameWorld framework/Language framework/Locale \
  mmgr Mode PathFinder Picker View GuildInfo Insult ItemInfo MsgStrings SkillInfo TipOfTheDay \
  movie/CBink base/CDec base/File base/FindHack GenUserCode base/GPak base/hash \
  base/Res SimpleAssert base/Util base/xml CSRCryptoR2 CSRCryptoR2Client \
  CSRCryptoR2PacketConfiguration CSRPacketR2 CSRPacketR2Client CSRPacketR2Var Cipher CipherImpl \
  CReAssemblyPacket lapi lauxlib lbaselib lcode ldblib ldebug ldo ldump lfunc lgc liolib llex \
  lmathlib lmem loadlib lobject lopcodes lparser lstate lstring lstrlib ltable ltablib ltests \
  ltm lundump lvm lzio cKeyProtector cSecureGamePack LanguageKeyProcess telexInputMode \
  VietnamLanguage VniInputMode GameActor GameActor3d GameActorMsgHandler GameMode GameMode2 \
  GameModePacket GronModePacket Item \
  LoginMode MercenaryAI Npc Pc Player RagEffect RagEffect2 RagEffectPrim Session Session2 Skill \
  WinMain

# The libraries of the link command line, in order.
LIBS := \
  dinput.lib binkw32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib \
  ddraw.lib dsound.lib dxguid.lib ijl15.lib ws2_32.lib mss32.lib imm32.lib cps.lib \
  npgamelibpro.lib granny2.lib

# The members of libcpmt.lib, in the order they are given to LIB.
LIBCPMT := \
  library/msvc6/xwctomb library/msvc6/_toupper library/msvc6/_tolower library/msvc6/xstrcoll \
  library/msvc6/xstod library/msvc6/xmbtowc library/msvc6/xlock library/msvc6/xlocale \
  stock/wlocale library/msvc6/strstrea library/msvc6/string library/msvc6/nomemory \
  library/msvc6/newop2 library/msvc6/locale0 library/msvc6/locale library/msvc6/ios \
  library/msvc6/delop

# The members of libcmt.lib, in the order they are given to LIB.
LIBCMT := \
  library/msvc6/util library/msvc6/matherr library/msvc6/ldexp library/msvc6/ieeemisc \
  library/msvc6/frexp library/msvc6/fpexcept library/msvc6/assembly/pow \
  library/msvc6/assembly/acos library/msvc6/assembly/asin library/msvc6/testfdiv \
  library/msvc6/powhlp library/msvc6/ieee87 library/msvc6/assembly/huge \
  library/msvc6/assembly/genexcep library/msvc6/assembly/ftol library/msvc6/frnd \
  library/msvc6/fpctrl library/msvc6/floor library/msvc6/assembly/common library/msvc6/ceil \
  library/msvc6/assembly/adj_fdiv library/msvc6/assembly/87tran library/msvc6/assembly/87fmod \
  library/msvc6/87except library/msvc6/assembly/87disp library/msvc6/assembly/87cdisp \
  library/msvc6/x10fout library/msvc6/tenpow library/msvc6/strgtold library/msvc6/mantold \
  library/msvc6/intrncvt library/msvc6/fpinit library/msvc6/cvt library/msvc6/constpow \
  library/msvc6/cfout library/msvc6/cfin library/CrtTimezone library/msvc6/timeset \
  library/msvc6/time library/CrtStrftime library/msvc6/mktime library/msvc6/localtim \
  library/msvc6/gmtime library/msvc6/dtoxtime library/msvc6/difftime library/msvc6/days \
  library/msvc6/ctime library/msvc6/clock library/msvc6/asctime library/msvc6/wcsstr \
  library/msvc6/wcslwr library/msvc6/wcslen library/msvc6/wcsdup library/msvc6/wcscat \
  library/msvc6/strtok library/msvc6/strlwr library/msvc6/strcoll library/msvc6/assembly/strstr \
  library/msvc6/assembly/strrchr library/msvc6/assembly/strpbrk library/msvc6/assembly/strnicmp \
  library/msvc6/assembly/strncpy library/msvc6/assembly/strncmp library/msvc6/assembly/strncat \
  library/msvc6/assembly/strlen library/msvc6/assembly/stricmp library/msvc6/strdup \
  library/msvc6/assembly/strcspn library/msvc6/assembly/strcmp library/msvc6/assembly/strchr \
  library/msvc6/assembly/strcat library/msvc6/assembly/memset library/msvc6/assembly/memmove \
  library/msvc6/assembly/memcmp library/msvc6/assembly/memchr library/msvc6/assembly/memcpy \
  library/msvc6/ncommode library/msvc6/vsprintf library/msvc6/ungetc library/msvc6/tmpfile \
  library/msvc6/stream library/msvc6/sscanf library/msvc6/sprintf library/msvc6/snprintf \
  library/msvc6/rmtmp library/msvc6/puts library/msvc6/output library/msvc6/input \
  library/msvc6/fwrite library/msvc6/ftell library/msvc6/fseek library/msvc6/fscanf \
  library/msvc6/fread library/msvc6/fprintf library/msvc6/fopen library/msvc6/fflush \
  library/msvc6/fclose library/msvc6/closeall library/msvc6/_sftbuf library/msvc6/_open \
  library/msvc6/_getbuf library/msvc6/_freebuf library/msvc6/_flsbuf library/msvc6/_file \
  library/msvc6/_filbuf library/msvc6/fputs library/msvc6/fgets library/msvc6/fgetc \
  library/msvc6/fp8 library/msvc6/wincrt0 library/msvc6/wincmdln library/msvc6/tidtable \
  library/msvc6/threadex library/msvc6/stdenvp library/msvc6/stdargv library/msvc6/mlock \
  library/msvc6/crt0msg library/msvc6/crt0init library/msvc6/crt0fp library/msvc6/crt0dat \
  library/msvc6/assembly/chkstk library/msvc6/wtombenv library/msvc6/winxfltr \
  library/msvc6/winsig library/msvc6/w_str library/msvc6/w_map library/msvc6/w_loc \
  library/msvc6/syserr library/msvc6/strerror library/msvc6/setlocal library/msvc6/setenv \
  library/msvc6/rand library/msvc6/purevirt library/msvc6/nlsdata3 library/msvc6/nlsdata2 \
  library/msvc6/nlsdata1 library/msvc6/onexit library/msvc6/lconv library/msvc6/inittime \
  library/msvc6/initnum library/msvc6/initmon library/msvc6/inithelp library/msvc6/initctyp \
  library/msvc6/initcoll library/msvc6/getqloc library/msvc6/getpath library/msvc6/getenv \
  library/msvc6/ctype library/msvc6/crtmbox library/msvc6/cmiscdat library/msvc6/abort \
  library/msvc6/a_str library/msvc6/a_map library/msvc6/a_loc library/msvc6/a_env \
  library/msvc6/a_cmp library/msvc6/assembly/setjmp3 library/msvc6/sehsupp \
  library/msvc6/assembly/longjmp library/msvc6/assembly/exsup3 library/msvc6/assembly/exsup \
  library/msvc6/mbsstr library/msvc6/mbsrchr library/msvc6/mbsnbico library/msvc6/mbsnbicm \
  library/msvc6/mbsnbcpy library/msvc6/mbsnbcat library/msvc6/mbsinc library/msvc6/mbsicmp \
  library/msvc6/mbschr library/msvc6/mbsbtype library/msvc6/mbctype library/msvc6/mbbtype \
  library/msvc6/ismbbyte library/msvc6/write library/msvc6/txtmode library/msvc6/setmode \
  library/msvc6/read library/msvc6/osfinfo library/msvc6/open library/msvc6/lseek \
  library/msvc6/isatty library/msvc6/ioinit library/msvc6/commit library/msvc6/close \
  library/msvc6/chsize library/msvc6/assembly/ullrem library/msvc6/assembly/ulldiv \
  library/msvc6/assembly/llshl library/msvc6/assembly/llmul library/CrtNew \
  library/msvc6/_newmode library/msvc6/handler library/CrtSmallHeap library/CrtRealloc \
  library/CrtMsize library/CrtMalloc library/CrtHeapInit library/CrtFree library/CrtCalloc \
  library/msvc6/system library/msvc6/spawnvpe library/msvc6/spawnve library/msvc6/spawnl \
  library/msvc6/dospawn library/msvc6/cenvarg library/msvc6/typinfo library/msvc6/stdexcpt \
  library/msvc6/ehvecdtr library/msvc6/ehvecctr library/msvc6/validate library/msvc6/unhandld \
  library/msvc6/throw library/msvc6/hooks library/msvc6/frame library/msvc6/assembly/ehprolog \
  library/msvc6/assembly/lowhelpr library/msvc6/trnsctrl library/msvc6/unlink \
  library/msvc6/rmdir library/msvc6/rename library/msvc6/mkdir library/msvc6/getpid \
  library/msvc6/getcwd library/msvc6/dosmap library/msvc6/chmod library/msvc6/access \
  library/msvc6/xtoa library/msvc6/wctomb library/msvc6/toupper library/msvc6/tolower \
  library/msvc6/strtol library/msvc6/strtod library/msvc6/mbtowc library/msvc6/isctype \
  library/msvc6/atox library/msvc6/atof library/msvc6/_fptostr library/msvc6/_ctype

# The members of npgamelibpro.lib, in the order they are given to LIB.
NPGAMELIBPRO := \
  NPGameLib SafeAPI FileAuthClient CommPipe rc4 Common MemIni PacketProtect Crc32

# Members whose name in the library differs from the source name.
MEMBER.library/CrtTimezone := tzset
MEMBER.library/CrtStrftime := strftime
MEMBER.library/CrtNew := new
MEMBER.library/CrtSmallHeap := sbheap
MEMBER.library/CrtRealloc := realloc
MEMBER.library/CrtMsize := msize
MEMBER.library/CrtMalloc := malloc
MEMBER.library/CrtHeapInit := heapinit
MEMBER.library/CrtFree := free
MEMBER.library/CrtCalloc := calloc
MEMBER.library/DxGuid := dxguid

# Compiler flags. Every C and C++ source gets $(BASE), its EXTRA flags and its include paths.
BASE := /nologo /c /Og /Oi /Ob2 /Gf /Gy /GR- /GX /MT /W3 /DWIN32 /D_WINDOWS /DNDEBUG /D_MBCS \
  /DWEDDING_PATCH /DMOB_SKILL_PATCH /D_UPDATE2005A /D_UPDATE2005C /D_UPDATE2004B /DBALANCE_PATCH \
  /DMOB_SKILL /DSECONDJOB_SKILL2 /D_HOMUN /D_MER /D_MER_R2 /D_PACKET_VER=7 /DKILLER_SYSTEM_V2 \
  /D_HUNTING_TIME_LIST /D__PARTY_INVITE /DBIND_ON_EQUIP
CRT := /D_CRTBLD /D_MB_MAP_DIRECT /DWINHEAP /DWIN32_LEAN_AND_MEAN /DNOSERVICE /Ivendor/msvc6/crt
NT5 := /Ivendor/msvc-internals/nt5/fpw32/include /Ivendor/msvc-internals/nt5/crtw32/eh \
  /Ivendor/msvc-internals/nt5/crtw32/h /Ivendor/msvc-internals/nt5/langapi/include
OPENNT := /Ivendor/msvc-internals/opennt/fpw32/include /Ivendor/msvc-internals/opennt/crtw32/eh \
  /Ivendor/msvc-internals/opennt/crtw32/h /Ivendor/msvc-internals/opennt/langapi/include
ASFLAGS := /nologo /coff /Cx /Zm /DQUIET /D?QUIET /Di386 /D_WIN32 /DWIN32 /D_MT /Ivendor/msvc6/crt \
  /Ivendor/msvc-internals/nt5/fpw32/inc/i386 /Ivendor/msvc-internals/nt5/fpw32/tran/i386 \
  /Ivendor/msvc-internals/nt5/crtw32/h /Ivendor

# The game's sources find their own headers first, the libraries' the SDK's.
INCLUDES := /Isrc /Itools/dx8sdk/Include /Itools/msvc6/Include
$(OBJ)/library/%: INCLUDES := /Itools/dx8sdk/Include /Itools/msvc6/Include /Isrc

# The runtime library's sources build with its makefiles' settings unless a line below says otherwise.
$(OBJ)/library/%: EXTRA := $(CRT) /O1 /GF /Oi- /Ob1

# Sources that another compiler than the default built.
$(call objects,\
  library/msvc6/_tolower library/msvc6/_toupper library/msvc6/delop library/msvc6/ios \
  library/msvc6/locale library/msvc6/locale0 library/msvc6/newop2 library/msvc6/nomemory \
  library/msvc6/string library/msvc6/strstrea library/msvc6/xlocale \
  library/msvc6/xlock library/msvc6/xmbtowc library/msvc6/xstod library/msvc6/xstrcoll \
  library/msvc6/xwctomb): COMPILER := $(CL_8047)
$(call objects,library/mss/cleanup): COMPILER := $(CL_8943)
$(call objects,library/DxGuid library/dinput/dilib1): COMPILER := $(CL_DDK)

$(call objects,library/DxGuid library/dinput/dilib1): EXTRA := /Zl /Zvc6
$(call objects,library/msvc6/assembly/pow): ASEXTRA := /D_NTSUBSET_=1
$(call objects,\
  library/msvc6/_newmode library/msvc6/cmiscdat library/msvc6/crt0init library/msvc6/ctype \
  library/msvc6/days library/msvc6/ncommode library/msvc6/nlsdata1 library/msvc6/nlsdata2 \
  library/msvc6/nlsdata3 library/msvc6/timeset library/msvc6/txtmode): EXTRA := $(CRT)
$(call objects,\
  library/msvc6/87except library/msvc6/fpctrl library/msvc6/frexp library/msvc6/frnd \
  library/msvc6/ieee87 library/msvc6/ieeemisc library/msvc6/ldexp library/msvc6/matherr \
  library/msvc6/powhlp): EXTRA := $(CRT) $(NT5) /O1 /GF /Oi- /Op
$(call objects,\
  CommPipe Common Crc32 FileAuthClient MemIni NPGameLib PacketProtect SafeAPI): EXTRA := /Oy /Ob1
$(call objects,\
  library/msvc6/cfout library/msvc6/constpow library/msvc6/strgtold library/msvc6/tenpow \
  library/msvc6/x10fout): EXTRA := $(CRT) $(NT5) /O1 /GF /Oi-
$(call objects,\
  library/msvc6/ehvecctr library/msvc6/throw library/msvc6/typinfo library/msvc6/unhandld \
  library/msvc6/validate): EXTRA := $(CRT) $(NT5) /O1 /GF /Oi- /d1Binl /GR /Ivendor/msvc-internals/nt5/langapi/undname
$(call objects,\
  library/msvc6/ceil library/msvc6/floor library/msvc6/fpexcept): EXTRA := $(CRT) $(OPENNT) /O1 /GF /Oi- /Op
$(call objects,library/CrtNew library/msvc6/handler): EXTRA := $(CRT) /O1 /GF /Oi- /Ob1 /GR /d1Binl
$(call objects,library/msvc6/cvt library/msvc6/intrncvt): EXTRA := $(CRT) $(OPENNT) /O1 /GF /Oi- /Ob1
$(call objects,library/msvc6/fpinit library/msvc6/mantold): EXTRA := $(CRT) $(NT5) /O1 /GF /Oi- /Ob1
$(call objects,library/msvc6/frame library/msvc6/trnsctrl): EXTRA := $(CRT) /Ivendor/msvc-internals/opennt/crtw32/eh /Itools/msvc6/Include /Ivendor/msvc-internals/opennt/crtw32/h /Ivendor/msvc-internals/opennt/langapi/include /O1 /GF /Oi- /d1Binl /GR /Ob1
$(call objects,library/msvc6/testfdiv library/msvc6/util): EXTRA := $(CRT) $(NT5) /O1 /GF /Oi- /Op /Ob1
$(call objects,game/Common): EXTRA := /Yccommon.h
$(call objects,library/msvc6/cfin): EXTRA := $(CRT) $(OPENNT) /O1 /GF /Oi-
$(call objects,library/msvc6/ehvecdtr): EXTRA := $(CRT) $(NT5) /O1 /GF /Oi- /d1Binl /GR /Ivendor/msvc-internals/nt5/langapi/undname /Ob1
$(call objects,library/msvc6/hooks): EXTRA := $(CRT) /Ivendor/msvc-internals/opennt/fpw32/include /Ivendor/msvc-internals/opennt/crtw32/eh /Itools/msvc6/Include /Ivendor/msvc-internals/opennt/crtw32/h /Ivendor/msvc-internals/opennt/langapi/include /O1 /GF /Oi- /d1Binl /GR /Ivendor/msvc-internals/nt5/langapi/undname /Ob1
$(call objects,library/msvc6/stdexcpt): EXTRA := $(CRT) $(OPENNT) /O1 /GF /Oi- /d1Binl /GR /Ivendor/msvc-internals/nt5/langapi/undname /Ob1
$(call objects,rc4): EXTRA := /Oy
$(call objects,\
  library/msvc6/ios library/msvc6/locale library/msvc6/locale0 library/msvc6/newop2 \
  library/msvc6/nomemory library/msvc6/string library/msvc6/strstrea \
  library/msvc6/xlocale library/msvc6/xlock): EXTRA := $(CRT) /O1 /GF /Oi- /Ob1 /GR /d1Binl /Zl
$(call objects,\
  library/msvc6/_tolower library/msvc6/_toupper library/msvc6/xmbtowc library/msvc6/xstod \
  library/msvc6/xstrcoll library/msvc6/xwctomb): EXTRA := $(CRT) /O1 /GF /Oi- /Ob1 /Zl
$(call objects,library/msvc6/delop): EXTRA := $(CRT) /O1 /GF /Oi- /Ob1 /GR /d1Binl /Zl /Ivendor
$(call objects,library/mss/cleanup): EXTRA := /Zl /G6
