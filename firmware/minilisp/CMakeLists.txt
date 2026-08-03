# Minilisp object library
add_library(minilisp OBJECT
    minilisp.c
    src/functions.c
    src/memory.c
    src/primitives.c
    src/parser.c
)

# Set compiler options matching the firmware
target_compile_options(minilisp PRIVATE
    -m68010
    -msoft-float
    -std=c23
    -specs=picolibc.specs
    -ffreestanding
    -fno-builtin
    -O1
    -fno-delete-null-pointer-checks
    -Wall
)

target_include_directories(minilisp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

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
    -Wall
)

target_link_options(unicorn_firmware.elf PRIVATE
    -m68010
    -specs=picolibc.specs
    # -T${CMAKE_CURRENT_SOURCE_DIR}/../picolibc_v1.ld
    -nostartfiles
    # -nostdlib
    # -fno-omit-frame-pointer
    -Wl,--build-id=none
    -Wl,--print-memory-usage
)