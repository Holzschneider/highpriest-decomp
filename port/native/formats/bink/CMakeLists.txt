cmake_minimum_required(VERSION 3.20)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../../name.cmake)
project(bf_bink LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 14.0)
endif()
if(NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES arm64)
endif()
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(BF_INSTRUMENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../instrument)

add_library(bf_bink STATIC
  src/bf_bink.cpp
  src/bf_bink_video.cpp
  src/bf_bink_audio.cpp
  src/bf_bink_tables.cpp)
target_include_directories(bf_bink PUBLIC include PRIVATE src ${BF_INSTRUMENT_DIR})
target_compile_definitions(bf_bink PRIVATE BF_APP_NAME="${BF_APP_NAME}")
target_compile_options(bf_bink PRIVATE -Wall -Wextra)
target_link_libraries(bf_bink PUBLIC "-framework Accelerate")

add_executable(bifrost-bink-test test/bink_test.cpp ${BF_INSTRUMENT_DIR}/bf_trace.cpp)
target_include_directories(bifrost-bink-test PRIVATE src ${BF_INSTRUMENT_DIR})
target_compile_definitions(bifrost-bink-test PRIVATE BF_APP_NAME="${BF_APP_NAME}")
target_compile_options(bifrost-bink-test PRIVATE -Wall -Wextra)
target_link_libraries(bifrost-bink-test PRIVATE bf_bink z)
