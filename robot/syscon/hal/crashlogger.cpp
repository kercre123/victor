#include <stdint.h>
#include <stddef.h>

#include "anki/cozmo/robot/crashLogs.h"

extern "C" {
  #include "nrf.h"
}

#include "storage.h"

extern "C" {
  void HardFault_Handler(CrashLog_NRF* crashlog) {
    Storage::set(STORAGE_CRASH_LOG_NRF, crashlog, sizeof(CrashLog_NRF));
    
    NVIC_SystemReset();
  }
}
