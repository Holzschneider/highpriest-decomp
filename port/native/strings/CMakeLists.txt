cmake_minimum_required(VERSION 3.20)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../name.cmake)
project(bf_strings LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 14.0)
endif()
if(NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES arm64)
endif()

set(BF_INSTRUMENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../instrument)

add_library(bf_strings STATIC src/bf_strings.cpp)
target_include_directories(bf_strings PUBLIC include PRIVATE ${BF_INSTRUMENT_DIR})
target_compile_definitions(bf_strings PRIVATE BF_APP_NAME="${BF_APP_NAME}")
target_compile_options(bf_strings PRIVATE -Wall -Wextra)

add_executable(bifrost-strings-test test/strings_test.cpp ${BF_INSTRUMENT_DIR}/bf_trace.cpp)
target_include_directories(bifrost-strings-test PRIVATE ${BF_INSTRUMENT_DIR})
target_compile_definitions(bifrost-strings-test PRIVATE BF_APP_NAME="${BF_APP_NAME}"
  BF_STRINGS_OUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}"
  BF_STRINGS_TABLE="${CMAKE_CURRENT_SOURCE_DIR}/../../data/strings/translation.tsv")
target_compile_options(bifrost-strings-test PRIVATE -Wall -Wextra)
target_link_libraries(bifrost-strings-test PRIVATE bf_strings)
