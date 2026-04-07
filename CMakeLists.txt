cmake_minimum_required(VERSION 3.16)
project(rsp LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(librsp STATIC)
add_executable(rsp)
target_link_libraries(rsp PRIVATE librsp)

include(Sources.cmake)

find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=*,-modernize-use-trailing-return-type")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(SAN_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer -g")
    target_compile_options(librsp PUBLIC ${SAN_FLAGS})
    target_link_options(librsp PUBLIC ${SAN_FLAGS})
endif()

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

add_custom_target(run
    COMMAND $<TARGET_FILE:rsp>
    DEPENDS rsp
    COMMENT "Running rsp with sanitizers..."
)

add_custom_target(format
    COMMAND find . -iname '*.h' -o -iname '*.c' | clang-format --style=file -i --files=/dev/stdin
    DEPENDS rsp
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
    COMMENT "formatting..."
)