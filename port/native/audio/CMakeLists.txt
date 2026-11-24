# bf_audio: AVAudioEngine voices, one music stream, WAV and MP3 decoding.
#   cmake -S port/native/audio -B build/native/audio -G Ninja
#   cmake --build build/native/audio && build/native/audio/bifrost-audio-test
cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	# Standalone: this module is the whole build. Under add_subdirectory the
	# parent project decides these.
	set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "")
	set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")
endif()

project(bf_audio LANGUAGES C CXX OBJCXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../name.cmake)
set(BF_INSTRUMENT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../instrument)

add_library(bf_audio STATIC
	src/bf_audio_decode.mm
	src/bf_audio_engine.mm)
target_include_directories(bf_audio PUBLIC include PRIVATE src ${BF_INSTRUMENT_DIR})
set_target_properties(bf_audio PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED ON)
target_compile_options(bf_audio PRIVATE -fobjc-arc -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(bf_audio PUBLIC "-framework AVFoundation" "-framework AudioToolbox" "-framework Foundation")

# The test program. bf_trace.cpp is compiled in as it is; in the client it is
# linked once for every module.
add_executable(bifrost-audio-test
	test/bifrost-audio-test.cpp
	test/grf.cpp
	test/header_c.c
	test/header_cxx14.cpp
	${BF_INSTRUMENT_DIR}/bf_trace.cpp)
target_include_directories(bifrost-audio-test PRIVATE ${BF_INSTRUMENT_DIR})
set_target_properties(bifrost-audio-test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
set_source_files_properties(test/header_cxx14.cpp PROPERTIES COMPILE_OPTIONS "-std=c++14")
target_compile_definitions(bifrost-audio-test PRIVATE BF_APP_NAME="${BF_APP_NAME}")
target_compile_options(bifrost-audio-test PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(bifrost-audio-test PRIVATE bf_audio z)
