/**
* File: main.cpp
*
* Author: Kevin Yoon
* Created: 8/18/2015
*
* Description: Main for running build server tests
*
* Copyright: Anki, inc. 2015
*
*/

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/shared/cozmoTypes.h"

#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/ptree/ptreeTools.h"
#include <stdio.h>
#include <string.h>
#include <fstream>

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"


using namespace Anki;
using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
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
  Data::DataPlatform dataPlatform(filesPath, cachePath, externalPath, resourcePath);

  
  // Create specified test controller
  auto cstCtrl = CozmoSimTestFactory::getInstance()->Create("CST_Animations");
  cstCtrl->SetDataPlatform(&dataPlatform);
  cstCtrl->Init();
  
  
  // Run update loop
  while (cstCtrl->Update() == 0) {}
  
  Anki::Util::gLoggerProvider = nullptr;
  return 0;
}

