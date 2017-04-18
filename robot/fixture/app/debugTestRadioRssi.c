#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "hal/board.h"
#include "hal/display.h"
#include "hal/flash.h"
#include "hal/monitor.h"
#include "hal/motorled.h"
#include "hal/portable.h"
#include "hal/radio.h"
#include "hal/timers.h"
#include "hal/testport.h"
#include "hal/uart.h"
#include "hal/console.h"
#include "hal/cube.h"
#include "app/fixture.h"
#include "hal/espressif.h"
#include "hal/random.h"

#include "app/app.h"
#include "app/tests.h"
#include "nvReset.h"

static int _whichType = 0;

bool DebugTestDetectDevice(void) {
  RadioProcess();
  return _whichType != g_fixtureType; //true once for each fixture type change (e.g. POR)
}

void DebugRadioModeR(void)
{
  SetRadioMode('R',0);
  _whichType = g_fixtureType; //prevent further 'contact detection'
}

TestFunction* GetDebugTestFunctions()
{
  static TestFunction m_debugFunctions[] = 
  {
    DebugRadioModeR,
    NULL
  };
  return m_debugFunctions;
}

