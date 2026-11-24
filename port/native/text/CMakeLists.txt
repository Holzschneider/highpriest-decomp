cmake_minimum_required(VERSION 3.20)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../name.cmake)
project(bf_text LANGUAGES C CXX OBJCXX)

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

add_library(bf_text STATIC src/bf_text_codepage.cpp src/bf_text_font.mm)
target_include_directories(bf_text PUBLIC include PRIVATE src ${BF_INSTRUMENT_DIR})
target_compile_definitions(bf_text PRIVATE BF_APP_NAME="${BF_APP_NAME}")
target_compile_options(bf_text PRIVATE -Wall -Wextra $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
target_link_libraries(bf_text PUBLIC "-framework CoreText" "-framework CoreGraphics" "-framework CoreFoundation")

add_executable(bifrost-text-test test/text_test.mm ${BF_INSTRUMENT_DIR}/bf_trace.cpp)
target_include_directories(bifrost-text-test PRIVATE src ${BF_INSTRUMENT_DIR})
target_compile_definitions(bifrost-text-test PRIVATE BF_APP_NAME="${BF_APP_NAME}"
  BF_TEXT_OUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}"
  BF_TEXT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(bifrost-text-test PRIVATE -Wall -Wextra -fobjc-arc)
target_link_libraries(bifrost-text-test PRIVATE bf_text "-framework ImageIO" "-framework Foundation")
