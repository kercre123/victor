#ifndef UART_H
#define UART_H

#include <stdint.h>

#include "hal/portable.h"
#include "anki/cozmo/robot/rec_protocol.h"
#include "anki/cozmo/robot/spineData.h"

static const uint32_t debug_baud_rate = 3000000;

#define BAUD_SBR(baud)  (CORE_CLOCK * 2 / baud / 32)
#define BAUD_BRFA(baud) (CORE_CLOCK * 2 / baud % 32)

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;
extern bool recoveryStateUpdated;

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      void UartInit(void);
      void UartTransmit(void);

      void EnterBodyRecovery(void);
      void SendRecoveryData(const uint8_t* data, int bytes);

      void DebugInit(void);
      void DebugPrintf(const char *format, ...);
      void DebugPutc(char c);
      extern volatile bool HeadDataReceived;
      extern volatile bool RecoveryStateUpdated;
      extern volatile RECOVERY_STATE recoveryMode;
    }
  }
}

#endif
