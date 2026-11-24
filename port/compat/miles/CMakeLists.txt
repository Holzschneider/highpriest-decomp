# The Miles adapter (M7.2a): mss32's AIL_* names on port/native/audio.
# Built only under the root project:
#   cmake --preset debug && cmake --build --preset debug --target bf_compat_miles bifrost-compat-miles-test
#   build/cmake/debug/compat/miles/bifrost-compat-miles-test

add_library(bf_compat_miles STATIC bf_compat_miles.cpp)
bf_compat_flags(bf_compat_miles)
target_link_libraries(bf_compat_miles PUBLIC bf_audio bf_sys)

add_executable(${BF_EXE}-compat-miles-test bifrost-compat-miles-test.cpp)
bf_compat_flags(${BF_EXE}-compat-miles-test)
target_link_libraries(${BF_EXE}-compat-miles-test PRIVATE bf_compat_miles bf_instrument)
