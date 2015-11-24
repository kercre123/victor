#include "app/bodyTest.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "app/fixture.h"

#define GPIOA_TRX 2

// Return true if device is detected on contacts
bool BodyDetect(void)
{
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
  throw ERROR_BODY_BOOTLOADER;
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
