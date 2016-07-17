#include "app/tests.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/swd.h"
#include "hal/monitor.h"
#include "hal/radio.h"

#include "app/fixture.h"
#include "app/binaries.h"

#include "../syscon/hal/tests.h"
using namespace Anki::Cozmo::RobotInterface;

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
  return MonitorGetCurrent() > BOOTED_CURRENT;
}

void SendTestChar(int c);

void SendTestMode(int test)
{
  // Pump the comm-link 4 times before trying to send
  for (int i = 0; i < 4; i++)
    try {
      SendTestChar(-1);
    } catch (int e) { }
    
  // Send the message
  SendTestChar('W');
  SendTestChar('t');
  SendTestChar('f');
  SendTestChar(test);
}

void InfoTest(void)
{
  unsigned int version[2];
  
  MicroWait(200000);
  SendTestChar(-1);
  SendCommand(1, 0, 0, 0);    // Put up info face and turn off motors
  SendCommand(0x83, 0, sizeof(version), (u8*)version);
  
  // XXX: Sample data so Smerp can start development of their info fixture reader
  ConsolePrintf("version,%08d,%08d,%08x,00000000\r\n", version[0]>>16, version[0]&0xffff, version[1]);
}

void PlaypenTest(void)
{
  // Try to put robot into playpen mode
  SendTestMode(FTM_PlayPenTest);    
  
  // Do this last:  Try to put fixture radio in advertising mode
  static bool setRadio = false;
  if (!setRadio)
    SetRadioMode('A');
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

void DropSensor(void);
int TryMotor(u8 motor, s8 speed);

// Run motors through their paces
const int SLOW_DRIVE_THRESH = 10, FAST_DRIVE_THRESH = 100;
const int SLOW_LIFT_THRESH = 10, FAST_LIFT_THRESH = 100;
const int SLOW_HEAD_THRESH = 10, FAST_HEAD_THRESH = 100;

void SlowMotors(void)
{
  TryMotor(0, 40);
  TryMotor(0, -40);  
  
  TryMotor(1, 40);
  TryMotor(1, -40);
  
  TryMotor(2, 40);
  TryMotor(2, -40);

  TryMotor(3, 40);
  TryMotor(3, -40);
}

void FastMotors(void)
{
  /*
  int first[4], second[4];
  
  // Turn on motor test
  TryMotor(0, 124);
  
  // Check encoders
  for (int i = 0; i < 2; i++)
  {
    SendCommand(0x85, 0, sizeof(first), (u8*)first);
    MicroWait(1250000);
    SendCommand(0x85, 0, sizeof(second), (u8*)second);
    
    for (int j = 0; j < 4; j++)
      if (first[j] == second[j])
      {
        DisableVEXT();
        MicroWait(1500000);
        throw 300 + j*10;
      }
  }
  
  // Kill robot dead
  DisableVEXT();
  MicroWait(1500000);

  TryMotor(0, -124);  
  
  TryMotor(1, 124);
  TryMotor(1, -124);
  
  TryMotor(2, 124);
  TryMotor(2, -124);

  TryMotor(3, 124);
  TryMotor(3, -124);
  */
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
    SlowMotors,
    FastMotors,
    DropSensor,
    NULL
  };

  return functions;
}
