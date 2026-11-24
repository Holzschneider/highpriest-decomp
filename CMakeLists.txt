# The port's build (implementation plan M2.7): the client, the native modules,
# the probes, and the test programs, for arm64 macOS 14 with Ninja.
#
#   cmake --preset debug && cmake --build --preset debug     (also asan, release)
#   cmake --build --preset debug --target check              all module tests
#   port/tools/fastcheck.sh                                  the integration check
#
# Two files outside this one decide what is built how, and are read at
# configure time (editing either re-runs the configure step):
#   sources.mk            the game's units: the GAME list, minus the exclusions
#                         below
#   port/tools/flags.sh   the client's compile flags, shared with census.sh,
#                         probe/build.sh and layoutcheck.py
cmake_minimum_required(VERSION 3.24)

set(BF_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
include(${BF_ROOT}/port/name.cmake)
string(TOLOWER "${BF_APP_NAME}" BF_EXE)

# ---------------------------------------------------------------- flags.sh
# Every word of flags.sh, one per line, tagged with the variable it came from.
# BF_WIN_HEADERS (cache or environment) names another Windows header directory.
set(BF_WIN_HEADERS "$ENV{BF_WIN_HEADERS}" CACHE PATH
	"Directory with windows.h, d3d.h and the rest (empty: port/compat/winheaders)")
set(BF_FLAGS_SH ${BF_ROOT}/port/tools/flags.sh)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
	${BF_FLAGS_SH} ${BF_ROOT}/sources.mk ${BF_ROOT}/port/name.cmake)
execute_process(
	COMMAND env ROOT=${BF_ROOT} BF_WIN_HEADERS=${BF_WIN_HEADERS} /bin/bash -c
		". \"$ROOT/port/tools/flags.sh\" && printf 'H %s\\n' \"$WIN_HEADERS\" && printf 'T %s\\n' $BF_TARGET && printf 'D %s\\n' $BF_DEFINES && printf 'I %s\\n' $BF_INCLUDES"
	OUTPUT_VARIABLE bf_flag_lines RESULT_VARIABLE bf_flag_result)
if(NOT bf_flag_result EQUAL 0)
	message(FATAL_ERROR "port/tools/flags.sh failed (${bf_flag_result})")
endif()
string(REPLACE ";" "\\;" bf_flag_lines "${bf_flag_lines}")
string(REPLACE "\n" ";" bf_flag_lines "${bf_flag_lines}")
set(BF_CLIENT_DEFINES)          # without -D
set(BF_CLIENT_OPTIONS)          # language options from BF_TARGET
set(BF_CLIENT_INCLUDES)         # -I paths, in order
set(BF_CLIENT_PREFIX)           # the force-included header
set(bf_next)
foreach(line IN LISTS bf_flag_lines)
	if(line STREQUAL "")
		continue()
	endif()
	string(SUBSTRING "${line}" 0 1 tag)
	string(SUBSTRING "${line}" 2 -1 word)
	if(bf_next STREQUAL "target")
		set(BF_TARGET_TRIPLE "${word}")
		set(bf_next)
	elseif(bf_next STREQUAL "prefix")
		set(BF_CLIENT_PREFIX "${word}")
		set(bf_next)
	elseif(tag STREQUAL "H")
		set(BF_CLIENT_WIN_HEADERS "${word}")
	elseif(tag STREQUAL "T" AND word STREQUAL "-target")
		set(bf_next target)
	elseif(tag STREQUAL "T")
		list(APPEND BF_CLIENT_OPTIONS "${word}")
	elseif(tag STREQUAL "D" AND word MATCHES "^-D(.+)$")
		list(APPEND BF_CLIENT_DEFINES "${CMAKE_MATCH_1}")
	elseif(tag STREQUAL "I" AND word STREQUAL "-include")
		set(bf_next prefix)
	elseif(tag STREQUAL "I" AND word MATCHES "^-I(.+)$")
		list(APPEND BF_CLIENT_INCLUDES "${CMAKE_MATCH_1}")
	else()
		message(FATAL_ERROR "flags.sh: no rule for '${word}' (${tag}); teach CMakeLists.txt")
	endif()
