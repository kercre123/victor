#include "anki/cozmo/robot/hal.h"
#include "movidius.h"

using namespace Anki::Cozmo::HAL;

int main()
{
  if (Init())
  {
    while (TRUE) ;
  }

  UARTInit();
  printf("\nUART Initialized\n");

  FrontCameraInit();
  printf("\nCamera Initialized\n");

  //USBInit();

  while (TRUE)
  {
    u8* image = 0;
    do
    {
      image = FrontCameraGetFrame();
    }
    while (!image);

    UARTPutChar(0xBE);
    UARTPutChar(0xEF);
    UARTPutChar(0xF0);
    UARTPutChar(0xFF);
    UARTPutChar(0xBD);

    for (int y = 0; y < 480; y += 8)
    {
      for (int x = 0; x < 640; x += 8)
      {
        UARTPutChar(image[x + y * 640]);
      }
    }
  }
}

