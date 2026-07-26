#pragma once

#include "../inc/irq.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  Serial_A,
  Serial_B,
} Serial_Instance_e;

// Assuming Baudrate set 1 (ACR[7] = 0) and 1.8432Mhz clock (twice as slow)
typedef enum {
  Serial_Baudrate_300 = 0b01010101,
  Serial_Baudrate_19200 = 0b11001100,
} Serial_Baudrate_e;

typedef enum {
  Serial_8n = 0b10011,
  Serial_7e = 0b00010,
} Serial_Bitconf_e;

typedef enum {
    Serial_StopBits1 = 0b0111,
    Serial_StopBits2 = 0b1111,
} Serial_StopBits_e;

typedef struct {
  Serial_Instance_e instance;
  Serial_Baudrate_e baudrate;
  Serial_Bitconf_e bit_conf;
  Serial_StopBits_e stop_bits;
  bool rx_rts;
  bool tx_rts;
  bool cts;
} Serial_Settings_t;

void Serial_Init(const Serial_Settings_t *settings);

void Serial_RxIrq(const Serial_Instance_e instance);

void Serial_Putc(const Serial_Instance_e instance, char c);
char Serial_Getc(const Serial_Instance_e instance);
void Serial_PrintStr(const Serial_Instance_e instance, const char *str);