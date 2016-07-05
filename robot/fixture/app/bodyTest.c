#include "app/tests.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/swd.h"

#include "app/fixture.h"
#include "app/binaries.h"

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

int TryMotor(u8 motor, s8 speed)
{
  const int MOTOR_RUNTIME = 100 * 1000;
  int first[4], second[4];
  
  // Get motor up to speed
  SendCommand(0x87, (speed&0xFC) + motor, 0, NULL);
  MicroWait(MOTOR_RUNTIME);
  
  // Get start point
  SendCommand(0x85, 0, sizeof(first), (u8*)first);
  MicroWait(MOTOR_RUNTIME);
  
  // Get end point
  SendCommand(0x85, 0, sizeof(second), (u8*)second);
  
  // Stop motor
  SendCommand(0x87, 0 + motor, 0, NULL);
  int ticks = second[motor] - first[motor];
  
  ConsolePrintf("speedtest,%d,%d,%d\r\n", motor, speed, ticks);
  return ticks;
}

const int THRESH = 1500;
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
}

// List of all functions invoked by the test, in order
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
