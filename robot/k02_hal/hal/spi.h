#ifndef SPI_H
#define SPI_H

#include <stdint.h>

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      void TransmitDrop(const uint8_t* buf, int buflen, int eof);
      void SPIInitDMA(void);
      void SPIInit(void);
    }
  }
}

#endif
