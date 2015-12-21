#ifndef SPI_H
#define SPI_H

#include <stdint.h>

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      namespace SPI
      {
        void EnterRecoveryMode(void);
        
        void TransmitDrop(const uint8_t* buf, int buflen, int eof);
        void InitDMA(void);
        void Init(void);
        void StartDMA(void);
      }
    }
  }
}

#endif
