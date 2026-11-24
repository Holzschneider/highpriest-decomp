# The control module: the control socket (PROTOCOL.md). Standalone:
#   cmake -S port/native/control -B build/native/control -G Ninja
#   cmake --build build/native/control && build/native/control/bifrost-control-test
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

project(bf_control LANGUAGES C CXX OBJC OBJCXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../name.cmake)
set(BF_INSTRUMENT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../instrument)

# The modules it builds on, unless the parent build has them already.
if(NOT TARGET bf_app)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../app ${CMAKE_CURRENT_BINARY_DIR}/app EXCLUDE_FROM_ALL)
endif()
if(NOT TARGET bf_sys)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../sys ${CMAKE_CURRENT_BINARY_DIR}/sys EXCLUDE_FROM_ALL)
endif()

add_library(bf_control STATIC
	src/bf_control.cpp
	src/bf_control_builtin.cpp
	src/bf_control_json.cpp)
target_include_directories(bf_control
	PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
	PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src ${BF_INSTRUMENT_DIR})
target_compile_definitions(bf_control PRIVATE BF_APP_NAME="${BF_APP_NAME}")
set_target_properties(bf_control PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_compile_options(bf_control PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(bf_control PUBLIC bf_app bf_sys)

# The test program; bf_trace.cpp is compiled into it as it is.
add_executable(bifrost-control-test
	test/bf_control_test.cpp
	test/header_c.c
	test/header_cxx14.cpp
	${BF_INSTRUMENT_DIR}/bf_trace.cpp)
target_include_directories(bifrost-control-test PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src ${BF_INSTRUMENT_DIR})
target_compile_definitions(bifrost-control-test PRIVATE BF_APP_NAME="${BF_APP_NAME}")
set_target_properties(bifrost-control-test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
set_source_files_properties(test/header_cxx14.cpp PROPERTIES COMPILE_OPTIONS "-std=c++14")
target_compile_options(bifrost-control-test PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(bifrost-control-test PRIVATE bf_control)

# A headless stand-in client serving the protocol with a fake login screen,
# for the harness cross-check (test/crosscheck.py).
add_executable(bifrost-control-echo
	test/bf_control_echo.cpp
	${BF_INSTRUMENT_DIR}/bf_trace.cpp)
target_include_directories(bifrost-control-echo PRIVATE ${BF_INSTRUMENT_DIR})
target_compile_definitions(bifrost-control-echo PRIVATE BF_APP_NAME="${BF_APP_NAME}")
set_target_properties(bifrost-control-echo PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_compile_options(bifrost-control-echo PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(bifrost-control-echo PRIVATE bf_control z)
