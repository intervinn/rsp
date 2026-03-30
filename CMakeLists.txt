cmake_minimum_required(VERSION 3.16)
project(rsp LANGUAGES C)

add_custom_target(run
COMMAND ./build/rsp
DEPENDS rsp
WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
COMMENT "running...")

add_custom_target(format
COMMAND find . -iname '*.h' -o -iname '*.c' | clang-format --style=file -i --files=/dev/stdin
DEPENDS rsp
WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
COMMENT "formatting...")

set(CMAKE_C_STANDARD 11)
add_executable(rsp)

include(Sources.cmake)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rsp PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(rsp PRIVATE -fsanitize=address)
endif()
