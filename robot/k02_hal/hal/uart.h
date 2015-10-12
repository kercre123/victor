#ifndef UART_H
#define UART_H

#include <stdint.h>

#include "hal/portable.h"
#include "anki/cozmo/robot/spineData.h"

static const uint32_t debug_baud_rate = 1000000;

#define BAUD_SBR(baud)  (CORE_CLOCK * 2 / baud / 32)
#define BAUD_BRFA(baud) (CORE_CLOCK * 2 / baud % 32)

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      void UartInit(void);
      void UartTransmit(void);
      void UartReceive(void);
      
      void DebugInit(void);
      void DebugPrintf(const char *format, ...);
    }
  }
}

#endif
