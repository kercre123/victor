/*
 * File:          webotsCtrlRobot2.cpp
 * Date:
 * Description:   Cozmo 2.0 robot process
 * Author:
 * Modifications:
 */

 
#include "../shared/ctrlCommonInitialization.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/simulator/robot/sim_overlayDisplay.h"
#include "anki/cozmo/robot/hal.h"
#include <cstdio>
#include <sstream>

#include <webots/Supervisor.hpp>

/*
 * This is the main program.
 * The arguments of the main function can be specified by the
 * "controllerArgs" field of the Robot node
 */

namespace Anki {
  namespace Cozmo {
    namespace Sim {
      extern webots::Supervisor* CozmoBot;
    }
  }
}


int main(int argc, char **argv)
{
  using namespace Anki;
  using namespace Anki::Cozmo;
  
  
  // parse commands
  WebotsCtrlShared::ParsedCommandLine params = WebotsCtrlShared::ParseCommandLine(argc, argv);
  // create platform
  const Anki::Util::Data::DataPlatform& dataPlatform = WebotsCtrlShared::CreateDataPlatformBS(argv[0], "webotsCtrlRobot2");
  // initialize logger
  WebotsCtrlShared::DefaultAutoGlobalLogger autoLogger(dataPlatform, params.filterLog);

  
  // Get current translation of robot node
  const double* robotPosition = Sim::CozmoBot->getSelf()->getField("translation")->getSFVec3f();
  const double* robotRotation = Sim::CozmoBot->getSelf()->getField("rotation")->getSFRotation();
  
  // Form import string
  const double kApproxCameraHeight = 0.05;
  std::stringstream ss;
  ss << "CozmoEngine2{ filterLogs " << (params.filterLog ? "TRUE " : "FALSE ")
  << "translation " << robotPosition[0] << " " << robotPosition[1] << " " << robotPosition[2] + kApproxCameraHeight << " "
  << "rotation "    << robotRotation[0] << " " << robotRotation[1] << " " << robotRotation[2] << " " << robotRotation[3] << " }";
  
  // Import CozmoEngine2 into scene tree
  webots::Node* root = Sim::CozmoBot->getRoot();
  webots::Field* root_children_field = root->getField("children");
  root_children_field->importMFNodeFromString(0, ss.str() );
  
  
  
  if(Robot::Init() != Anki::RESULT_OK) {
    fprintf(stdout, "Failed to initialize Cozmo::Robot!\n");
    return -1;
  }

  Sim::OverlayDisplay::Init();

  HAL::Step();
  while(Robot::step_MainExecution() == Anki::RESULT_OK)
  {
    HAL::UpdateDisplay();
    HAL::Step();      
  }

  return 0;
}
