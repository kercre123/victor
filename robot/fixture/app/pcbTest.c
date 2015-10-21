#include "app/pcbTest.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "app/fixture.h"

#define GPIOC_NRST      GPIO_Pin_5

extern BOOL g_needsFlash;
extern FixtureType g_fixtureType;


extern void TryToEnterDiagnosticMode(void);
extern void InitSWD(void);
extern error_t SWDTest(void);
void FlashBootLoader(void)
{
  error_t error;  
  InitSWD();
  
  // ENBAT off
  DisableBAT();
  
  error = SWDTest();
  // JTAG is probably already locked, but we'll attempt the vehicle flashing
  if (error != ERROR_PCB_ZERO_UID && error != ERROR_OK)
  {
    throw error;
  }
  
  // Turn on ENBAT
  EnableBAT();
  MicroWait(1000);
  
  // Reset
  GPIO_ResetBits(GPIOC, GPIOC_NRST);
  TestEnableRx();
  MicroWait(80000);
  
  //TestClear();
  
  //TestEnableRx();
  // Stop reset signal
  GPIO_SetBits(GPIOC, GPIOC_NRST);
  
  // Wait for boot message
  
  // This code used to use the testport to catch a sign-on message from the boot loader
  // Then, it would send the rest of the app over the testport/in serial port
  
  throw ERROR_PCB_ENTER_DIAG_MODE;
}

void TestPins(void)
{
  // XXX: This test will be a built-in self-test in Cozmo
  // Each CPU will test its own pins for shorts/opens
}

static TestFunction m_functions[] =
{
  FlashBootLoader,      // This must run first - it leaves the PCB powered on for further tests
  TestPins,
  NULL
};

TestFunction* GetPCBTestFunctions(void)
{
  return m_functions;
}
