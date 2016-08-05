#ifndef UART_H
#define UART_H

#include <stdint.h>

#include "hal/portable.h"
#include "anki/cozmo/robot/rec_protocol.h"
#include "anki/cozmo/robot/spineData.h"

static const uint32_t debug_baud_rate = 3000000;

#ifdef ENABLE_FCC_TEST
static const int UART_CORE_CLOCK = 32768*2560;
#else
static const int UART_CORE_CLOCK = CORE_CLOCK;
#endif

#define BAUD_SBR(baud)  (UART_CORE_CLOCK * 2 / (baud) / 32)
#define BAUD_BRFA(baud) (UART_CORE_CLOCK * 2 / (baud) % 32)

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;
extern bool recoveryStateUpdated;

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      namespace UART
      {
        void Init(void);
        void Transmit(void);

        void SendRecoveryData(const uint8_t* data, int bytes);
        void SendCrashLog(const void* data, int bytes);
        
        void DebugInit(void);
        void DebugPrintf(const char *format, ...);
        void DebugPrintProfile(int value);
        void DebugPutc(char c);
        bool FoundSync();
        void WaitForSync();
        void pause(void);
        
        extern volatile bool HeadDataReceived;
        extern volatile uint16_t RecoveryStateUpdated;
        extern volatile RECOVERY_STATE recoveryMode;
      }
    }
  }
}

#endif
