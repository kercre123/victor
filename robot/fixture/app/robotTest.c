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

// Return true if device is detected on contacts
bool RobotDetect(void)
{
  // Turn power on and check for current draw
  PIN_SET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);
  PIN_IN(GPIOC, PINC_CHGRX);
  MicroWait(1000);
  return MonitorGetCurrent() > 20000;
}

void SendTestChar(int c)
{
  u32 start = getMicroCounter();
  
  // Receive mode - TX low is floating
  PIN_RESET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);
  PIN_PULL_UP(GPIOC, PINC_CHGRX);
  PIN_IN(GPIOC, PINC_CHGRX);

  // Wait for RX to go low/be low
  while (GPIO_READ(GPIOC) & GPIOC_CHGRX)
    if (getMicroCounter()-start > 1000000)
      throw ERROR_NO_PULSE;
  // Now wait for it to go high
  while (!(GPIO_READ(GPIOC) & GPIOC_CHGRX))
    if (getMicroCounter()-start > 1000000)
      throw ERROR_NO_PULSE;
    
  // Before we can send, we must drive the signal up via TX, and pull the signal down via RX
  PIN_SET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);
  PIN_PULL_NONE(GPIOC, PINC_CHGRX);
  PIN_RESET(GPIOC, PINC_CHGRX);
  PIN_OUT(GPIOC, PINC_CHGRX);
  
  MicroWait(30);  // Enough time for robot to turn around
  
  // Bit bang the UART, since it's miswired on EP3
  u32 now, last = getMicroCounter();
  c <<= 1;      // Start bit
  c |= (3<<9);  // Stop bits
  for (int i = 0; i < 11; i++)
  {
    while ((u32)((now = getMicroCounter())-last) < 10)
      ;
    last = now;
    if (c & (1<<i))
      PIN_SET(GPIOC, PINC_CHGTX);
    else
      PIN_RESET(GPIOC, PINC_CHGTX);      
  }
  
  // Receive mode - TX low is floating
  PIN_RESET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);
  PIN_PULL_UP(GPIOC, PINC_CHGRX);
  PIN_IN(GPIOC, PINC_CHGRX);
}
void SendTestMode(int test)
{
  SendTestChar('W');
  SendTestChar('t');
  SendTestChar('f');
  SendTestChar(test);
}

void PlaypenTest(void)
{
  SendTestMode(7);    // Lucky 7 is Playpen fixture mode
}

extern int g_stepNumber;
void ChargerTest(void)
{
  int offContact = 0;
  
  // Act like a charger - the robot expects to start on one!
  PIN_SET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);

  while (1)
  {
    MicroWait(50000);
    int current = MonitorGetCurrent();
    ConsolePrintf("%d..", current);
    if (current < 10000)
      offContact++;
    else if (current > 100000)    // Robot is up and running
      offContact = 0;
    else {
    /* This approach to detecting a rebooting robot doesn't work because the current draw is ALL over
      // Robot is rebooting
      while (current < 100000)
        current = MonitorGetCurrent();
      g_stepNumber = -1;   // Post-reboot, restart to first step
      return;
    */
    }
    if (offContact > 10)
      return;
  }
}

// List of all functions invoked by the test, in order
TestFunction* GetRobotTestFunctions(void)
{
  static TestFunction functions[] =
  {
    PlaypenTest,
    ChargerTest,
    NULL
  };

  return functions;
}
