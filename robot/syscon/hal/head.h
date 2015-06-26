#ifndef SPI_H
#define SPI_H

#include "portable.h"

namespace Head {
  // Whether we have received any data from the head yet
  extern bool spokenTo;

  // Initialize the SPI peripheral on the designated pins in the source file.
  void init();

  // Transmit dataTX for specified length and receive the same length in dataRX
  void TxRx(u16 length, const u8* dataTX, u8* dataRX);
  void TxPacket(u16 length, const u8* dataTX);
  void RxPacket(u16 length, u8* dataRX);
}

#endif
