
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"

#include "keyboardController.h"

#include <stdio.h>
#include <string.h>


namespace Anki {
  namespace Cozmo {
    namespace KeyboardController {
      
      //
      // Parameters / Constants
      //
      
      

      static bool isInitialized = false;
      static bool isEnabled = false;


      void Init(webots::Robot &robot)
      {
        
      }
      
      void KeyboardController::Enable(void)
{
  //Update Keyboard every 0.1 seconds
  gCozmoBot.keyboardEnable(TIME_STEP);
  keyboardCtrlEnabled_ = TRUE;
  
  printf("Drive: arrows\n");
  printf("Lift up/down: a/z\n");
  printf("Head up/down: s/x\n");
  printf("Undock: space\n");
}

void DisableKeyboardController(void)
{
  //Disable the keyboard
  gCozmoBot.keyboardDisable();
  keyboardCtrlEnabled_ = FALSE;
}

BOOL IsKeyboardControllerEnabled()
{
  return keyboardCtrlEnabled_;
}


