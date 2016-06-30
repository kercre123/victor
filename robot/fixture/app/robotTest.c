#include "app/tests.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/swd.h"
#include "hal/monitor.h"

#include "app/fixture.h"
#include "app/binaries.h"

// XXX: Fix this if you ever fix current monitoring units
// Robot is up and running - usually around 48K, rebooting at 32K - what a mess!
#define BOOTED_CURRENT  40000

// Return true if device is detected on contacts
bool RobotDetect(void)
{
  // Turn power on and check for current draw
  PIN_SET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);
  PIN_IN(GPIOC, PINC_CHGRX);
  MicroWait(500);
  return MonitorGetCurrent() > 2000;
}

void SendTestChar(int c);

void SendTestMode(int test)
{
  for (int i = 0; i < 200; i++)
    SendTestChar(-1);   // Give the robot 1 second (200 ticks) to warm up
  
  SendTestChar('W');
  SendTestChar('t');
  SendTestChar('f');
  SendTestChar(test);
}

void InfoTest(void)
{
  // XXX: Sample data so Smerp can start development of their info fixture reader
  ConsolePrintf("version,00000026,00000006,00C0009A,00000001\r\n");
}

void PlaypenTest(void)
{
  SendTestMode(7);    // Lucky 7 is Playpen fixture mode
}

extern int g_stepNumber;
void PlaypenWaitTest(void)
{
  int offContact = 0;
  int restart = 0;
  
  // Act like a charger - the robot expects to start on one!
  PIN_SET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);

  while (1)
  {
    int current = 0;
    for (int i = 0; i < 64; i++)
    {
      MicroWait(750);
      current += MonitorGetCurrent();
    }
    current >>= 6;
    ConsolePrintf("%d..", current);
    if (current < 2000)
      offContact++;
    else if (current < BOOTED_CURRENT)
      restart = 20*4;   // Restart in 4 seconds
    else {
      if (restart) {
        g_stepNumber = -1;
        restart--;      // Each count is about 50ms
        if (!restart)
          return;
      }
      offContact = 0;
    }
    if (offContact > 10)
      return;
  }
}

// List of all functions invoked by the test, in order
TestFunction* GetInfoTestFunctions(void)
{
  static TestFunction functions[] =
  {
    InfoTest,
    NULL
  };

  return functions;
}

// List of all functions invoked by the test, in order
TestFunction* GetPlaypenTestFunctions(void)
{
  static TestFunction functions[] =
  {
    PlaypenTest,
    PlaypenWaitTest,
    NULL
  };

  return functions;
}

// List of all functions invoked by the test, in order
TestFunction* GetRobotTestFunctions(void)
{
  static TestFunction functions[] =
  {
    InfoTest,
    NULL
  };

  return functions;
}
