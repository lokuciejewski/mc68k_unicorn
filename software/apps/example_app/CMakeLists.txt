cmake_minimum_required(VERSION 4.4)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/unicorn_toolchain.cmake")

project(example_app C)

# Add C source files
set(C_SOURCES
    crt0.c
    main.c
)

# Create the executable
add_executable(example_app.elf ${C_SOURCES})


target_compile_definitions(example_app.elf PRIVATE
    ${BOARD_DEFINITIONS}
)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_options(example_app.elf PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/unicorn_app_v1.ld)

unicorn_add_listing(example_app.elf)
unicorn_strip_elf(example_app.elf)