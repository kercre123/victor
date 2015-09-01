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

#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include <stdio.h>
#include <string.h>

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"

#if (DO_NOT_QUIT_WEBOTS == 1)
#define QUIT_WEBOTS(status) return status;
#else
#define QUIT_WEBOTS(status) webots::Supervisor dummySupervisor; dummySupervisor.simulationQuit(status);
#endif


using namespace Anki;
using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
  // Setup logger
  Anki::Util::PrintfLoggerProvider loggerProvider;
  loggerProvider.SetMinLogLevel(0);
  Anki::Util::gLoggerProvider = &loggerProvider;
  
  // Get the last position of '/'
  std::string aux(argv[0]);
  size_t pos = aux.rfind('/');
  
  // Get the path and the name
  std::string path = aux.substr(0,pos+1);
  std::string resourcePath = path;
  std::string filesPath = path + "temp";
  std::string cachePath = path + "temp";
  std::string externalPath = path + "temp";
  Util::Data::DataPlatform dataPlatform(filesPath, cachePath, externalPath, resourcePath);

  
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
  cstCtrl->SetDataPlatform(&dataPlatform);
  cstCtrl->Init();
  
  // Run update loop
  while (cstCtrl->Update() == 0) {}
  
  Anki::Util::gLoggerProvider = nullptr;
  return 0;
}

