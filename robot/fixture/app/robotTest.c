#include "app/robotTest.h"
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

#define PINC_TX           11
#define GPIOC_TX          (1 << PINC_TX)
#define PINC_RX           10
#define GPIOC_RX          (1 << PINC_RX)

// Return true if device is detected on contacts
bool RobotDetect(void)
{
  static int heardPing = 0;

  u32 start = getMicroCounter();
    
  if (heardPing > 0)
    heardPing--;

  // Drive power on if we haven't heard a ping lately
  if (!heardPing)
  {
    GPIO_SET(GPIOC, PINC_TX);
    PIN_OUT(GPIOC, PINC_TX);
    MicroWait(500);
  }
  
  // Receive mode - TX low is floating
  GPIO_RESET(GPIOC, PINC_TX);
  PIN_OUT(GPIOC, PINC_TX);
  PIN_PULL_UP(GPIOC, PINC_RX);
  PIN_IN(GPIOC, PINC_RX);
  MicroWait(10);
  
  // Wait for RX to go low/be low
  while (GPIO_READ(GPIOC) & GPIOC_RX)
    if (getMicroCounter()-start > 1000)
      return !!heardPing;
  MicroWait(10);
  
  // Now wait for it to go high again
  while (!(GPIO_READ(GPIOC) & GPIOC_RX))
    if (getMicroCounter()-start > 1000)
      return !!heardPing;

  // One ping every 30ms is enough to keep us listening
  heardPing = 30;
  return true;
}

void SendTestChar(int c)
{
  u32 start = getMicroCounter();
  
  // Receive mode - TX low is floating
  GPIO_RESET(GPIOC, PINC_TX);
  PIN_OUT(GPIOC, PINC_TX);
  PIN_PULL_UP(GPIOC, PINC_RX);
  PIN_IN(GPIOC, PINC_RX);

  // Wait for RX to go low/be low
  while (GPIO_READ(GPIOC) & GPIOC_RX)
    if (getMicroCounter()-start > 1000000)
      throw 2;
  // Now wait for it to go high
  while (!(GPIO_READ(GPIOC) & GPIOC_RX))
    if (getMicroCounter()-start > 1000000)
      throw 2;
    
  // Before we can send, we must drive the signal up via TX, and pull the signal down via RX
  GPIO_SET(GPIOC, PINC_TX);
  PIN_OUT(GPIOC, PINC_TX);
  PIN_PULL_NONE(GPIOC, PINC_RX);
  GPIO_RESET(GPIOC, PINC_RX);
  PIN_OUT(GPIOC, PINC_RX);
  
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
      GPIO_SET(GPIOC, PINC_TX);
    else
      GPIO_RESET(GPIOC, PINC_TX);      
  }
  
  // Receive mode - TX low is floating
  GPIO_RESET(GPIOC, PINC_TX);
  PIN_OUT(GPIOC, PINC_TX);
  PIN_PULL_UP(GPIOC, PINC_RX);
  PIN_IN(GPIOC, PINC_RX);
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

void ChargerTest(void)
{
  int offContact = 0;
  
  // Act like a charger - the robot expects to start on one!
  GPIO_SET(GPIOC, PINC_TX);
  PIN_OUT(GPIOC, PINC_TX);

  while (1)
  {
    MicroWait(50000);
    int current = MonitorGetCurrent();
    ConsolePrintf("%d..", current);
    if (current < 20000)
      offContact++;
    else
      offContact = 0;
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
