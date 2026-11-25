# bifrost-types-test: port/client/include/bf_types.h against what the Wine
# headers gave the build (M11.0b), recorded in wine_types.ref before M11.11
# took them out of the repository. Part of the root build (ctest label
# headless); also standalone:
#   cmake -S port/client/test -B build/client/test -G Ninja
#   cmake --build build/client/test && (cd build/client/test && ctest)
# Two programs, generated from bf_types.h by gen_types_test.py: the C++14
# side and the gnu89 C side, each compiled with the flags of a src/ unit
# (no Windows headers anywhere). The build also compiles bf_types.h alone
# the same way, and strictly (-Wall -Wextra -pedantic -Werror) without the
# prefix; ctest runs both programs and checks that an Objective-C++ unit
# cannot include the header. Rewriting the reference needs the Wine headers
# back (port/README.md): gen_types_test.py --write-reference.
cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "")
endif()
if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
	set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")
endif()

project(bf_client_types LANGUAGES C CXX)

get_filename_component(BF_TYPES_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
include(${BF_TYPES_ROOT}/port/name.cmake)
string(TOLOWER "${BF_APP_NAME}" bf_types_exe)
set(BF_TYPES_INCLUDE "${BF_TYPES_ROOT}/port/client/include")
set(BF_TYPES_REF "${CMAKE_CURRENT_SOURCE_DIR}/wine_types.ref")
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# The client's flags, from the one place they are written.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BF_TYPES_ROOT}/port/tools/flags.sh)
execute_process(
	COMMAND env ROOT=${BF_TYPES_ROOT} /bin/bash -c
		". \"$ROOT/port/tools/flags.sh\" && printf '%s\\n' \"$BF_TARGET $BF_DEFINES $BF_INCLUDES\""
	OUTPUT_VARIABLE bf_types_flags RESULT_VARIABLE bf_result OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT bf_result EQUAL 0)
	message(FATAL_ERROR "port/tools/flags.sh failed (${bf_result})")
endif()
separate_arguments(bf_unit_flags UNIX_COMMAND "${bf_types_flags}")

# The test units, generated from bf_types.h and the reference.
set(gen ${BF_TYPES_ROOT}/port/client/test/gen_types_test.py)
foreach(lang cxx c)
	if(lang STREQUAL "cxx")
		set(unit ${CMAKE_CURRENT_BINARY_DIR}/types_test.cpp)
		set(name ${bf_types_exe}-types-test)
		set(arg c++)
	else()
		set(unit ${CMAKE_CURRENT_BINARY_DIR}/types_test_c.c)
		set(name ${bf_types_exe}-types-test-c)
		set(arg c)
	endif()
	add_custom_command(
		OUTPUT ${unit}
		COMMAND ${Python3_EXECUTABLE} ${gen} --lang ${arg}
			--cxx ${CMAKE_CXX_COMPILER} --cc ${CMAKE_C_COMPILER}
			--header ${BF_TYPES_INCLUDE}/bf_types.h --reference ${BF_TYPES_REF} --out ${unit}
		DEPENDS ${gen} ${BF_TYPES_INCLUDE}/bf_types.h ${BF_TYPES_REF}
		COMMENT "Generating the ${arg} layout test of bf_types.h"
		VERBATIM)
	add_executable(${name} ${unit})
	target_include_directories(${name} BEFORE PRIVATE ${BF_TYPES_INCLUDE})
	target_compile_options(${name} PRIVATE ${bf_unit_flags})
	set_target_properties(${name} PROPERTIES CXX_STANDARD 14 CXX_EXTENSIONS ON C_STANDARD 90 C_EXTENSIONS ON)
endforeach()

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
add_dependencies(${bf_types_exe}-types-test bf_types_alone bf_types_strict)
add_custom_target(types DEPENDS ${bf_types_exe}-types-test ${bf_types_exe}-types-test-c)

enable_testing()
add_test(NAME types-layout COMMAND ${bf_types_exe}-types-test)
add_test(NAME types-layout-c COMMAND ${bf_types_exe}-types-test-c)
add_test(NAME types-not-objcxx COMMAND ${CMAKE_CXX_COMPILER} -x objective-c++ -fsyntax-only
	-I${BF_TYPES_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR}/objcxx.mm)
set_tests_properties(types-not-objcxx PROPERTIES PASS_REGULAR_EXPRESSION "must not include it")
set_tests_properties(types-layout types-layout-c types-not-objcxx PROPERTIES LABELS headless TIMEOUT 300)
