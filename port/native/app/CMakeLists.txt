# The application module: window, event queue, key and pointer state,
# clipboard. Standalone:
#   cmake -S port/native/app -B build/native/app -G Ninja
#   cmake --build build/native/app && build/native/app/bifrost-app-test
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

project(bf_app LANGUAGES C CXX OBJC OBJCXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../name.cmake)
set(BF_INSTRUMENT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../instrument)

add_library(bf_app STATIC
	src/bf_app.mm
	src/bf_app_queue.cpp
	src/bf_app_keys.cpp)
target_include_directories(bf_app
	PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
	PRIVATE ${BF_INSTRUMENT_DIR})
target_compile_definitions(bf_app PRIVATE
	BF_APP_NAME="${BF_APP_NAME}" BF_BUNDLE_ID="${BF_BUNDLE_ID}")
set_target_properties(bf_app PROPERTIES
	CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
	OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED ON)
target_compile_options(bf_app PRIVATE
	$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
	-Wall -Wextra -Wno-unused-parameter)
target_link_libraries(bf_app PUBLIC
	"-framework AppKit" "-framework QuartzCore" "-framework Metal")

# The test program; bf_trace.cpp is compiled into it as it is.
add_executable(bifrost-app-test
	test/bf_app_test.mm
	${BF_INSTRUMENT_DIR}/bf_trace.cpp)
target_include_directories(bifrost-app-test PRIVATE ${BF_INSTRUMENT_DIR})
target_compile_definitions(bifrost-app-test PRIVATE BF_APP_NAME="${BF_APP_NAME}")
set_target_properties(bifrost-app-test PROPERTIES
	CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
	OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED ON)
target_compile_options(bifrost-app-test PRIVATE
	$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc> -Wall -Wno-unused-parameter)
target_link_libraries(bifrost-app-test PRIVATE bf_app)
