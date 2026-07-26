cmake_minimum_required(VERSION 4.4)

project(ushell C)

# Set the toolchain
set(CMAKE_SYSTEM_NAME Unicorn)
set(CMAKE_SYSTEM_PROCESSOR m68k)

# Add C source files
set(C_SOURCES
    ushell.c
)

# Create ushell static library
add_library(ushell STATIC ${C_SOURCES})

# Panic configuration for no_std
# target_compile_options(unicorn_firmware.elf PRIVATE
#     -m68010
#     -msoft-float 
#     -std=c23
#     -nostdlib
#     -nostartfiles
#     -ffreestanding
#     -fno-builtin
#     -fno-omit-frame-pointer
#     -O0
#     -ffunction-sections
#     -fdata-sections
#     -fno-delete-null-pointer-checks
#     -fverbose-asm
#     -Wall
# )

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/
)

# Set the linker script + other ld flags
# target_link_options(unicorn_firmware.elf PRIVATE
#     -m68010
#     -nostartfiles
#     -nostdlib
#     -fno-omit-frame-pointer
#     -Wl,--build-id=none
#     -Wl,--print-memory-usage
#     -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/unicorn_v1.ld
#     -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/unicorn_firmware.map
# )
