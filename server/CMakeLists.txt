cmake_minimum_required(VERSION 3.26)
project(server C)

set(CMAKE_C_STANDARD 23)

find_package(cJSON REQUIRED)
include_directories(${CJSON_INCLUDE_DIRS})

add_compile_options(-Wall -Wextra -Wdouble-promotion)

if (CMAKE_BUILD_TYPE MATCHES "Debug")

    add_compile_options(-g3)
    add_compile_options(-fsanitize=address,undefined -fsanitize-trap=undefined)
    add_compile_options(-fsanitize=null)
    add_compile_options(-fsanitize=leak)

    add_link_options(-fsanitize=address,undefined -fsanitize-trap=undefined)
    add_link_options(-fsanitize=null)
    add_link_options(-fsanitize=leak)

    set(ASAN_OPTIONS=abort_on_error=1:halt_on_error=1:detect_leaks=1)
    set(UBSAN_OPTIONS=abort_on_error=1:halt_on_error=1)

endif ()

if (CMAKE_BUILD_TYPE MATCHES "Release")
    add_compile_options(-O3)
    add_compile_options(-march=native)
    add_compile_options(-ffast-math)
endif ()

add_executable(server main.c
        common.h
        chess_rules.h
        chess_rules.c
        connection.c
        game.c
        game.h)
target_link_libraries(server PUBLIC ${CJSON_LIBRARIES})
