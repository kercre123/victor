/*
 * File:          cozmoSimTestController.h
 * Date:
 * Description:   Any UI/Game to be run as a Webots controller should be derived from this class.
 * Author:
 * Modifications:
 */

#ifndef __COZMO_SIM_TEST_CONTROLLER__H__
#define __COZMO_SIM_TEST_CONTROLLER__H__


#include "anki/cozmo/simulator/game/uiGameController.h"

// For build server tests
#include <string>
#include <queue>
#include "json/json.h"


namespace Anki {
namespace Cozmo {

class CozmoSimTestController : public UiGameController {

public:
  CozmoSimTestController(s32 step_time_ms);
  virtual ~CozmoSimTestController();
  
protected:
  
  //virtual void InitInternal() {}
  virtual s32 UpdateInternal() = 0;

  
}; // class CozmoSimTestController
  
  
  
// Macros for tests
  
#define COZMO_SIM_TEST_CLASS BuildServerTestController

#define RUN_BUILD_SERVER_TEST \
using namespace Anki; \
using namespace Anki::Cozmo; \
\
int main(int argc, char **argv) \
{ \
  Anki::Cozmo::BuildServerTestController buildServerTestCtrl(BS_TIME_STEP); \
  \
  Anki::Util::PrintfLoggerProvider loggerProvider; \
  loggerProvider.SetMinLogLevel(0); \
  Anki::Util::gLoggerProvider = &loggerProvider; \
  \
  /* Get the last position of '/' */ \
  std::string aux(argv[0]); \
  size_t pos = aux.rfind('/'); \
  \
  /* Get the path and the name */ \
  std::string path = aux.substr(0,pos+1); \
  std::string resourcePath = path; \
  std::string filesPath = path + "temp"; \
  std::string cachePath = path + "temp"; \
  std::string externalPath = path + "temp"; \
  Data::DataPlatform dataPlatform(filesPath, cachePath, externalPath, resourcePath); \
  \
  buildServerTestCtrl.SetDataPlatform(&dataPlatform); \
  buildServerTestCtrl.Init(); \
  \
  while (buildServerTestCtrl.Update() == 0) {}\
  \
  Anki::Util::gLoggerProvider = nullptr; \
  return 0; \
}

  
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __COZMO_SIM_TEST_CONTROLLER__H__


