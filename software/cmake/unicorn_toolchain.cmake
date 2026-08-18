# Unicorn SBC toolchain file
cmake_minimum_required(VERSION 4.4)

# Enable generation of compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR m68k)

set(CMAKE_C_COMPILER m68k-elf-gcc)
set(CMAKE_CXX_COMPILER m68k-elf-g++)
set(CMAKE_ASM_COMPILER m68k-elf-as)
set(CMAKE_AR m68k-elf-ar)
set(CMAKE_RANLIB m68k-elf-ranlib)
set(CMAKE_SIZE m68k-elf-size)
set(CMAKE_LINKER "m68k-elf-ld")

# Board configuration definitions
set(BOARD_DEFINITIONS
    CLOCK_SPEED_HZ=8192000UL
)

# Architecture / ABI flags (applied to both compile and link)
set(UNICORN_ARCH_FLAGS
    -m68010
    -msoft-float
)

# Common flags 
set(UNICORN_COMMON_FLAGS
    -specs=picolibc.specs
    -ffreestanding
    -fno-builtin
    -fno-omit-frame-pointer
    -fno-delete-null-pointer-checks
    -Wall
    -Wextra
    -std=c23
)

# Default optimisation
set(UNICORN_OPT_FLAGS -O1)

# Join the flags
set(UNICORN_C_FLAGS
    ${UNICORN_ARCH_FLAGS}
    ${UNICORN_COMMON_FLAGS}
    ${UNICORN_OPT_FLAGS}
)

# Create linker flags
set(UNICORN_EXE_LINKER_FLAGS
    ${UNICORN_ARCH_FLAGS}
    -nostartfiles
    -Wl,--build-id=none
    -Wl,--print-memory-usage
)

list(JOIN UNICORN_C_FLAGS " " _unicorn_c_flags_str)
set(CMAKE_C_FLAGS "${_unicorn_c_flags_str}" CACHE STRING "C flags for Unicorn" FORCE)

list(JOIN UNICORN_EXE_LINKER_FLAGS " " _unicorn_link_flags_str)
set(CMAKE_EXE_LINKER_FLAGS "${_unicorn_link_flags_str}" CACHE STRING "Linker flags for Unicorn" FORCE)

# Macros

# Macro to turn an ELF into a raw binary + listing
macro(unicorn_add_bin_target target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
                $<TARGET_FILE:${target}>
                $<TARGET_FILE_DIR:${target}>/${target}.bin
        COMMENT "Generating ${target}.bin"
    )
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJDUMP} -m68010 -a -h -d -S -l -r -C
                $<TARGET_FILE:${target}>
                > $<TARGET_FILE_DIR:${target}>/${target}.lst
        COMMENT "Generating ${target}.lst"
    )
endmacro()

# Macro to set PIE for an executable
macro(unicorn_set_pie target)
    set_target_properties(${target} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
    target_compile_options(${target} PRIVATE -fPIE)
    target_link_options(${target} PRIVATE -pie)
endmacro()