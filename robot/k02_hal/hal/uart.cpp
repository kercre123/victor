#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

#include "anki/cozmo/robot/spineData.h"
#include "MK02F12810.h"

#include "uart.h"

#include <string.h>
#include <stdint.h>

enum TRANSFER_MODE {
  TRANSMIT_RECEIVE,
  TRANSMIT_SEND
};

static uint8_t txBuffer[sizeof(GlobalDataToBody)];
static uint8_t rxBuffer[sizeof(GlobalDataToHead)];
static int txIndex;
static int rxIndex;

inline void transmit_mode(bool tx);

void Anki::Cozmo::HAL::UartInit() {
  // Enable clocking to the UART and PORTD
  SIM_SOPT5 &= ~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK);
  SIM_SOPT5 |= SIM_SOPT5_UART0TXSRC(0) | SIM_SOPT5_UART0RXSRC(0);

  SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;

  // Enable UART for this shiz

  UART0_BDL = UART_BDL_SBR(BAUD_SBR(spine_baud_rate));
  UART0_BDH = 0;

  UART0_C1 = 0;                                      // 8 bit, 1 bit stop no parity (single wire)
  UART0_S2 |= UART_S2_RXINV_MASK;
  UART0_C3 = UART_C3_TXINV_MASK;
  UART0_C4 = UART_C4_BRFA(BAUD_BRFA(spine_baud_rate));

  UART0_PFIFO = UART_PFIFO_TXFE_MASK | UART_PFIFO_TXFIFOSIZE(6) | UART_PFIFO_RXFE_MASK | UART_PFIFO_RXFIFOSIZE(6) ;
  UART0_CFIFO = UART_CFIFO_TXFLUSH_MASK | UART_CFIFO_RXFLUSH_MASK ;

  txIndex = 0;
  rxIndex = 0;

  g_dataToBody.common.source = (uint32_t)SPI_SOURCE_HEAD;
}

inline void transmit_mode(bool tx) {
  if (tx) {
    PORTD_PCR6 = PORT_PCR_MUX(0);
    PORTD_PCR7 = PORT_PCR_MUX(3);
    UART0_C2 = UART_C2_TE_MASK;
  }else {
    PORTD_PCR6 = PORT_PCR_MUX(3);
    PORTD_PCR7 = PORT_PCR_MUX(0);
    UART0_C2 = UART_C2_RE_MASK;
  }
}

void Anki::Cozmo::HAL::UartTransmit(void) {
  // Dequeue received bytes
  while (UART0_RCFIFO) {
    rxBuffer[rxIndex] = UART0_D;
    
    const uint32_t target = SPI_SOURCE_BODY;
    
    // Re-sync
    if (rxIndex < 4) {
      const uint8_t header = target >> (rxIndex * 8);
      
      if(rxBuffer[rxIndex] != header) {
        rxIndex = 0;
        continue ;
      }
    }
    
    // We received a full packet
    if (++rxIndex >= 64) {
      rxIndex = 0;
      memcpy(&g_dataToHead, rxBuffer, sizeof(GlobalDataToHead));
      DebugPrintf("\r\nUart Rx");
    }
  }

  transmit_mode(true);
  MicroWait(10);

  // Enqueue transmissions
  for (int i = 0; i < uart_chunk_size; i++) {
    if (txIndex == 0) {
      memcpy(txBuffer, &g_dataToBody, sizeof(GlobalDataToBody));
    }
    
    UART0_D = txBuffer[txIndex];
    txIndex = (txIndex + 1) % sizeof(GlobalDataToBody);
  }
}

void Anki::Cozmo::HAL::UartReceive(void) {
  while (~UART0_S1 & UART_S1_TC_MASK ) ;
  transmit_mode(false);
}
