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

#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/debug.h"
#include "testModeController.h"
#include "keyboardController.h"
#include "sim_overlayDisplay.h"
#include "sim_viz.h"
#include "anki/cozmo/robot/hal.h"


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
  
  
#if(FREE_DRIVE_DUBINS_TEST || ENABLE_KEYBOARD_CONTROL)
  Sim::KeyboardController::Enable();
#endif
  
  Sim::OverlayDisplay::Init();
  Sim::Viz::Init();
  
  while(Robot::step_MainExecution() == EXIT_SUCCESS)
  {
    if( Robot::step_LongExecution() == EXIT_FAILURE ) {
      fprintf(stdout, "step_LongExecution failed.\n");
      break;
    }
      
    if(Sim::KeyboardController::IsEnabled() && TestModeController::GetMode() == TestModeController::TM_NONE) {
      Sim::KeyboardController::ProcessKeystroke();
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
