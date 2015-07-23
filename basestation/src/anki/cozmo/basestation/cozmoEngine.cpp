/*
 * File:          cozmoEngine.cpp
 * Date:          12/23/2014
 *
 * Description:   (See header file.)
 *
 * Author: Andrew Stein / Kevin Yoon
 *
 * Modifications:
 */

#include "anki/cozmo/basestation/cozmoEngine.h"
#include "anki/cozmo/basestation/engineImpl/cozmoEngineImpl.h"


namespace Anki {
namespace Cozmo {

CozmoEngine::CozmoEngine(IExternalInterface* externalInterface)
: _impl(nullptr)
, _externalInterface(externalInterface)
, _loggerProvider()
{
  _loggerProvider.SetMinLogLevel(0);
  if (Anki::Util::gLoggerProvider == nullptr) {
    Anki::Util::gLoggerProvider = &_loggerProvider;
  }
  ASSERT_NAMED(externalInterface != nullptr, "Cozmo.Engine.ExternalInterface.nullptr");
}

CozmoEngine::~CozmoEngine()
{
  if (Anki::Util::gLoggerProvider == &_loggerProvider) {
    Anki::Util::gLoggerProvider = nullptr;
  }

  if(_impl != nullptr) {
    delete _impl;
    _impl = nullptr;
  }
}

Result CozmoEngine::Init(const Json::Value& config) {
  return _impl->Init(config);
}

Result CozmoEngine::Update(const float currTime_sec) {
  return _impl->Update(SEC_TO_NANOS(currTime_sec));
}

/*
void CozmoEngine::GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots) {
  _impl->GetAdvertisingRobots(advertisingRobots);
}
 */

bool CozmoEngine::ConnectToRobot(AdvertisingRobot whichRobot) {
  return _impl->ConnectToRobot(whichRobot);
}

void CozmoEngine::DisconnectFromRobot(RobotID_t whichRobot) {
  _impl->DisconnectFromRobot(whichRobot);
}

void CozmoEngine::ProcessDeviceImage(const Vision::Image &image) {
  _impl->ProcessDeviceImage(image);
}

void CozmoEngine::StartAnimationTool() {
  _impl->StartAnimationTool();
}
  

} // namespace Cozmo
} // namespace Anki