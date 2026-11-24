# The preferences module: game-folder chooser and settings window. Standalone:
#   cmake -S port/native/prefs -B build/native/prefs -G Ninja
#   cmake --build build/native/prefs && build/native/prefs/bifrost-prefs-test
cmake_minimum_required(VERSION 3.20)

# Standalone only: under add_subdirectory the parent's settings stand.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
		set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "")
	endif()
	if(NOT CMAKE_OSX_ARCHITECTURES)
		set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")
	endif()
endif()

project(bf_prefs LANGUAGES C CXX OBJC OBJCXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../name.cmake)
set(BF_INSTRUMENT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../instrument)

# The modules this one builds on, unless the parent already has them.
if(NOT TARGET bf_sys)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../sys ${CMAKE_CURRENT_BINARY_DIR}/sys EXCLUDE_FROM_ALL)
endif()
if(NOT TARGET bf_app)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../app ${CMAKE_CURRENT_BINARY_DIR}/app EXCLUDE_FROM_ALL)
endif()

add_library(bf_prefs STATIC
	src/bf_prefs_folder.mm
	src/bf_prefs_window.mm)
target_include_directories(bf_prefs
	PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
	PRIVATE ${BF_INSTRUMENT_DIR})
target_compile_definitions(bf_prefs PRIVATE BF_APP_NAME="${BF_APP_NAME}")
set_target_properties(bf_prefs PROPERTIES
	CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
	OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED ON)
target_compile_options(bf_prefs PRIVATE
	$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
	-Wall -Wextra -Wno-unused-parameter)
target_link_libraries(bf_prefs PUBLIC bf_sys bf_app "-framework AppKit")

# The test program; bf_trace.cpp is compiled into it as it is.
add_executable(bifrost-prefs-test
	test/bf_prefs_test.mm
	${BF_INSTRUMENT_DIR}/bf_trace.cpp)
target_include_directories(bifrost-prefs-test PRIVATE ${BF_INSTRUMENT_DIR})
target_compile_definitions(bifrost-prefs-test PRIVATE
	BF_APP_NAME="${BF_APP_NAME}"
	BF_PREFS_TEST_DIR="${CMAKE_CURRENT_BINARY_DIR}/prefs-test")
set_target_properties(bifrost-prefs-test PROPERTIES
	CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
	OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED ON)
target_compile_options(bifrost-prefs-test PRIVATE
	$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc> -Wall -Wno-unused-parameter)
target_link_libraries(bifrost-prefs-test PRIVATE bf_prefs)
