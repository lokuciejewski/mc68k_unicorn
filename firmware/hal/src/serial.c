#include "../inc/serial.h"
#include "../inc/duart.h"
#include "../inc/memory.h"

#include <stddef.h>

#define RX_BUFFER_SIZE 256U
#define INVALID_INSTANCE "Invalid serial instance"

typedef struct {
  char rx_buffer[RX_BUFFER_SIZE];
  volatile uint8_t rx_head; // Written by ISR
  volatile uint8_t rx_tail; // Read by main loop
} Serial_RxBuffer_t;

static Serial_RxBuffer_t rx_buffer_a = {0};
static Serial_RxBuffer_t rx_buffer_b = {0};
static uint8_t interrupt_register = 0;

/**
 * Used to avoid clbr since on 68010 it performs implicit read which messes up
 * with the baudrate scaling
 */
static inline void __attribute__((always_inline)) duart_write(uint32_t addr,
                                                              uint8_t val) {
  __asm__ volatile("move.b %0, (%1)" : : "d"(val), "a"(addr) : "memory");
}

typedef struct {
  uint32_t cr;
  uint32_t mr;
  uint32_t csr;
  uint32_t sr;
  uint32_t rb;
  uint32_t tb;
  Serial_RxBuffer_t *rx_buffer;
  uint8_t rx_irq_mask;
} Serial_InstanceData_t;

static void fill_instance_data(const Serial_Instance_e instance,
                               Serial_InstanceData_t *data) {
  switch (instance) {
  case Serial_A:
    data->cr = DUART_CRA;
    data->mr = DUART_MR1A;
    data->csr = DUART_CSRA;
    data->sr = DUART_SRA;
    data->rb = DUART_RBA;
    data->tb = DUART_TBA;
    data->rx_buffer = &rx_buffer_a;
    data->rx_irq_mask = 0b00000010;
    break;
  case Serial_B:
    data->cr = DUART_CRB;
    data->mr = DUART_MR1B;
    data->csr = DUART_CSRB;
    data->sr = DUART_SRB;
    data->rb = DUART_RBB;
    data->tb = DUART_TBB;
    data->rx_buffer = &rx_buffer_b;
    data->rx_irq_mask = 0b00100000;
    break;
  }
}

static inline void enable_interrupt(uint8_t irq_mask) {
  interrupt_register |= irq_mask;
  duart_write(DUART_IMR, interrupt_register);
}

static inline void disable_interrupt(uint8_t irq_mask) {
  interrupt_register &= ~irq_mask;
  duart_write(DUART_IMR, interrupt_register);
}

void Serial_Init(const Serial_Settings_t *settings) {
  duart_write(DUART_IMR, 0x00); // Mask all interrupts
  Serial_InstanceData_t inst = {0};
  fill_instance_data(settings->instance, &inst);
  duart_write(inst.cr, 0b00010000); // Reset MR Pointer
  duart_write(inst.mr,
              ((uint8_t)settings->rx_rts << 7) |
                  settings->bit_conf); // MR1: Rx RTS, parity and bit count
  __asm__ volatile("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;");
  duart_write(inst.mr,
              ((uint8_t)settings->tx_rts << 5) | ((uint8_t)settings->cts << 4) |
                  settings->stop_bits); // MR2: Tx RTS, CTS, stop bits

  duart_write(DUART_ACR,
              0x00); // Baudrate set 1 -> common setting for both UARTs
  duart_write(inst.cr, 0b00100000); // Rx reset
  duart_write(inst.cr, 0b00110000); // Tx reset
  duart_write(inst.csr, settings->baudrate);

  duart_write(inst.cr, 0b01010000); // Reset Break Change Interrupt
  duart_write(inst.cr, 0b01000000); // Clear errors
  duart_write(inst.cr, 0b00000101); // Enable Tx/Rx

  enable_interrupt(inst.rx_irq_mask); // Enable RxRDY interrupt
}

void Serial_RxIrq(const Serial_Instance_e instance) {
  Serial_InstanceData_t inst = {0};
  fill_instance_data(instance, &inst);
  // Process all available bytes and prevent reading an empty hardware FIFO
  while (MEM(inst.sr) & 0x01) {
    uint8_t status = MEM(inst.sr);
    char data = MEM(inst.rb);

    if (status & 0x70) {
      MEM(inst.cr) = 0x40; // Clear error flags
    } else {
      uint8_t next_head = (uint8_t)(inst.rx_buffer->rx_head + 1);
      if (next_head != inst.rx_buffer->rx_tail) {
        inst.rx_buffer->rx_buffer[inst.rx_buffer->rx_head] = data;
        inst.rx_buffer->rx_head = next_head;
      }
    }
  }
}

void Serial_Putc(const Serial_Instance_e instance, char c) {
  Serial_InstanceData_t inst = {0};
  fill_instance_data(instance, &inst);
  while (!(MEM(inst.sr) & 0b0100)) {
  } // wait until TX is ready
  MEM(inst.tb) = c;
}

char Serial_Getc(const Serial_Instance_e instance) {
  Serial_InstanceData_t inst = {0};
  fill_instance_data(instance, &inst);
  while (inst.rx_buffer->rx_head == inst.rx_buffer->rx_tail) {
    // Wait for incoming byte
  }
  char c = inst.rx_buffer->rx_buffer[inst.rx_buffer->rx_tail];
  (inst.rx_buffer->rx_tail)++; // wrap on overflow
  return c;
}

void Serial_PrintStr(const Serial_Instance_e instance, const char *str) {
  if (str == NULL) {
    return;
  }

  Serial_InstanceData_t inst = {0};
  fill_instance_data(instance, &inst);

  while (*str) {
    // *tx_rdy_flag = false;
    while (!(MEM(inst.sr) & 0b0100)) {
    } // wait until TX is ready
    MEM(inst.tb) = *str;
    str++;
  }

  while (!(MEM(inst.sr) & 0b1000)) {
    // Wait for transmitter empty
  }
}
