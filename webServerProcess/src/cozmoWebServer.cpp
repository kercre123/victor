/*
 * File:          cozmoWebServer.cpp
 * Date:          01/30/2018
 *
 * Description:   A platform-independent container for spinning up all the pieces
 *                required to run Cozmo Web Server Process.
 *
 * Author: Paul Terry
 *
 * Modifications:
 */

#include "cozmoWebServer.h"
#include "webService.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "anki/cozmo/shared/cozmoConfig.h"

//#include "osState/osState.h"

#include "util/logging/logging.h"
#include "util/time/universalTime.h"

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>


#if USE_DAS
#include <DAS/DAS.h>
#include <DAS/DASPlatform.h>
#endif

#define LOG_CHANNEL    "CozmoWebServer"

namespace Anki {
namespace Cozmo {

// using namespace WebService;

CozmoWebServerEngine::CozmoWebServerEngine(Util::Data::DataPlatform* dataPlatform)
  : _isInitialized(false)
  , _webService(new Anki::Cozmo::WebService::WebService())
  , _dataPlatform(dataPlatform)
//  , _context(std::make_unique<CozmoAnimContext>(dataPlatform))
{
}

CozmoWebServerEngine::~CozmoWebServerEngine()
{
  _webService.reset();
  _dataPlatform.reset();
}

Result CozmoWebServerEngine::Init() {

  if (_isInitialized) {
    LOG_INFO("CozmoWebServerEngine.Init.ReInit", "Reinitializing already-initialized CozmoWebServerEngine with new config.");
  }
  LOG_INFO("CozmoWebServerEngine.Init.Init", "Initializing embedded web server");

  _webService->Start(_dataPlatform.get(), "8887");

  //OSState::getInstance()->SetUpdatePeriod(1000);

  // RobotDataLoader * dataLoader = _context->GetDataLoader();
  // dataLoader->LoadConfigData();
  // dataLoader->LoadNonConfigData();
  
  // // Set up message handler
  // auto * audioInput = static_cast<Audio::EngineRobotAudioInput*>(audioMux->GetInput(regId));
  // AnimProcessMessages::Init( _animationStreamer.get(), audioInput, _context.get());

  LOG_INFO("CozmoWebServerEngine.Init.Success", "Success");
  _isInitialized = true;

  return RESULT_OK;
}


// PTerry 01/30/2018:  Not even sure if we'll need an Update call for the web server; I think
// the civetweb/webservice stuff just gets launched and runs on its own; but this loop MIGHT
// still be needed for some sort of interprocess communication.

// Update:  Called from cozmoWebServerMain.cpp (android) or webotsCtrlWebserver (mac)
Result CozmoWebServerEngine::Update(BaseStationTime_t currTime_nanosec)
{
  //ANKI_CPU_PROFILE("CozmoWebServerEngine::Update");
  
  if (!_isInitialized) {
    LOG_ERROR("CozmoWebServerEngine.Update", "Cannot update CozmoWebServerEngine before it is initialized.");
    return RESULT_FAIL;
  }

  _webService->Update();

  //BaseStationTimer::getInstance()->UpdateTime(currTime_nanosec);

  //OSState::getInstance()->Update();

  return RESULT_OK;
}


} // namespace Cozmo
} // namespace Anki
