# The DirectDraw 7 / Direct3D 7 adapter (M4.4): the names src/3dlib calls, on
# port/native/gfx. Built only under the root project:
#   cmake --preset debug && cmake --build --preset debug --target bf_compat_d3d bifrost-compat-d3d-test
#   build/cmake/debug/compat/d3d/bifrost-compat-d3d-test [output directory]

add_library(bf_compat_d3d STATIC
	src/ddraw.cpp
	src/d3ddevice.cpp
	src/target_lock.cpp
	src/guids.cpp)
bf_compat_flags(bf_compat_d3d)
target_include_directories(bf_compat_d3d PUBLIC include)
target_compile_options(bf_compat_d3d PRIVATE -Wall -Wextra)
# A surface DC is a bf_text_canvas (port/native/text).
target_link_libraries(bf_compat_d3d PUBLIC bf_gfx bf_text bf_instrument)

add_executable(${BF_EXE}-compat-d3d-test test/d3d_test.cpp)
bf_compat_flags(${BF_EXE}-compat-d3d-test)
target_compile_options(${BF_EXE}-compat-d3d-test PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
target_compile_definitions(${BF_EXE}-compat-d3d-test PRIVATE BF_D3D_TEST_OUT="${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(${BF_EXE}-compat-d3d-test PRIVATE bf_compat_d3d)
