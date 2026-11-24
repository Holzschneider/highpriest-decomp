# The view module: logical resolution, UI scale, the resize flow and the
# pointer mapping, joining the app and gfx modules. Standalone:
#   cmake -S port/native/view -B build/native/view -G Ninja
#   cmake --build build/native/view && build/native/view/bifrost-view-test
cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
		set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "")
	endif()
	if(NOT CMAKE_OSX_ARCHITECTURES)
		set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")
	endif()
endif()

project(bf_view LANGUAGES C CXX OBJC OBJCXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../name.cmake)
set(BF_INSTRUMENT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../instrument)

# The two modules it joins, unless a parent project has them already.
if(NOT TARGET bf_app)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../app ${CMAKE_CURRENT_BINARY_DIR}/app EXCLUDE_FROM_ALL)
endif()
if(NOT TARGET bf_gfx)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../gfx ${CMAKE_CURRENT_BINARY_DIR}/gfx EXCLUDE_FROM_ALL)
endif()

add_library(bf_view STATIC src/bf_view.cpp)
target_include_directories(bf_view
	PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
	PRIVATE ${BF_INSTRUMENT_DIR})
target_compile_definitions(bf_view PRIVATE BF_APP_NAME="${BF_APP_NAME}")
set_target_properties(bf_view PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_compile_options(bf_view PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(bf_view PUBLIC bf_app bf_gfx)

# The test program; bf_trace.cpp is compiled into it as it is.
add_executable(bifrost-view-test
	test/bf_view_test.mm
	${BF_INSTRUMENT_DIR}/bf_trace.cpp)
target_include_directories(bifrost-view-test PRIVATE ${BF_INSTRUMENT_DIR})
target_compile_definitions(bifrost-view-test PRIVATE BF_APP_NAME="${BF_APP_NAME}"
	BF_VIEW_TEST_OUT="${CMAKE_CURRENT_BINARY_DIR}/test-out")
set_target_properties(bifrost-view-test PROPERTIES
	CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
	OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED ON)
target_compile_options(bifrost-view-test PRIVATE
	$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc> -Wall -Wno-unused-parameter)
target_link_libraries(bifrost-view-test PRIVATE bf_view
	"-framework AppKit" "-framework CoreText" "-framework ImageIO")
