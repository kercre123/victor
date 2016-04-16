#include "app/cubeTest.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/cube.h"
#include "hal/monitor.h"
#include "app/fixture.h"

#define GPIOB_VDD   0
#define GPIOC_RESET 5

// Return true if device is detected on contacts
bool CubeDetect(void)
{
  DisableBAT();
  DisableVEXT();
  
  // Set VDD high (probably was already) 
  GPIO_SET(GPIOB, GPIOB_VDD);
  
  // Pull down RESET - max 30K fights a 10K yielding 0.25 - or just barely low
  PIN_IN(GPIOC, GPIOC_RESET);
  PIN_PULL_DOWN(GPIOC, GPIOC_RESET);
  
  // Wait for pull-ups to fight it out
  MicroWait(10);
  
  // Return true if reset is pulled up by board
  bool detect = !!(GPIO_READ(GPIOC) & (1 << GPIOC_RESET));
  
  // Put everything back to normal
  PIN_PULL_NONE(GPIOC, GPIOC_RESET);
  
  return detect;
}

// Connect to and burn the program into the cube or charger
void CubeBurn(void)
{  
  ProgramCube();
}

// List of all functions invoked by the test, in order
TestFunction* GetCubeTestFunctions(void)
{
  static TestFunction functions[] =
  {
    CubeBurn,
    NULL
  };

  return functions;
}
