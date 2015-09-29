#ifndef UART_H
#define UART_H

#include <stdint.h>

static const uint32_t perf_clock = 96000000;
static const uint32_t debug_baud_rate = 1000000;
static const uint32_t spine_baud_rate = 600000;

#define BAUD_SBR(baud)  (perf_clock * 2 / baud / 32)
#define BAUD_BRFA(baud) (perf_clock * 2 / baud % 32)
#define PRINTF DebugPrintf

void UartInit(void);
void DebugInit(void);

void DebugPrintf(const char *format, ...);

#endif
