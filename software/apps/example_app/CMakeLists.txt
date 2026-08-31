cmake_minimum_required(VERSION 4.4)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/unicorn_toolchain.cmake")

set(TARGET example_app)

project(${TARGET} C)

# Add C source files
set(C_SOURCES
    crt0.c
    main.c
)

# Create the executable
add_executable(${TARGET}.elf ${C_SOURCES})


target_compile_definitions(${TARGET}.elf PRIVATE
    ${BOARD_DEFINITIONS}
)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_options(${TARGET}.elf PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/unicorn_app_v1.ld)

unicorn_add_listing(${TARGET}.elf)
unicorn_strip_elf(${TARGET}.elf)