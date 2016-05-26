#include "app/tests.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/cube.h"
#include "hal/monitor.h"
#include "app/fixture.h"
#include "binaries.h"

// Return true if device is detected on contacts
bool CubeDetect(void)
{
  DisableBAT();
  DisableVEXT();
  
  // Set VDD high (probably was already) 
  PIN_SET(GPIOB, PINB_VDD);
  
  // Pull down RESET - max 30K fights a 10K yielding 0.25 - or just barely low
  PIN_IN(GPIOC, PINC_RESET);
  PIN_PULL_DOWN(GPIOC, PINC_RESET);
  
  // Wait for pull-ups to fight it out
  MicroWait(10);
  
  // Return true if reset is pulled up by board
  bool detect = !!(GPIO_READ(GPIOC) & (1 << PINC_RESET));
  
  // Put everything back to normal
  PIN_PULL_NONE(GPIOC, PINC_RESET);
  
  // Wait 1ms in detect
  MicroWait(1000);
  
  return detect;
}

// Connect to and burn the program into the cube or charger
void CubeBurn(void)
{  
  ProgramCubeWithSerial();    // Normal bootloader (or cert firmware in FCC build)
}

// List of all functions invoked by the test, in order
TestFunction* GetCubeTestFunctions(void)
{
  static TestFunction functions[] =
  {
    CubeBurn,
    NULL
  };

  return functions;
}
