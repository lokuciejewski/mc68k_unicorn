cmake_minimum_required(VERSION 4.4)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/unicorn_toolchain.cmake")

project(unicorn_firmware C ASM)

# Add C source files
set(C_SOURCES
    bootloader.c
    hal/src/gpio.c
    hal/src/serial.c
    hal/src/duart.c
    hal/src/irq.c
    hal/src/spi.c
    hal/src/i2c.c
    hal/src/time.c

    system/system.c

    drivers/sd_card/sd_card.c

    middleware/tinylisp/tinylisp.c
    middleware/fatfs/ff.c
    middleware/fatfs/ffsystem.c
    middleware/fatfs/ffunicode.c
    middleware/fatfs/diskio.c
)

# Add ASM source files
set(ASM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/system/vectors.s
    ${CMAKE_CURRENT_SOURCE_DIR}/system/trap_handler.s
)

# Create object file for assembly sources
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/asm_sources.o
    COMMAND ${CMAKE_ASM_COMPILER} -m68010 ${ASM_SOURCES} -o ${CMAKE_CURRENT_BINARY_DIR}/asm_sources.o
    DEPENDS ${ASM_SOURCES}
)

# Create the bootloader executable
add_executable(unicorn_firmware ${C_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/asm_sources.o)

target_compile_definitions(unicorn_firmware PRIVATE
    ${BOARD_DEFINITIONS}
)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/system
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/sd_card/api
    ${CMAKE_CURRENT_SOURCE_DIR}/middleware/fatfs/api
    ${CMAKE_CURRENT_SOURCE_DIR}/middleware/tinylisp
)

# Set the linker script
target_link_options(unicorn_firmware PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/unicorn_fw_v1.ld
)


unicorn_add_bin_target(unicorn_firmware)
unicorn_add_listing(unicorn_firmware)

# Split binary into lower and upper
add_custom_command(TARGET unicorn_firmware POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Splitting into low/high byte EEPROM images..."
    COMMAND m68k-elf-objcopy --interleave=2 --byte=1 -I binary -O binary 
            ${CMAKE_CURRENT_BINARY_DIR}/unicorn_firmware.bin 
            ${CMAKE_CURRENT_BINARY_DIR}/unicorn_firmware_lower.bin
    COMMAND m68k-elf-objcopy --interleave=2 --byte=0 -I binary -O binary 
            ${CMAKE_CURRENT_BINARY_DIR}/unicorn_firmware.bin 
            ${CMAKE_CURRENT_BINARY_DIR}/unicorn_firmware_upper.bin
    COMMENT "Creating unicorn_firmware_lower.bin and unicorn_firmware_upper.bin"
)
