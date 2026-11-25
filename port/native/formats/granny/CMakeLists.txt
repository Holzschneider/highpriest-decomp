# bf_granny: reader for Granny 2 (.gr2) models. Standalone:
#   cmake -S port/native/formats/granny -B build/native/granny -G Ninja
#   cmake --build build/native/granny && build/native/granny/bifrost-granny-test
# or under add_subdirectory() from a larger build.
cmake_minimum_required(VERSION 3.16)
project(bf_granny LANGUAGES C CXX)

set(BF_GRANNY_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
include(${BF_GRANNY_ROOT}/port/name.cmake)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_library(bf_granny STATIC
	src/bf_granny.cpp
	src/bf_granny_file.cpp
	src/bf_granny_oodle0.cpp
	src/bf_granny_pose.cpp
	src/bf_granny_runtime.cpp
	src/bf_granny_texture.cpp
)
target_include_directories(bf_granny
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${BF_GRANNY_ROOT}/port/instrument)

# The test program carries its own copy of the trace; the client links one.
add_executable(bifrost-granny-test
	test/bifrost_granny_test.cpp
	test/runtime_checks.cpp
	test/grf.cpp
	${BF_GRANNY_ROOT}/port/instrument/bf_trace.cpp
)
target_include_directories(bifrost-granny-test PRIVATE ${BF_GRANNY_ROOT}/port/instrument)
target_compile_definitions(bifrost-granny-test PRIVATE BF_APP_NAME="${BF_APP_NAME}")
find_package(ZLIB REQUIRED)
target_link_libraries(bifrost-granny-test PRIVATE bf_granny ZLIB::ZLIB)

foreach(t bf_granny bifrost-granny-test)
	set_target_properties(${t} PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
		OSX_DEPLOYMENT_TARGET 14.0
		OSX_ARCHITECTURES arm64)
	target_compile_options(${t} PRIVATE -Wall -Wextra -Wno-unused-parameter
		$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
endforeach()
