#include <stdint.h>
#include <stddef.h>

#include "anki/cozmo/robot/crashLogs.h"

CrashLog_NRF* CRASHLOG_POINTER;

extern "C" {
  #include "nrf.h"
}

#include "storage.h"

extern "C" {
  void HardFault_Handler(void) {
    Storage::set(STORAGE_CRASH_LOG_NRF, CRASHLOG_POINTER, sizeof(CrashLog_NRF));
    
    NVIC_SystemReset();
  }
}
