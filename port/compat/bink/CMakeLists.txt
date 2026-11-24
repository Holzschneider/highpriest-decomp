# The Bink adapter (M12.4a): the Bink SDK names src/movie/CBink.cpp calls
# (port/compat/include/bf_bink_sdk.h), on port/native/formats/bink and
# port/native/audio. Built only under the root project:
#   cmake --preset debug && cmake --build --preset debug --target bf_compat_bink bifrost-compat-bink-test
#   build/cmake/debug/compat/bink/bifrost-compat-bink-test

add_library(bf_compat_bink STATIC bf_compat_bink.cpp)
bf_compat_flags(bf_compat_bink)
target_include_directories(bf_compat_bink PUBLIC include)
target_compile_options(bf_compat_bink PRIVATE -Wall -Wextra)
target_link_libraries(bf_compat_bink PUBLIC bf_bink bf_audio bf_sys bf_instrument)

# Draws through the DirectDraw adapter offscreen, as CBink does; fakes the
# gdi adapter's surface-DC functions, which the d3d adapter imports.
add_executable(${BF_EXE}-compat-bink-test bifrost-compat-bink-test.cpp)
bf_compat_flags(${BF_EXE}-compat-bink-test)
target_compile_options(${BF_EXE}-compat-bink-test PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
target_link_libraries(${BF_EXE}-compat-bink-test PRIVATE bf_compat_bink bf_compat_d3d bf_gfx)
