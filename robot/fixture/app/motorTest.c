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
bool MotorDetect(void)
{
  return 0;
}

// List of all functions invoked by the test, in order
TestFunction* GetMotorTestFunctions(void)
{
  static TestFunction functions[] =
  {
    NULL
  };

  return functions;
}