endforeach()
if(NOT BF_TARGET_TRIPLE MATCHES "^arm64-apple-macos([0-9.]+)$")
	message(FATAL_ERROR "flags.sh: target '${BF_TARGET_TRIPLE}' is not arm64-apple-macosNN")
endif()
set(bf_macos ${CMAKE_MATCH_1})
if(NOT bf_macos MATCHES "\\.")
	set(bf_macos ${bf_macos}.0)
endif()
if(NOT EXISTS ${BF_CLIENT_WIN_HEADERS}/windows.h)
	message(FATAL_ERROR "no windows.h in ${BF_CLIENT_WIN_HEADERS}; set BF_WIN_HEADERS")
endif()

# arm64 only, the deployment target of flags.sh (technical plan 3.1, 6).
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "arm64 only" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET ${bf_macos} CACHE STRING "from port/tools/flags.sh" FORCE)

project(${BF_APP_NAME} LANGUAGES C CXX OBJC OBJCXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	message(FATAL_ERROR "the port builds on macOS with Apple clang (technical plan 3.1, 10)")
endif()

# ---------------------------------------------------------------- build modes
# The asan preset turns both on. Pattern-filled locals: release code built by
# Visual C++ 6.0 tends to read uninitialised memory that happened to be zero
# (technical plan 3.6). Directory-wide, so the modules and tests get them too.
option(BF_SANITIZE "Address and undefined-behaviour sanitizers, everywhere" OFF)
option(BF_PATTERN_INIT "Fill automatic variables with a pattern (-ftrivial-auto-var-init=pattern)" OFF)
if(BF_SANITIZE)
	add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined)
	add_link_options(-fsanitize=address,undefined)
endif()
if(BF_PATTERN_INIT)
	add_compile_options(-ftrivial-auto-var-init=pattern)
endif()

enable_testing()

# ---------------------------------------------------------------- instrument
# The trace and the packet-name table, linked once into every program that
# uses the client's sources. Port code: C++17. The table is generated from
# src/packet.h at build time.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(BF_PACKET_NAMES ${CMAKE_CURRENT_BINARY_DIR}/generated/bf_packet_names.cpp)
add_custom_command(OUTPUT ${BF_PACKET_NAMES}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/generated
	COMMAND Python3::Interpreter ${BF_ROOT}/port/tools/gen_packet_names.py ${BF_ROOT}/src/packet.h > ${BF_PACKET_NAMES}
	DEPENDS ${BF_ROOT}/port/tools/gen_packet_names.py ${BF_ROOT}/src/packet.h
	COMMENT "Generating the packet-name table from src/packet.h" VERBATIM)
add_library(bf_instrument STATIC ${BF_ROOT}/port/instrument/bf_trace.cpp ${BF_PACKET_NAMES})
target_include_directories(bf_instrument PUBLIC ${BF_ROOT}/port/instrument)
target_compile_definitions(bf_instrument PRIVATE BF_APP_NAME="${BF_APP_NAME}")
set_target_properties(bf_instrument PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------- native modules
# Each builds standalone and under add_subdirectory; each sets C++17 and ARC on
# its own targets. A module whose CMakeLists.txt has not landed yet is skipped.
set(BF_NATIVE_MODULES sys codec app gfx text audio prefs view control strings formats/bink formats/granny)
set(BF_NATIVE_LIBS)
set(BF_NATIVE_TESTS)
add_custom_target(native)
foreach(module IN LISTS BF_NATIVE_MODULES)
	if(NOT EXISTS ${BF_ROOT}/port/native/${module}/CMakeLists.txt)
		message(STATUS "native module ${module}: not in this tree, skipped")
		continue()
	endif()
	get_filename_component(leaf ${module} NAME)
	add_subdirectory(port/native/${module} native/${leaf})
	if(TARGET bf_${leaf})
		list(APPEND BF_NATIVE_LIBS bf_${leaf})
		add_dependencies(native bf_${leaf})
	endif()
	set(test ${BF_EXE}-${leaf}-test)
	if(NOT TARGET ${test})
		message(WARNING "native module ${module} has no test program ${test}")
		continue()
	endif()
	list(APPEND BF_NATIVE_TESTS ${test})
	add_dependencies(native ${test})
	# Labels: headless tests need no window and no sound output; fastcheck
	# runs those. app opens a window, audio plays sound.
	if(leaf STREQUAL "app" OR leaf STREQUAL "prefs" OR leaf STREQUAL "view")
		set(label window)
	elseif(leaf STREQUAL "audio")
		set(label sound)
	else()
		set(label headless)
	endif()
	get_target_property(test_dir ${test} BINARY_DIR)
	add_test(NAME ${leaf} COMMAND ${test} WORKING_DIRECTORY ${test_dir})
	set_tests_properties(${leaf} PROPERTIES LABELS "${label}" TIMEOUT 300
		ENVIRONMENT "BF_NO_ALERTS=1;BF_TEST_OUT=${test_dir}")
endforeach()

# `check` runs every module test (a window opens, sound plays);
# `check-headless` only those that need neither.
add_custom_target(check
	COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
	DEPENDS ${BF_NATIVE_TESTS} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} USES_TERMINAL)
