# bifrost-types-test: port/client/include/bf_types.h against the Wine headers
# the build uses today (M11.0b). Standalone:
#   cmake -S port/client/test -B build/client/test -G Ninja
#   cmake --build build/client/test && build/client/test/bifrost-types-test
# The build also compiles bf_types.h alone, as gnu89 C and as C++14 with the
# flags of a src/ unit (no Windows headers on the path), and strictly
# (-Wall -Wextra -pedantic -Werror) without the prefix; ctest runs the test
# program and checks that an Objective-C++ unit cannot include the header.
cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "")
endif()
if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
	set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")
endif()

project(bf_client_types LANGUAGES C CXX)

get_filename_component(BF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(BF_TYPES_INCLUDE "${BF_ROOT}/port/client/include")
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# The client's flags, from the one place they are written.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BF_ROOT}/port/tools/flags.sh)
execute_process(
	COMMAND env ROOT=${BF_ROOT} /bin/bash -c
		". \"$ROOT/port/tools/flags.sh\" && printf '%s\\n' \"$BF_TARGET $BF_DEFINES $BF_INCLUDES\" \"$WIN_HEADERS\""
	OUTPUT_VARIABLE bf_flags RESULT_VARIABLE bf_result OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT bf_result EQUAL 0)
	message(FATAL_ERROR "port/tools/flags.sh failed (${bf_result})")
endif()
string(REPLACE "\n" ";" bf_flags "${bf_flags}")
list(GET bf_flags 0 BF_UNIT_FLAGS)     # target, defines, prefix and port include paths
list(GET bf_flags 1 BF_WIN_HEADERS)
separate_arguments(bf_unit_flags UNIX_COMMAND "${BF_UNIT_FLAGS}")
set(BF_WINE_FLAGS "${BF_UNIT_FLAGS} -I${BF_ROOT}/src -isystem ${BF_WIN_HEADERS}")
separate_arguments(bf_wine_flags UNIX_COMMAND "${BF_WINE_FLAGS}")

# The test unit, generated from bf_types.h.
set(gen ${BF_ROOT}/port/client/test/gen_types_test.py)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/types_test.cpp
	COMMAND ${Python3_EXECUTABLE} ${gen} --cxx ${CMAKE_CXX_COMPILER}
		--header ${BF_TYPES_INCLUDE}/bf_types.h
		--out ${CMAKE_CURRENT_BINARY_DIR}/types_test.cpp
		"--wine-flags=${BF_WINE_FLAGS}"
	DEPENDS ${gen} ${BF_TYPES_INCLUDE}/bf_types.h ${BF_ROOT}/port/tools/flags.sh
	COMMENT "Generating the layout test of bf_types.h"
	VERBATIM)

add_executable(bifrost-types-test ${CMAKE_CURRENT_BINARY_DIR}/types_test.cpp)
target_include_directories(bifrost-types-test PRIVATE ${BF_TYPES_INCLUDE})
set_target_properties(bifrost-types-test PROPERTIES CXX_STANDARD 14 CXX_EXTENSIONS ON)
target_compile_options(bifrost-types-test PRIVATE ${bf_wine_flags})

# bf_types.h alone, with a src/ unit's flags and no Windows headers.
add_library(bf_types_alone OBJECT alone.c alone.cpp)
target_include_directories(bf_types_alone PRIVATE ${BF_TYPES_INCLUDE})
target_compile_options(bf_types_alone PRIVATE ${bf_unit_flags}
	$<$<COMPILE_LANGUAGE:C>:-std=gnu89> $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++14>)
# bf_types.h alone, strictly, without the prefix.
add_library(bf_types_strict OBJECT strict.c strict.cpp)
target_include_directories(bf_types_strict PRIVATE ${BF_TYPES_INCLUDE})
target_compile_options(bf_types_strict PRIVATE -Wall -Wextra -Werror
	$<$<COMPILE_LANGUAGE:C>:-std=gnu89> $<$<COMPILE_LANGUAGE:CXX>:-std=c++14 -pedantic>)
add_dependencies(bifrost-types-test bf_types_alone bf_types_strict)

enable_testing()
add_test(NAME types-layout COMMAND bifrost-types-test)
add_test(NAME types-not-objcxx COMMAND ${CMAKE_CXX_COMPILER} -x objective-c++ -fsyntax-only
	-I${BF_TYPES_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR}/objcxx.mm)
set_tests_properties(types-not-objcxx PROPERTIES PASS_REGULAR_EXPRESSION "must not include it")
