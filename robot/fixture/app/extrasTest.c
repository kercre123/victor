#include "app/tests.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/cube.h"
#include "hal/monitor.h"
#include "hal/radio.h"
#include "app/app.h"
#include "app/fixture.h"
#include "binaries.h"

static int _whichType = 0;

// Return true if device is detected on contacts
// Since the "finished good" fixture has no contacts, we set to true until first successful run
bool FinishDetect(void)
{
  RadioProcess();
  return _whichType != g_fixtureType;
}

// The actual test runs on the nRF51, so just put the radio in the correct mode for this time
void FinishTest(void)
{
  // Try to set the mode - if that succeeds, we're golden
  if (g_fixtureType == FIXTURE_FINISH_TEST)
    SetRadioMode('C');  // All types
  else
    SetRadioMode('0' + g_fixtureType - FIXTURE_FINISHC_TEST);
  _whichType = g_fixtureType;
}

// List of all functions invoked by the test, in order
TestFunction* GetFinishTestFunctions(void)
{
  static TestFunction functions[] =
  {
    FinishTest,
    NULL
  };

  return functions;
}
