#include "app/headTest.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/swd.h"
#include "hal/espressif.h"

#include "app/fixture.h"

#define GPIOB_BOOT   0
#define GPIOB_CS     7

// Return true if device is detected on contacts
bool HeadDetect(void)
{
  // XXX: HORRIBLE EP1 HACK TIME - hopefully we have better pins next time
  // This measures the capacitance on the WIFI_BOOT pin - if an Espressif is present, we'll see a load
  /*
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = 1 << GPIOB_BOOT;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  
  PIN_IN(GPIOB, GPIOB_CS);
  
  // Drive low for 1uS to clear any charge
  GPIO_SET(GPIOB, GPIOB_BOOT);
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  MicroWait(3);
  
  PIN_IN(GPIOB, GPIOB_BOOT);
  PIN_PULL_DOWN(GPIOB, GPIOB_BOOT);
  
  u32 start = getMicroCounter(), howlong;
  do
    howlong = getMicroCounter() - start;
  while ((GPIO_READ(GPIOB) & (1 << GPIOB_BOOT)) && howlong < 10000);
  
  SlowPrintf("waited %d\n", howlong);
  */
  return false;
}

// Connect to and flash the K02
void HeadK02(void)
{
  InitSWD();
  
  // ENBAT off
  DisableBAT();

  // Turn on ENBAT
  EnableBAT();
  MicroWait(1000);
  
  throw ERROR_HEAD_APP;
  
  // Reset
  
  //TestClear();
  
  //TestEnableRx();
  // Stop reset signal
  
  // Wait for boot message
  
  // This code used to use the testport to catch a sign-on message from the boot loader
  // Then, it would send the rest of the app over the testport/in serial port
}

// Connect to and flash the Espressif
void HeadESP(void)
{
}

void HeadTest(void)
{
  // XXX: This test will be a built-in self-test in Cozmo
  // Each CPU will test its own pins for shorts/opens
}

TestFunction* GetHeadTestFunctions(void)
{
  static TestFunction functions[] =
  {
    HeadK02,
    HeadESP,
    HeadTest,
    NULL
  };

  return functions;
}
