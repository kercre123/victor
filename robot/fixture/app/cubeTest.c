#include "app/cubeTest.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/cube.h"
#include "app/fixture.h"

#define GPIOA_SCK  5
#define GPIOA_MOSI 7
#define GPIOB_CS   7

// Return true if device is detected on contacts
bool CubeDetect(void)
{
  // Set CS high (probably was already) and set SCK as pull-down input
  // The IR LED on the charger and cube will pull SCK high if the board is present
  GPIO_SET(GPIOB, GPIOB_CS);
  
  PIN_IN(GPIOA, GPIOA_SCK);
  PIN_PULL_DOWN(GPIOA, GPIOA_SCK);
  
  // Must float MOSI since it shares a leg with the LED we're trying to light
  PIN_IN(GPIOA, GPIOA_MOSI);
  PIN_PULL_NONE(GPIOA, GPIOA_MOSI);
  
  // Wait for LED to do its business
  MicroWait(10);
  
  // Return true if SCK is pulled up by board
  bool detect = !!(GPIO_READ(GPIOA) & (1 << GPIOA_SCK));
  
  // Put everything back to normal
  PIN_PULL_NONE(GPIOA, GPIOA_SCK);  
  PIN_AF(GPIOA, GPIOA_SCK);
  PIN_AF(GPIOA, GPIOA_MOSI);
  
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
