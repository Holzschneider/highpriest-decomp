cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "")
endif()
if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
	set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")
endif()

project(bf_sys LANGUAGES C CXX OBJCXX)

set(BF_SYS_PORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
include("${BF_SYS_PORT_DIR}/name.cmake")

option(BF_SYS_SANITIZE "Build bf_sys and its test with address and undefined-behaviour sanitizers" OFF)

add_library(bf_sys STATIC
	src/bf_clock.cpp
	src/bf_thread.cpp
	src/bf_file.cpp
	src/bf_settings.mm
	src/bf_keychain.cpp
	src/bf_shell.mm
)
target_include_directories(bf_sys
	PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE "${BF_SYS_PORT_DIR}/instrument")
target_compile_definitions(bf_sys PRIVATE BF_APP_NAME="${BF_APP_NAME}")
target_compile_features(bf_sys PUBLIC cxx_std_17)
set_target_properties(bf_sys PROPERTIES CXX_EXTENSIONS OFF OBJCXX_STANDARD 17)
target_compile_options(bf_sys PRIVATE -Wall -Wextra
	$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
target_link_libraries(bf_sys PUBLIC
	"-framework CoreFoundation"
	"-framework Foundation"
	"-framework AppKit"
	"-framework CoreGraphics"
	"-framework Security")

add_executable(bifrost-sys-test
	test/sys_test.cpp
	"${BF_SYS_PORT_DIR}/instrument/bf_trace.cpp")
target_include_directories(bifrost-sys-test PRIVATE "${BF_SYS_PORT_DIR}/instrument")
target_compile_definitions(bifrost-sys-test PRIVATE BF_APP_NAME="${BF_APP_NAME}")
target_compile_options(bifrost-sys-test PRIVATE -Wall -Wextra)
target_link_libraries(bifrost-sys-test PRIVATE bf_sys)

if(BF_SYS_SANITIZE)
	foreach(target bf_sys bifrost-sys-test)
		target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g)
		target_link_options(${target} PUBLIC -fsanitize=address,undefined)
	endforeach()
endif()
