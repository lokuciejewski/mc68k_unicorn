#pragma once

#include <stdbool.h>

#define INTERRUPT __attribute((interrupt_handler))

#define EXCEPTION_AUTOVECTOR 24
#define EXCEPTION_TRAP 32
#define EXCEPTION_USER 64

#define IRQ_NUM_DUART 3

void panic(const char *err);

void Irq_Init(void);
void Irq_SetInterrupts(bool enabled);
void Irq_SetHandler(unsigned char exception_number,
                    void (*exception_handler)());