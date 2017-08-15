/**
* File: webotsCtrlCozmoSimTest.cpp
*
* Author: Kevin Yoon
* Created: 8/18/2015
*
* Description: Main for running build server tests
*
* Copyright: Anki, inc. 2015
*
*/

#include "../shared/ctrlCommonInitialization.h"
#include <stdio.h>
#include <string.h>
#include "simulator/game/cozmoSimTestController.h"

#if (DO_NOT_QUIT_WEBOTS == 1)
#define QUIT_WEBOTS(status) return status;
#else
#define QUIT_WEBOTS(status) webots::Supervisor dummySupervisor; dummySupervisor.simulationQuit(status);
#endif

using namespace Anki;
using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
  // Note: we don't allow logFiltering in BuildServerTest like we do in the other controllers because this
  // controller is meant to show all logs.

  // create platform
  const Anki::Util::Data::DataPlatform& dataPlatform = WebotsCtrlShared::CreateDataPlatformTest(argv[0], "webotsCtrlBuildServer");
  
  // initialize logger
  const bool filterLog = false;
  WebotsCtrlShared::DefaultAutoGlobalLogger autoLogger(dataPlatform, filterLog);
  
  // Create specified test controller.
  // Only a single argument is supported and it must the name of a valid test.
  if (argc < 2) {
    PRINT_NAMED_ERROR("WebotsCtrlBuildServerTest.main.NoTestSpecified","");
    QUIT_WEBOTS(-1);
  }
  
  std::string testName(argv[1]);
  auto cstCtrl = CozmoSimTestFactory::getInstance()->Create(testName);
  if (nullptr == cstCtrl)
  {
    PRINT_NAMED_ERROR("WebotsCtrlBuildServerTest.main.TestNotFound", "'%s' test not found", testName.c_str());
    QUIT_WEBOTS(-1);
  }
  PRINT_NAMED_INFO("WebotsCtrlBuildServerTest.main.StartingTest", "%s", testName.c_str());
  
  // Init test
  cstCtrl->Init();
  // Run update loop
  while (cstCtrl->Update() == 0) {}

  return 0;
}

