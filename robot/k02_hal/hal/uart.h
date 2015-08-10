#ifndef UART_H
#define UART_H

#define BAUD_SBR(baud)  (perf_clock * 2 / baud_rate / 32)
#define BAUD_BRFA(baud) (perf_clock * 2 / baud_rate % 32)

void uart_init(void);

#endif
