cmake_minimum_required(VERSION 4.4)

set(CMAKE_C_COMPILER m68k-elf-gcc)
set(CMAKE_CXX_COMPILER m68k-elf-g++)
set(CMAKE_ASM_COMPILER m68k-elf-as)
set(CMAKE_AR m68k-elf-ar)
set(CMAKE_RANLIB m68k-elf-ranlib)
set(CMAKE_LINKER "m68k-elf-ld")

project(unicorn_firmware C ASM)

# Set the toolchain
set(CMAKE_SYSTEM_NAME Unicorn)
set(CMAKE_SYSTEM_PROCESSOR m68k)

# Board configuration definitions
set(BOARD_DEFINITIONS
    CLOCK_SPEED_HZ=8192000U
)

# Linker definitions (expanded for ld)
set(LINKER_DEFINITIONS
    -DCLOCK_SPEED_HZ=8192000U
)

# Add C source files
set(C_SOURCES
    bootloader.c
    # syscalls.c
    hal/src/gpio.c
    hal/src/serial.c
    hal/src/duart.c
    hal/src/irq.c
)

# Add ASM source files
set(ASM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/vectors.s
)

# Create object file for assembly sources
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/asm_sources.o
    COMMAND ${CMAKE_ASM_COMPILER} -m68010 ${ASM_SOURCES} -o ${CMAKE_CURRENT_BINARY_DIR}/asm_sources.o
    DEPENDS ${ASM_SOURCES}
)

# set(CMAKE_C_LINK_EXECUTABLE 
#     "${CMAKE_CURRENT_SOURCE_DIR}/m68k-ld-wrapper <LINK_FLAGS> <OBJECTS> -o <TARGET>"
# )

# Create the bootloader executable
add_executable(unicorn_firmware.elf ${C_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/asm_sources.o)

# Add ushell subdirectory to build the library
add_subdirectory(ushell)

# Link the ushell library
target_link_libraries(unicorn_firmware.elf PRIVATE ushell)

# Panic configuration for no_std
target_compile_options(unicorn_firmware.elf PRIVATE
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
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/ushell/inc
)

# Set the linker script + other ld flags
target_link_options(unicorn_firmware.elf PRIVATE
    -m68010
    -specs=picolibc.specs
    -T${CMAKE_CURRENT_SOURCE_DIR}/picolibc_v1.ld
    -nostartfiles
    # -nostdlib
    # -fno-omit-frame-pointer
    -Wl,--build-id=none
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/unicorn_firmware.map
)

# Convert ELF to binary 
add_custom_command(TARGET unicorn_firmware.elf POST_BUILD
    COMMAND m68k-elf-objcopy -O binary $<TARGET_FILE:unicorn_firmware.elf> ${CMAKE_CURRENT_BINARY_DIR}/unicorn_firmware.bin
    COMMENT "Converting ELF to binary"
)

# Split binary into lower and upper
add_custom_command(TARGET unicorn_firmware.elf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Splitting into low/high byte EEPROM images..."
    COMMAND m68k-elf-objcopy --interleave=2 --byte=1 -I binary -O binary 
            ${CMAKE_CURRENT_BINARY_DIR}/unicorn_firmware.bin 
            ${CMAKE_CURRENT_BINARY_DIR}/unicorn_firmware_lower.bin
    COMMAND m68k-elf-objcopy --interleave=2 --byte=0 -I binary -O binary 
            ${CMAKE_CURRENT_BINARY_DIR}/unicorn_firmware.bin 
            ${CMAKE_CURRENT_BINARY_DIR}/unicorn_firmware_upper.bin
    COMMENT "Creating unicorn_firmware_lower.bin and unicorn_firmware_upper.bin"
)

# Generate listing file from ELF
add_custom_command(TARGET unicorn_firmware.elf POST_BUILD
    COMMAND m68k-elf-objdump -m68010 -a -h -d -S -l -r -C $<TARGET_FILE:unicorn_firmware.elf> > ${CMAKE_CURRENT_BINARY_DIR}/unicorn_firmware.lst
    COMMENT "Generating listing file"
)