# port/compat/gdi: the gdi32 names of the client on port/native/text (M4.5a).
# Built only under the root project (bf_compat_flags, bf_instrument, bf_text,
# bf_strings for the coverage hook of M9.5).
add_library(bf_compat_gdi STATIC src/bf_compat_gdi.cpp)
bf_compat_flags(bf_compat_gdi)
target_include_directories(bf_compat_gdi PUBLIC include)
target_compile_options(bf_compat_gdi PRIVATE -Wall -Wextra)
target_link_libraries(bf_compat_gdi PUBLIC bf_text bf_strings bf_instrument)

add_executable(${BF_EXE}-compat-gdi-test test/gdi_test.cpp test/png_write.cpp)
bf_compat_flags(${BF_EXE}-compat-gdi-test)
target_compile_options(${BF_EXE}-compat-gdi-test PRIVATE -Wall -Wextra)
target_compile_definitions(${BF_EXE}-compat-gdi-test PRIVATE
	BF_GDI_OUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}"
	BF_GDI_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${BF_EXE}-compat-gdi-test PRIVATE bf_compat_gdi z)
