# port/compat/gdi: what is left of the gdi32 adapter after M11.8 (README.md):
# GetStockObject's brushes and the input adapter's two port-side functions.
# Built only under the root project (bf_compat_flags, bf_instrument).
add_library(bf_compat_gdi STATIC src/bf_compat_gdi.cpp)
bf_compat_flags(bf_compat_gdi)
target_include_directories(bf_compat_gdi PUBLIC include)
target_compile_options(bf_compat_gdi PRIVATE -Wall -Wextra)
target_link_libraries(bf_compat_gdi PUBLIC bf_instrument)

add_executable(${BF_EXE}-compat-gdi-test test/gdi_test.cpp)
bf_compat_flags(${BF_EXE}-compat-gdi-test)
target_compile_options(${BF_EXE}-compat-gdi-test PRIVATE -Wall -Wextra)
target_link_libraries(${BF_EXE}-compat-gdi-test PRIVATE bf_compat_gdi)
