#ifndef SPI_H
#define SPI_H

#include "portable.h"

// Whether we have received any data from the head yet
extern bool SPISpokenTo;

// Initialize the SPI peripheral on the designated pins in the source file.
void SPIInit();

// Transmit dataTX for specified length and receive the same length in dataRX
void SPITransmitReceive(u16 length, const u8* dataTX, u8* dataRX);

#endif
