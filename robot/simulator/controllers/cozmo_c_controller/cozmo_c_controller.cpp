/*
 * File:          cozmo_c_controller.c
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

/*
 * You may need to add include files like <webots/distance_sensor.h> or
 * <webots/differential_wheels.h>, etc.
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/localization.h"

#include "keyboardController.h"

#include "anki/cozmo/robot/hal.h"

namespace Anki {
  namespace Cozmo {
    extern webots::Supervisor* gCozmoBot;
  }
}

/*
 * You may want to add macros here.
 */

/*
 * This is the main program.
 * The arguments of the main function can be specified by the
 * "controllerArgs" field of the Robot node
 */
int main(int argc, char **argv)
{
  using namespace Anki::Cozmo;

  if(Robot::Init() == EXIT_FAILURE) {
    fprintf(stdout, "Failed to initialize Cozmo::Robot!\n");
    return -1;
  }

  KeyboardController::Init(gCozmoBot);
  
  KeyboardController::Enable();
  
  // Localization::InitLocalization();
  // TODO: Init more things?
  
  while(Robot::step_MainExecution() == EXIT_SUCCESS)
  {
    if( (HAL::GetMicroCounter() % 100000) == 0 ) {
      if( Robot::step_LongExecution() == EXIT_FAILURE ) {
        fprintf(stdout, "step_LongExecution failed.\n");
        break;
      }
    }
      
    if(KeyboardController::IsEnabled()) {
      KeyboardController::ProcessKeystroke();
    }      
  }
  
  /* Enter your cleanup code here */
  //DisableKeyboardController();
  //wb_camera_disable(cam_head);
  //wb_camera_disable(cam_lift);
  //wb_camera_disable(cam_down);
  
  /* This is necessary to cleanup webots resources */
  //wb_robot_cleanup();
  
  return 0;
}
