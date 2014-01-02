/**
 * @file
 * @version		2.2.0.0
 * @date 		2011-May-30
 *
 * Copyright(C) Redpine Signals 2011
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 *
 * @section Description
 * This file contains the function definition prototypes for HAL
 *
 *
 */


#ifndef _RSIHAL_H_
#define _RSIHAL_H_


/**
 * INCLUDES
 */
#include <isaac_registers.h>
#include <swcLeonUtils.h>
#include "rsi_global.h"
#include "DrvIcb.h"

extern u32 rsiSpiBaseAddr;
/**
 * DEFINES
 */

#define RSI_SPI_SEND_BYTE(A)  { \
  *(volatile unsigned char*)(rsiSpiBaseAddr + TRANSMIT_REG) = A; \
}

#define RSI_SPI_SEND_4BYTE(A)  { \
  while(!(*(volatile unsigned*)(rsiSpiBaseAddr + EVENT_REG) & NF)) NOP; \
  *(volatile unsigned*)(rsiSpiBaseAddr + TRANSMIT_REG)=A; \
}

#define RSI_SPI_READ_BYTE(B)  { \
  while(!(*(volatile unsigned*)(rsiSpiBaseAddr + EVENT_REG) & NE)) NOP; \
  B=*(volatile unsigned*)(rsiSpiBaseAddr + RECEIVE_REG) >> 16; \
}

#define RSI_SPI_READ_DUMMY_BYTE(B)  { \
  B=*(volatile unsigned*)(rsiSpiBaseAddr + RECEIVE_REG); \
}


#define RSI_SPI_READ_4BYTE(B)  { \
  while(!(*(volatile unsigned*)(rsiSpiBaseAddr + EVENT_REG) & NE)) NOP; \
  B=*(volatile unsigned*)(rsiSpiBaseAddr + RECEIVE_REG); \
  RSI_DPRINT(RSI_PL1, ":%08X\n", B); \
}

/**
 * Function Prototypes
 */
void rsi_intHandler(void);
void rsi_spiIrqStart(void);
void rsi_spiIrqEnable(void);
void rsi_spiIrqDisable(void);
void rsi_spiIrqClearPending(void);
void rsi_moduleReset(uint8 tf);
void rsi_modulePower(uint8 tf);
int16 rsi_spiSend(uint8 *ptrBuf, uint16 bufLen,uint8 *valBuf,uint8 mode);
int16 rsi_spiSendNoIrq(uint8 *ptrBuf, uint16 bufLen,uint8 *valBuf,uint8 mode);
int16 rsi_spiRecv(uint8 *ptrBuf, uint16 bufLen,uint8 mode);
void rsi_delayMs (uint16 delay);
void rsi_delayUs (uint16 delay);
#endif
