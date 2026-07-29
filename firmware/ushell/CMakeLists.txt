cmake_minimum_required(VERSION 4.4)

set(CMAKE_C_COMPILER /usr/bin/m68k-elf-gcc)
set(CMAKE_CXX_COMPILER m68k-elf-g++)
set(CMAKE_ASM_COMPILER m68k-elf-as)
set(CMAKE_AR m68k-elf-ar)
set(CMAKE_RANLIB m68k-elf-ranlib)
set(CMAKE_LINKER "m68k-elf-ld")

project(ushell C)

# Set the toolchain
set(CMAKE_SYSTEM_NAME Unicorn)
set(CMAKE_SYSTEM_PROCESSOR m68k)

# Add C source files
set(C_SOURCES
    stdio.c
    ushell.c
)

# Create ushell static library
add_library(ushell OBJECT ${C_SOURCES})

target_compile_options(ushell PRIVATE
    -m68010
    -msoft-float 
    -std=c23
    -specs=picolibc.specs
    # -nostartfiles
    -ffreestanding
    -fno-builtin
    -fno-omit-frame-pointer
    -O1
    # -ffunction-sections
    # -fdata-sections
    -fno-delete-null-pointer-checks
    # -fverbose-asm
    -Wall
)

# Add include directories
target_include_directories(ushell PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/
)

# Set the linker script + other ld flags
target_link_options(ushell PRIVATE
    -m68010
    -specs=picolibc.specs
    -nostartfiles
    # -nostdlib
    # -fno-omit-frame-pointer
    -Wl,--build-id=none
    -Wl,--print-memory-usage
)