add_custom_target(check-headless
	COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L headless
	DEPENDS ${BF_NATIVE_TESTS} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} USES_TERMINAL)

# ---------------------------------------------------------------- client flags
# bf_client_flags(<target> <tree>): the flags of flags.sh for game sources
# under <tree>, in census.sh's order: the prefix header, the port's -I paths,
# the tree, then the Windows headers with -isystem. C++ is gnu++14 (the code
# uses pieces removed in C++17), C (Lua) is gnu89. -ferror-limit=0 lets the
# build report every error, as the census does.
function(bf_client_flags target tree)
	target_compile_definitions(${target} PRIVATE ${BF_CLIENT_DEFINES})
	target_compile_options(${target} PRIVATE ${BF_CLIENT_OPTIONS} -ferror-limit=0
		"SHELL:-include ${BF_CLIENT_PREFIX}")
	target_include_directories(${target} BEFORE PRIVATE ${BF_CLIENT_INCLUDES} ${tree})
	target_include_directories(${target} SYSTEM PRIVATE ${BF_CLIENT_WIN_HEADERS})
	set_target_properties(${target} PROPERTIES
		CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS ON
		C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS ON)
endfunction()

# ---------------------------------------------------------------- adapters
# port/compat/<area>: the Win32-shaped adapters of stage A, which M11 deletes.
# Each area's CMakeLists.txt defines a static library bf_compat_<area> and a
# test program bifrost-compat-<area>-test, and calls bf_compat_flags on its
# targets so that it compiles against exactly the declarations the client
# sees (the prefix header, the port's includes, the Windows headers), but as
# C++17 and without the game's tree. Areas own disjoint sets of Win32 names:
#   win32  kernel32, advapi32, winmm, shell32, MessageBox (on port/native/sys)
#   d3d    DirectDraw 7, Direct3D 7 (on port/native/gfx)
#   gdi    gdi32 (on port/native/text)
#   miles  mss32 (on port/native/audio)
#   input  user32 windows, messages, keyboard, clipboard; DirectInput; imm32
#          (on port/native/app)
function(bf_compat_flags target)
	target_compile_definitions(${target} PRIVATE ${BF_CLIENT_DEFINES})
	target_compile_options(${target} PRIVATE ${BF_CLIENT_OPTIONS}
		"SHELL:-include ${BF_CLIENT_PREFIX}")
	target_include_directories(${target} BEFORE PRIVATE ${BF_CLIENT_INCLUDES})
	target_include_directories(${target} SYSTEM PRIVATE ${BF_CLIENT_WIN_HEADERS})
	set_target_properties(${target} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
endfunction()
set(BF_COMPAT_AREAS win32 d3d gdi miles input)
set(BF_COMPAT_LIBS)
add_custom_target(compat)
foreach(area IN LISTS BF_COMPAT_AREAS)
	if(NOT EXISTS ${BF_ROOT}/port/compat/${area}/CMakeLists.txt)
		message(STATUS "adapter ${area}: not in this tree, skipped")
		continue()
	endif()
	add_subdirectory(port/compat/${area} compat/${area})
	list(APPEND BF_COMPAT_LIBS bf_compat_${area})
	add_dependencies(compat bf_compat_${area})
	set(test ${BF_EXE}-compat-${area}-test)
	if(TARGET ${test})
		add_dependencies(compat ${test})
		get_target_property(test_dir ${test} BINARY_DIR)
		add_test(NAME compat-${area} COMMAND ${test} WORKING_DIRECTORY ${test_dir})
		set(label headless)
		if(area STREQUAL "input")
			set(label window)
		elseif(area STREQUAL "miles")
			set(label sound)
		endif()
		set_tests_properties(compat-${area} PROPERTIES LABELS "${label}" TIMEOUT 300
			ENVIRONMENT "BF_NO_ALERTS=1;BF_TEST_OUT=${test_dir}")
	endif()
endforeach()

add_dependencies(check compat)
add_dependencies(check-headless compat)

# ---------------------------------------------------------------- the client
# The units of sources.mk's GAME list, in link order, minus the exclusions.
file(STRINGS ${BF_ROOT}/sources.mk bf_mk)
set(BF_GAME_UNITS)
set(bf_in_game OFF)
foreach(line IN LISTS bf_mk)
	if(line MATCHES "^GAME :=")
		set(bf_in_game ON)
		string(REGEX REPLACE "^GAME :=" "" line "${line}")
	elseif(NOT bf_in_game)
		continue()
	endif()
	set(bf_more OFF)
	if(line MATCHES "\\\\$")
		set(bf_more ON)
		string(REGEX REPLACE "\\\\$" "" line "${line}")
	endif()
	string(STRIP "${line}" line)
	if(line)
		string(REGEX REPLACE "[ \t]+" ";" words "${line}")
		list(APPEND BF_GAME_UNITS ${words})
	endif()
	if(NOT bf_more)
		break()
	endif()
endforeach()
list(LENGTH BF_GAME_UNITS bf_count)
if(bf_count EQUAL 0)
	message(FATAL_ERROR "sources.mk: no GAME list")
endif()

# Units taken out of the build (implementation plan M2.5, technical plan 3.3),
# each with its reason. Nothing else in the GAME list names what they define,
# except where noted. GameGuard's 9 files (NPGameLib, SafeAPI, FileAuthClient,
# CommPipe, rc4, Common, MemIni, PacketProtect, Crc32) are not here because
# they are not game units: sources.mk builds them into npgamelibpro.lib, which
# the port never compiles. Its four entry points (SetHwnd, SendID,
# ExitFindHack, Auth2 in base/FindHack.cpp) are empty under BF_PORT.
# Replaced behind their interfaces under BF_PORT, and so still built:
# movie/CBink (reports no movie; the Bink SDK headers are not included) and
# the Granny resource classes in resource/Model.cpp (load nothing; CGameMode
# creates no Granny actor).
set(BF_CLIENT_EXCLUDE
	# Keyboard encryption (npkcrypt) and the self-patching code that installs
	# it: protection, removed (3.3, "Protection").
	device/EncClient
	# The hook probe: protection, removed (3.3, "Protection").
	cKeyProtector
	# GameGuard's packet protection wrapper: protection, removed (3.3).
	cSecureGamePack
	# The Vietnamese input classes: the Vietnamese service's key processor and
	# its Telex and VNI input modes, not needed by this client (M2.5).
	LanguageKeyProcess telexInputMode VietnamLanguage VniInputMode
	# The crash dialog and minidump writer: replaced by the signal handler in
	# port/compat/stubs/bf_crash.cpp, which writes a backtrace through the
	# trace and defines ExceptionFilter, the one name WinMain uses (3.3,
	# "Protection").
	ExceptionHandler
)
foreach(unit IN LISTS BF_CLIENT_EXCLUDE)
	if(NOT unit IN_LIST BF_GAME_UNITS)
		message(WARNING "exclusion ${unit} is no longer in sources.mk; drop it here")
	endif()
endforeach()

set(BF_CLIENT_SOURCES)
foreach(unit IN LISTS BF_GAME_UNITS)
	if(unit IN_LIST BF_CLIENT_EXCLUDE)
		continue()
	elseif(EXISTS ${BF_ROOT}/src/${unit}.cpp)
		list(APPEND BF_CLIENT_SOURCES src/${unit}.cpp)
	elseif(EXISTS ${BF_ROOT}/src/${unit}.c)
		list(APPEND BF_CLIENT_SOURCES src/${unit}.c)
	else()
		message(FATAL_ERROR "sources.mk names ${unit}, but src/ has neither ${unit}.cpp nor ${unit}.c")
	endif()
endforeach()
list(LENGTH BF_CLIENT_SOURCES bf_count)
message(STATUS "client: ${bf_count} units from sources.mk")
# The list for port/tools/fastcheck.sh, which compares the client's compile
# errors with the census's for the same units.
string(REPLACE ";" "\n" bf_units "${BF_CLIENT_SOURCES}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/client-units.txt "${bf_units}\n")

# The executable is the application name in lower case. The .app bundle is M12.
# Port code compiled into the client with the client's flags: what replaces
# an excluded unit (port/compat/stubs/README.md).
set(BF_CLIENT_PORT_SOURCES
	port/compat/stubs/bf_crash.cpp)
add_executable(${BF_EXE} ${BF_CLIENT_SOURCES} ${BF_CLIENT_PORT_SOURCES})
bf_client_flags(${BF_EXE} ${BF_ROOT}/src)
# Apple frameworks and zlib only (technical plan 3.1, 9): the native modules
# bring their frameworks; nothing of Wine is linked. The modules are linked
# only (LINK_ONLY): their usage requirements (C++17, their include paths)
# must not reach the game's units, which stay gnu++14 with the census's paths.
set(bf_link)
foreach(lib IN LISTS BF_COMPAT_LIBS BF_NATIVE_LIBS)
	list(APPEND bf_link $<LINK_ONLY:${lib}>)
endforeach()
target_link_libraries(${BF_EXE} PRIVATE $<LINK_ONLY:bf_instrument> ${bf_link} z)

# ---------------------------------------------------------------- probes
# bifrost-packetlen and bifrost-netprobe compile the game's packet.cpp,
# PacketQueue.cpp and Connection.cpp. BF_PROBE_SRC names the tree they come
# from: src/ since M2.2 rewrote `long` and `time_t` there (relative paths are
# taken from the repository root; -DBF_PROBE_SRC=build/port/shadow/src builds
# from the copy that port/tools/shadow.sh makes, made at configure time if absent).
set(BF_PROBE_SRC "src" CACHE PATH
	"Tree the probes compile packet.cpp, PacketQueue.cpp and Connection.cpp from")
get_filename_component(bf_probe_tree "${BF_PROBE_SRC}" ABSOLUTE BASE_DIR ${BF_ROOT})
if(NOT EXISTS ${bf_probe_tree}/packet.cpp AND bf_probe_tree STREQUAL "${BF_ROOT}/build/port/shadow/src")
	message(STATUS "probes: making the shadow tree (port/tools/shadow.sh)")
	execute_process(COMMAND ${BF_ROOT}/port/tools/shadow.sh OUTPUT_QUIET RESULT_VARIABLE bf_shadow)
	if(NOT bf_shadow EQUAL 0)
		message(FATAL_ERROR "port/tools/shadow.sh failed (${bf_shadow})")
	endif()
endif()
if(NOT EXISTS ${bf_probe_tree}/packet.cpp)
	message(FATAL_ERROR "BF_PROBE_SRC=${BF_PROBE_SRC}: no packet.cpp in ${bf_probe_tree}")
endif()
message(STATUS "probes: from ${bf_probe_tree}")

add_library(bf_probe_game STATIC
	${bf_probe_tree}/packet.cpp
	${bf_probe_tree}/device/PacketQueue.cpp
	${bf_probe_tree}/device/Connection.cpp)
bf_client_flags(bf_probe_game ${bf_probe_tree})
add_executable(${BF_EXE}-packetlen port/probe/packetlen.cpp)
bf_client_flags(${BF_EXE}-packetlen ${bf_probe_tree})
target_link_libraries(${BF_EXE}-packetlen PRIVATE bf_probe_game)
add_executable(${BF_EXE}-netprobe port/probe/netprobe.cpp port/probe/probe_support.cpp)
bf_client_flags(${BF_EXE}-netprobe ${bf_probe_tree})
target_link_libraries(${BF_EXE}-netprobe PRIVATE bf_probe_game bf_instrument)
add_custom_target(probes DEPENDS ${BF_EXE}-packetlen ${BF_EXE}-netprobe)
