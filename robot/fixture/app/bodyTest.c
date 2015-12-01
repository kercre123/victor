#include "app/bodyTest.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/swd.h"

#include "app/fixture.h"
#include "app/binaries.h"

#define GPIOA_TRX 2

// Return true if device is detected on contacts
bool BodyDetect(void)
{
  DisableVEXT();  // Make sure power is not applied, as it messes up the detection code below

  // Set up TRX as weakly pulled-up - it will detect as grounded when the board is attached
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = 1 << GPIOA_TRX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  // Wait for pull-up to do its business
  MicroWait(10);
  
  // Return true if TRX is pulled down by body board
  return !(GPIO_READ(GPIOA) & (1 << GPIOA_TRX));
}

// Program code on body
void BodyNRF51(void)
{
  EnableVEXT();   // Turn on external power to the body
  MicroWait(100);
  
  // Try to talk to head on SWD
  SWDInitStub(0x20000000, 0x20001400, g_stubBody, g_stubBodyEnd);

  // Send the bootloader and app
  SWDSend(0x20001000, 0x400, 0x0,    g_BodyBoot, g_BodyBootEnd,   0,    0);   // XXX: No serial number this time
  SWDSend(0x20001000, 0x400, 0x1000, g_Body,     g_BodyEnd,       0,    0);  
  
  DisableVEXT();  // Even on failure, this should happen
}

// List of all functions invoked by the test, in order
TestFunction* GetBodyTestFunctions(void)
{
  static TestFunction functions[] =
  {
    BodyNRF51,
    NULL
  };

  return functions;
}
