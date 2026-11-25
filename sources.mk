# The game's translation units, in the order of the 2008 client's link
# command line; objects are named by their source path under src/, without
# the extension. The port's build (CMakeLists.txt) and the census
# (port/tools/census.sh) compile them.
#
# On main this file also lists what only the Windows build used: the
# libraries of the link, the members of libcpmt.lib, libcmt.lib and
# npgamelibpro.lib, the Visual C++ flags of every source. M11.11 removed
# those lists, the Makefile, tools/ and vendor/ from macos-port, together
# with the units the port never built (each with its reason, as
# BF_CLIENT_EXCLUDE in CMakeLists.txt gave it): device/EncClient (keyboard
# encryption, npkcrypt, and its self-patching installer), cKeyProtector (the
# hook probe) and cSecureGamePack (GameGuard's packet protection), all
# protection, removed (technical plan 3.3, "Protection"); LanguageKeyProcess,
# telexInputMode, VietnamLanguage and VniInputMode (the Vietnamese service's
# key processor and input modes, not needed by this client, M2.5);
# ExceptionHandler (the crash dialog and minidump writer, replaced by
# port/client/src/bf_crash.cpp, which defines ExceptionFilter).

GAME := \
  3dlib/Anim 3dlib/D3dutil 3dlib/DC 3dlib/Device 3dlib/DrawUtil 3dlib/mesh 3dlib/Prim \
  3dlib/Renderer 3dlib/Texture device/Connection device/Control device/GronPacket device/md5 \
  packet device/PacketQueue Padding device/SequenceRandomGenerator device/Sound device/Timer \
  VariablePacket resource/Action resource/Attr resource/Bitmap resource/Emblem \
  resource/EZeffect resource/Ground resource/ImfRes resource/Model resource/PaletteRes \
  resource/Skybox resource/Sprite resource/Wave resource/World skin SnapMgr UIControl \
  UIControl2 window/UIEmotionListWnd window/UIEmotionWnd window/UIFrameWnd UIFrameWnd2 \
  UIFrameWnd3 UIGronMessengerWnd UIGuildWnd window/UIIllustWnd window/UIImeWnd UIItemWnd \
  UIMessengerWnd window/UIMetalWorkWnd UIRectInfo window/UISys UIWindowMgr 3dActor 3dGround \
  game/Common framework/ftwbrk GameWorld framework/Language framework/Locale mmgr Mode \
  PathFinder Picker View GuildInfo Insult ItemInfo MsgStrings SkillInfo TipOfTheDay movie/CBink \
  base/CDec base/File base/FindHack GenUserCode base/GPak base/hash base/Res SimpleAssert \
  base/Util base/xml CSRCryptoR2 CSRCryptoR2Client CSRCryptoR2PacketConfiguration CSRPacketR2 \
  CSRPacketR2Client CSRPacketR2Var Cipher CipherImpl CReAssemblyPacket lapi lauxlib lbaselib \
  lcode ldblib ldebug ldo ldump lfunc lgc liolib llex lmathlib lmem loadlib lobject lopcodes \
  lparser lstate lstring lstrlib ltable ltablib ltests ltm lundump lvm lzio GameActor \
  GameActor3d GameActorMsgHandler GameMode GameMode2 GameModePacket GronModePacket Item \
  LoginMode MercenaryAI Npc Pc Player RagEffect RagEffect2 RagEffectPrim Session Session2 Skill \
  WinMain
