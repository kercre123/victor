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
 
#include "../shared/ctrlCommonInitialization.h"
#include "testModeController.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/simulator/robot/sim_overlayDisplay.h"
#include "anki/cozmo/robot/hal.h"
#include <cstdio>

/*
 * This is the main program.
 * The arguments of the main function can be specified by the
 * "controllerArgs" field of the Robot node
 */
int main(int argc, char **argv)
{
  using namespace Anki;
  using namespace Anki::Cozmo;
  
  // parse commands
  WebotsCtrlShared::ParsedCommandLine params = WebotsCtrlShared::ParseCommandLine(argc, argv);
  // create platform
  const Anki::Util::Data::DataPlatform& dataPlatform = WebotsCtrlShared::CreateDataPlatformBS(argv[0], "webotsCtrlRobot");
  // initialize logger
  WebotsCtrlShared::DefaultAutoGlobalLogger autoLogger(dataPlatform, params.filterLog);

  if(Robot::Init() != Anki::RESULT_OK) {
    fprintf(stdout, "Failed to initialize Cozmo::Robot!\n");
    return -1;
  }

  Sim::OverlayDisplay::Init();

  HAL::Step();
  while(Robot::step_MainExecution() == Anki::RESULT_OK)
  {
    HAL::UpdateDisplay();
    if( Robot::step_LongExecution() == Anki::RESULT_FAIL ) {
      fprintf(stdout, "step_LongExecution failed.\n");
      break;
    }

    HAL::Step();      
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
