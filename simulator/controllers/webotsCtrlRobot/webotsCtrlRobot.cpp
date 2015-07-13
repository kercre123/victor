/*
 * File:          webotsCtrlRobot.cpp
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
#include "anki/cozmo/simulator/keyboardController.h"
#include "anki/cozmo/simulator/sim_overlayDisplay.h"
#include "anki/cozmo/simulator/sim_viz.h"
#include "anki/cozmo/robot/hal.h"

// If this is enabled here, it should be disabled in the basestation. (See ENABLE_BS_KEYBOARD_CONTROL.)
#define ENABLE_KEYBOARD_CONTROL 0

/*
 * This is the main program.
 * The arguments of the main function can be specified by the
 * "controllerArgs" field of the Robot node
 */
int main(int argc, char **argv)
{
  using namespace Anki::Cozmo;

  if(Robot::Init() != Anki::RESULT_OK) {
    fprintf(stdout, "Failed to initialize Cozmo::Robot!\n");
    return -1;
  }
  
  
#if(FREE_DRIVE_DUBINS_TEST || ENABLE_KEYBOARD_CONTROL)
  Sim::KeyboardController::Enable();
#endif
  
  Sim::OverlayDisplay::Init();
  Sim::Viz::Init();
  
  while(Robot::step_MainExecution() == Anki::RESULT_OK)
  {
    if( Robot::step_LongExecution() == Anki::RESULT_FAIL ) {
      fprintf(stdout, "step_LongExecution failed.\n");
      break;
    }
      
    if(Sim::KeyboardController::IsEnabled() && TestModeController::GetMode() == TM_NONE) {
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
