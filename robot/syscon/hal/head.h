#ifndef SPI_H
#define SPI_H

#include "portable.h"

#define DISABLE_UART_IRQ      NVIC_DisableIRQ(UART0_IRQn)
#define ENABLE_UART_IRQ       NVIC_EnableIRQ(UART0_IRQn)

namespace Head {
  // Whether we have received any data from the head yet
  extern bool spokenTo;
  extern volatile bool uartIdle;

  // Initialize the SPI peripheral on the designated pins in the source file.
  void init();

  // Transmit dataTX for specified length and receive the same length in dataRX
  void TxRx();
}

#endif
