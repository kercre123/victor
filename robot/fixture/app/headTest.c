#include "app/tests.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/swd.h"
#include "hal/espressif.h"
#include "hal/board.h"

#include "app/binaries.h"
#include "app/fixture.h"
#include "hal/monitor.h"

// Return true if device is detected on contacts
bool HeadDetect(void)
{
  // HORRIBLE PERMANENT HACK TIME - if we leave battery power enabled, the CPU will pull SWD high
  // Main problem is that power is always enabled, not exactly what we want
  EnableBAT();
  
  // First drive SWD low for 1uS to remove any charge from the pin
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = GPIOB_SWD;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  PIN_RESET(GPIOB, PINB_SWD);
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  MicroWait(1);
  
  // Now let it float and see if it ends up high
  PIN_IN(GPIOB, PINB_SWD);
  MicroWait(50);  // Reaches 1.72V after 25uS - so give it 50 just to be safe
   
  // Wait 1ms in detect
  MicroWait(1000);
  
  // True if high
  return !!(GPIO_READ(GPIOB) & GPIOB_SWD);
}

int GetSequence(void);
int serial_;

// Connect to and flash the K02
void HeadK02(void)
{ 
  // Try to talk to head on SWD
  SWDInitStub(0x20000000, 0x20001800, g_stubK02, g_stubK02End);

  serial_ = GetSequence();
  
  // If we get this far, allocate a serial number
  // Send the bootloader and app
  SWDSend(0x20001000, 0x800, 0x0,    g_K02Boot, g_K02BootEnd,   0xFFC,  serial_);   // Put serial number at 0xFFC
  SWDSend(0x20001000, 0x800, 0x1000, g_K02,     g_K02End,       0,      0);
}

// Connect to and flash the Espressif
void HeadESP(void)
{
  // Turn off and let power drain out
  DeinitEspressif();  // XXX - would be better to ensure it was like this up-front
  SWDDeinit();
  DisableBAT();     // This has a built-in delay while battery power leaches out
  /*
  // Let head discharge (this takes a while)
  for (int i = 5; i > 0; i--)
  {
    MicroWait(1000000);
    ConsolePrintf("%d..", i);
  } 
  */
  InitEspressif();
  EnableBAT();

  // Program espressif, which will start up following the program
  ProgramEspressif(serial_);
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
