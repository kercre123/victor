#include <stdint.h>
#include <string.h>

#include "MK02F12810.h"
#include "spine.h"
#include "uart.h"

#include "anki/cozmo/robot/crashLogs.h"

CrashLog_K02* CRASHLOG_POINTER;

extern "C" void HardFault_Handler(void) {
  using namespace Anki::Cozmo::HAL;
  __disable_irq();

  CrashLog_K02 regs;
  memcpy(&regs, CRASHLOG_POINTER, sizeof(regs));
  
  // Note: these values are not stacked
  regs.bfar = SCB->BFAR;
  regs.cfsr = SCB->CFSR;
  regs.hfsr = SCB->HFSR;
  regs.dfsr = SCB->DFSR;
  regs.afsr = SCB->AFSR;
  regs.shcsr = SCB->SHCSR;

  UART::SendCrashLog(&regs, sizeof(regs));
    
  for(;;) {
    UART::Transmit();
  }
}
