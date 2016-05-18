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
        void EnterOTAMode(void);
        void EnterRecoveryMode(void);
        
        void InitDMA(void);
        void Init(void);
        void StartDMA(void);

        void ManageDrop(void);
          
        // Finalize the drop for transmission next time
        void FinalizeDrop(int jpeglen, const bool eof, const uint32_t frameNumber);
      }
    }
  }
}

#endif
