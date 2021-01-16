cmake_minimum_required(VERSION 2.18)

project(rfind C)

# set default build type if not specified by user
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=c99")

set(sources
    src/find.c
    src/cmdline.c
    src/expressions.c
    src/test_empty.c
    src/test_name.c
    src/action_print.c)

add_executable(rfind ${sources})

enable_testing()
add_test(NAME compares COMMAND ${CMAKE_SOURCE_DIR}/test/compare.sh ${CMAKE_BINARY_DIR}/rfind )

install(TARGETS rfind DESTINATION ${CMAKE_INSTALL_BINDIR})

