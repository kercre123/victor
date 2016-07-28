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
#include <string.h>

#include "../syscon/hal/tests.h"
#include "../syscon/hal/motors.h"

using namespace Anki::Cozmo::RobotInterface;

// Return true if device is detected on contacts
bool BodyDetect(void)
{
  DisableVEXT();  // Make sure power is not applied, as it messes up the detection code below

  // Set up TRX as weakly pulled-up - it will detect as grounded when the board is attached
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = GPIOC_TRX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  // Wait for 1ms (minimum detect time)
  MicroWait(1000);
  
  // Return true if TRX is pulled down by body board
  return !(GPIO_READ(GPIOC) & GPIOC_TRX);
}

// Program code on body
void BodyNRF51(void)
{
  EnableVEXT();   // Turn on external power to the body
  MicroWait(100000);
  
  // Try to talk to head on SWD
  SWDInitStub(0x20000000, 0x20001400, g_stubBody, g_stubBodyEnd);

  // Send the bootloader and app
  SWDSend(0x20001000, 0x400, 0,       g_BodyBLE,  g_BodyBLEEnd,    0,    0,   true);  // Quick check
  SWDSend(0x20001000, 0x400, 0x18000, g_Body,     g_BodyEnd,       0,    0);  
  SWDSend(0x20001000, 0x400, 0x1F000, g_BodyBoot, g_BodyBootEnd,   0x1F014,    0);    // Serial number
 
  DisableVEXT();  // Even on failure, this should happen
}

void SendTestChar(int c);

void HeadlessBoot(void)
{ 
  // Let last step drain out
  DisableVEXT();
  MicroWait(100000);
  
  // Power up with VEXT driven low - tells robot to boot headless
  PIN_RESET(GPIOC, PINC_TRX);
  PIN_OUT(GPIOC, PINC_TRX);
  EnableVEXT();
  MicroWait(350000);        // Around 275ms for robot to recognize fixture
  PIN_IN(GPIOC, PINC_TRX);
    
  // Make sure the robot really booted into test mode
  SendTestChar(-1);  
}

// The real (mass production) numbers are: 8 ticks/revolution, 28.5mm diameter, 172.3:1 
const int MM_PER_TICK_F12 = 0.5 + (4096.0 * 0.125 * 28.5 * 3.14159265359) / 172.3;
const int MDEG_PER_RADIAN = 0.5 + 4096.0 * (180*1000 / (3.14159265359*65536));      // 0.12 const * 16.16 radian -> millidegrees

// The API for this is tangled and gnarly:
// Treads are simple - we just measure motor speed
// To get head and lift limit-to-limit, start a "dummy TryMotor" with -motor, then reverse direction for real read
// Tread speed is returned in mm/sec, head/lift are turned in 1/1000 degree (so 45000 is 45 deg)
int TryMotor(s8 motor, s8 speed, bool limitToLimit = false)
{
  const int MOTOR_RUNTIME = 100 * 1000;   // 100ms is plenty
  static int first[4], second[4];
  bool doPrint = true;
  
  if (motor < 0)
  {
    doPrint = false;
    motor = -motor;
  }
  
  // Reset motor watchdog
  SendCommand(TEST_POWERON, 0, 0, NULL);
  
  // Get motor up to speed (if tread)
  if (motor < 2)
  {
    SendCommand(TEST_RUNMOTOR, (speed&0xFC) + motor, 0, NULL);
    MicroWait(MOTOR_RUNTIME);   // Let tread spinup
    SendCommand(TEST_GETMOTOR, 0, sizeof(first), (u8*)first);
    MicroWait(MOTOR_RUNTIME);   // Then run it
 
  // For lift head/head, let motor -stop- moving first when it hits limit (but keep pushing)
  } else {
    MicroWait(MOTOR_RUNTIME);   // Let fast-spinning motor come to a stop first
    SendCommand(TEST_RUNMOTOR, (speed&0xFC) + motor, 0, NULL);
    if (limitToLimit)
      memcpy(first, second, sizeof(first));   // Copy from previous run (against limit)
    else
      SendCommand(TEST_GETMOTOR, 0, sizeof(first), (u8*)first);
    
    // Run it for a while
    MicroWait(MOTOR_RUNTIME*2);
    if (motor == MOTOR_HEAD)
      MicroWait(MOTOR_RUNTIME*2); // Run the head a little longer - since it's slower moving
  }
  
  // Get end point, then stop motor
  SendCommand(TEST_GETMOTOR, 0, sizeof(second), (u8*)second);
  SendCommand(TEST_RUNMOTOR, 0 + motor, 0, NULL);
  
  int ticks = second[motor] - first[motor];
  if (motor < 2)
    ticks = ((1000000/MOTOR_RUNTIME)*ticks * MM_PER_TICK_F12) >> 12;    // mm/sec for 1/10th sec
  else
    ticks = (ticks * MDEG_PER_RADIAN) >> 12;       // Convert from 16.16 radians to millidegrees
  
  if (doPrint)
    ConsolePrintf("speedtest,%d,%d,%d,%d,%d\r\n", motor+1, speed, ticks, first[motor], second[motor]);
  return ticks;
}

const int THRESH = 975;   // 975mm/sec
void BodyMotor(void)
{
  if (TryMotor(0, 124) < THRESH)
    throw ERROR_MOTOR_LEFT;
  if (TryMotor(0, -124) > -THRESH)
    throw ERROR_MOTOR_LEFT;
    
  if (TryMotor(1, 124) < THRESH)
    throw ERROR_MOTOR_RIGHT;
  if (TryMotor(1, -124) > -THRESH)
    throw ERROR_MOTOR_RIGHT;    
}

void DropSensor(void)
{
  int onoff[2];
  SendCommand(TEST_DROP, 0, sizeof(onoff), (u8*)onoff);
  ConsolePrintf("drop,%d,%d\r\n", onoff[0], onoff[1]);
  //if (onoff[0] < 
}

void DropLeakage(void)
{
  int onoff[2];
  SendCommand(TEST_DROP, 0, sizeof(onoff), (u8*)onoff);
  ConsolePrintf("drop,%d,%d\r\n", onoff[0], onoff[1]);
  if (onoff[0] > 200 || onoff[1] > 200)
    throw ERROR_DROP_LEAKAGE;
}

static void SleepCurrent(void)
{
  try {
    SendCommand(TEST_POWERON, 0x5A, 0, 0);  // Force power off
  } catch (int e) {
    // Duh, can't reply!
  }
  
  EnableBAT();
  DisableVEXT();
  EnableBAT();
  MicroWait(100000);
  int total = 0;
  // Compute microamps by summing milliamps
  for (int i = 0; i < 1000; i++)
    total += BatGetCurrent();
  ConsolePrintf("sleep-current,%d\r\n", total);
  if (total > 200)
    throw ERROR_BAT_LEAKAGE;
  DisableBAT();
}

// List of all functions invoked by the test, in order
TestFunction* GetBody1TestFunctions(void)
{
  static TestFunction functions[] =
  {
    BodyNRF51,
    NULL
  };
  
  return functions;
};
TestFunction* GetBody2TestFunctions(void)
{
  static TestFunction functions[] =
  {
    BodyNRF51,
    HeadlessBoot,
    DropLeakage,
    SleepCurrent,
    NULL
  };
  
  return functions;
};
TestFunction* GetBody3TestFunctions(void)
{
  static TestFunction functions[] =
  {
    BodyNRF51,
    HeadlessBoot,
    BodyMotor,
    DropSensor,
    NULL
  };

  return functions;
}
