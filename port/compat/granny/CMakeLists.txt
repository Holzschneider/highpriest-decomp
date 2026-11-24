# The Granny adapter (M12.3a): granny2.dll's Granny* names on
# port/native/formats/granny. Built only under the root project:
#   cmake --preset debug && cmake --build --preset debug --target bf_compat_granny bifrost-compat-granny-test
#   build/cmake/debug/compat/granny/bifrost-compat-granny-test

add_library(bf_compat_granny STATIC bf_compat_granny.cpp)
bf_compat_flags(bf_compat_granny)
target_link_libraries(bf_compat_granny PUBLIC bf_granny bf_instrument)

# The test reads the game's .gr2 files with the reader module's own archive
# reader (its test/grf.cpp), so it needs no client code.
add_executable(${BF_EXE}-compat-granny-test
	bifrost-compat-granny-test.cpp
	${BF_ROOT}/port/native/formats/granny/test/grf.cpp)
bf_compat_flags(${BF_EXE}-compat-granny-test)
target_include_directories(${BF_EXE}-compat-granny-test PRIVATE ${BF_ROOT}/port/native/formats/granny/test)
find_package(ZLIB REQUIRED)
target_link_libraries(${BF_EXE}-compat-granny-test PRIVATE bf_compat_granny ZLIB::ZLIB)
