#include "app/headTest.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/swd.h"
#include "hal/espressif.h"

#include "app/fixture.h"

// Connect to and flash the K02
void HeadK02(void)
{
  InitSWD();
  
  // ENBAT off
  DisableBAT();

  // Turn on ENBAT
  EnableBAT();
  MicroWait(1000);
  
  // Reset
  
  //TestClear();
  
  //TestEnableRx();
  // Stop reset signal
  
  // Wait for boot message
  
  // This code used to use the testport to catch a sign-on message from the boot loader
  // Then, it would send the rest of the app over the testport/in serial port
}

// Connect to and flash the Espressif
void HeadESP(void)
{
}

void HeadTest(void)
{
  // XXX: This test will be a built-in self-test in Cozmo
  // Each CPU will test its own pins for shorts/opens
}

TestFunction* GetHeadTestFunctions(void)
{
  static TestFunction functions[] =
  {
    HeadK02,
    HeadESP,
    HeadTest,
    NULL
  };

  return functions;
}
