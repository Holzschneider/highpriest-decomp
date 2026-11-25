# port/compat/win32: kernel32, shell32's ShellExecute and user32's
# MessageBox, on port/native/sys and port/native/text (M3.1a).
# Built only under the root project (bf_compat_flags, bf_sys, bf_text, bf_instrument).
add_library(bf_compat_win32 STATIC
	src/bf_win32_base.cpp
	src/bf_win32_file.cpp
	src/bf_win32_misc.cpp
	src/bf_win32_shell.cpp)
bf_compat_flags(bf_compat_win32)
target_compile_options(bf_compat_win32 PRIVATE -Wall -Wextra)
target_link_libraries(bf_compat_win32 PUBLIC bf_sys bf_text bf_instrument)

# The test calls the names as the client does. client_calls.cpp is compiled
# with the client's own flags (bf_client_flags: gnu++14, the prefix header
# without BF_COMPAT_ADAPTER), so its fopen, access, _mkdir, remove, ... are
# the client's, redirected by bf_crt.h.
add_library(bf_compat_win32_client_calls OBJECT test/client_calls.cpp)
bf_client_flags(bf_compat_win32_client_calls ${CMAKE_CURRENT_SOURCE_DIR}/test)

# The C library's file names on client paths are port/client/src/bf_crt.cpp's
# since M11.3 (port code that stays); the test still calls them as the client does.
add_executable(${BF_EXE}-compat-win32-test test/win32_test.cpp ${BF_ROOT}/port/client/src/bf_crt.cpp
	$<TARGET_OBJECTS:bf_compat_win32_client_calls>)
bf_compat_flags(${BF_EXE}-compat-win32-test)
target_compile_options(${BF_EXE}-compat-win32-test PRIVATE -Wall -Wextra)
target_link_libraries(${BF_EXE}-compat-win32-test PRIVATE bf_compat_win32)
