#include "app/bodyTest.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "app/fixture.h"

// List of all functions invoked by the test, in order
TestFunction* GetBodyTestFunctions(void)
{
  static TestFunction functions[] =
  {
    NULL
  };

  return functions;
}
