# mc68k SBC - Unicorn v1

## System Overview

- CPU: mc68c010 @ 4Mhz
- RAM: 2x 512k AS6C4008-55PCN SRAM
- ROM: 2x AT28c256 EEPROM
- MMU & glue logic: xc9572xl CPLD
- Peripherals:
  - 68681 DPUART chip - one UART connected via FT231 to USB-C and another connected via MAX232 to DB-9
  - SPI-capable (software) bus from 68681
  - I2C bus using PCF8584
- Power: 5V rail from the USB-C and 3v3 rail derived using ams1117-3.3

## Memory map

A first simple memory map to test the board and get the system running.

0x000000 - 0x00ffff: ROM
0x010000 - 0x03ffff: SRAM (user space)
0x040000 - 0x10ffff: SRAM (kernel space)
0x110000 - 0x110fff: 68681 DPUART (UART1, UART2 and SPI)
0x111000 - 0x111fff: PCF8584 (I2C)

## Glue logic

xc9572xl is connected to all necessary lines to perform the role of a glue logic:

- Address lines:
  - A1 --> PIN1
  - A2 --> PIN2
  - A3 --> PIN4
  - A4 --> PIN5
  - A5 --> PIN6
  - A6 --> PIN7
  - A7 --> PIN8
  - A8 --> PIN9
  - A9 --> PIN10
  - A10 --> PIN11
  - A11 --> PIN12
  - A12 --> PIN13
  - A13 --> PIN15
  - A14 --> PIN16
  - A15 --> PIN17
  - A16 --> PIN18
  - A17 --> PIN19
  - A18 --> PIN20
  - A19 --> PIN22
  - A20 --> PIN23
  - A21 --> PIN24
  - A22 --> PIN25
  - A23 --> PIN27
- CPU signals:
  - /AS --> PIN31
  - /UDS --> PIN32
  - /LDS --> PIN33
  - /DTACK --> PIN34
  - /RW --> PIN35
  - CLK --> PIN36
  - /IPL0 --> PIN38
  - /IPL1 --> PIN39
  - /IPL2 --> PIN40
  - /RES --> PIN42
  - /HALT --> PIN43
  - FC2 --> PIN44
  - FC1 --> PIN45
  - FC0 --> PIN46
- Glue logic signals:
  - /ROM_CS --> PIN47
  - /ROM_LB_OE --> PIN48 (ROM Lower Byte OE)
  - /ROM_UB_OE --> PIN49 (ROM Upper Byte OE)
  - /RAM_CS --> PIN50
  - /RAM_LB_OE --> PIN51 (RAM Lower Byte OE)
  - /RAM_UB_OE --> PIN52 (RAM Upper Byte OE)
  - /RAM_LB_WE --> PIN56
  - /RAM_UB_WE --> PIN57
  - /I2C_CS --> PIN58
  - /I2C_IACK --> PIN59
  - /I2C_IRQ --> PIN60
  - /68681_CS --> PIN61
  - /68681_IACK --> PIN62
  - /68681_IRQ --> PIN63
  - PLD_SIG --> PIN64 (signal to/from the expander)
