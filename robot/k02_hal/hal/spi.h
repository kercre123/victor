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
        
        void TransmitDrop(void);
        void InitDMA(void);
        void Init(void);
        void StartDMA(void);
      }
    }
  }
}

#endif
