cmake_minimum_required(VERSION 3.20)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../name.cmake)
project(bf_gfx LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 17)
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 14.0)
endif()
if(NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES arm64)
endif()

set(BF_INSTRUMENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../instrument)

# The shaders are compiled at run time (the Metal compiler is not part of
# the command-line tools): the source is embedded as a string.
set(BF_GFX_SHADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/bf_gfx_shaders.metal)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BF_GFX_SHADER_SRC})
file(READ ${BF_GFX_SHADER_SRC} BF_GFX_SHADER_TEXT)
set(BF_GFX_SHADER_INC ${CMAKE_CURRENT_BINARY_DIR}/generated/bf_gfx_shaders.inc)
file(CONFIGURE OUTPUT ${BF_GFX_SHADER_INC}
     CONTENT "static const char k_bf_gfx_shader_source[] = R\"BFMSL(@BF_GFX_SHADER_TEXT@)BFMSL\";\n"
     @ONLY)

add_library(bf_gfx STATIC src/bf_gfx.mm)
target_include_directories(bf_gfx PUBLIC include PRIVATE ${BF_INSTRUMENT_DIR} ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_compile_definitions(bf_gfx PRIVATE BF_APP_NAME="${BF_APP_NAME}")
target_compile_options(bf_gfx PRIVATE -Wall -Wextra $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
target_link_libraries(bf_gfx PUBLIC "-framework Foundation" "-framework Metal" "-framework QuartzCore"
                      "-framework CoreGraphics" "-framework ImageIO")

add_executable(bifrost-gfx-test test/gfx_test.mm ${BF_INSTRUMENT_DIR}/bf_trace.cpp)
target_include_directories(bifrost-gfx-test PRIVATE ${BF_INSTRUMENT_DIR})
target_compile_definitions(bifrost-gfx-test PRIVATE BF_APP_NAME="${BF_APP_NAME}"
                           BF_GFX_TEST_OUT="${CMAKE_CURRENT_BINARY_DIR}/test-out")
target_compile_options(bifrost-gfx-test PRIVATE -Wall -Wextra $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
target_link_libraries(bifrost-gfx-test PRIVATE bf_gfx "-framework CoreText")
