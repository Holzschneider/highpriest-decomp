# port/compat/input: the client's user32 window, message, keyboard, cursor and
# clipboard names, DirectInput 7 and imm32, on port/native/app and
# port/native/view (M5.1, M5.2). Built only under the root project
# (bf_compat_flags, bf_instrument, bf_app, bf_view, bf_text).
add_library(bf_compat_input STATIC
	src/bf_compat_input_keys.cpp
	src/bf_compat_input_window.cpp
	src/bf_compat_input_misc.cpp
	src/bf_compat_input_dinput.cpp
	src/bf_compat_input_imm.cpp)
bf_compat_flags(bf_compat_input)
target_include_directories(bf_compat_input PUBLIC include)
target_compile_options(bf_compat_input PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(bf_compat_input PUBLIC bf_app bf_view bf_text bf_instrument)
# GetDC(NULL) and the class brush come from the gdi area; a tree without it
# builds the test with stand-ins for its two functions.
if(TARGET bf_compat_gdi)
	target_link_libraries(bf_compat_input PUBLIC bf_compat_gdi)
	set(bf_input_test_gdi)
else()
	set(bf_input_test_gdi test/fake_gdi.cpp)
endif()

# Opens a window; events are injected through bf_app_post_event, so it runs on
# the locked screen of the unattended Mac.
add_executable(${BF_EXE}-compat-input-test test/input_test.cpp test/fake_kernel32.cpp ${bf_input_test_gdi})
bf_compat_flags(${BF_EXE}-compat-input-test)
target_compile_options(${BF_EXE}-compat-input-test PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(${BF_EXE}-compat-input-test PRIVATE bf_compat_input)
