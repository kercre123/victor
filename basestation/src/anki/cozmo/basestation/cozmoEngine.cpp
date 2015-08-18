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
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

CozmoEngine::CozmoEngine(IExternalInterface* externalInterface, Data::DataPlatform* dataPlatform)
: _impl(nullptr)
, _externalInterface(externalInterface)
, _dataPlatform(dataPlatform)
{
  ASSERT_NAMED(externalInterface != nullptr, "Cozmo.Engine.ExternalInterface.nullptr");
}

CozmoEngine::~CozmoEngine()
{
  if(_impl != nullptr) {
    delete _impl;
    _impl = nullptr;
  }
}

Result CozmoEngine::Init(const Json::Value& config) {
  
  // We'll use this callback for all the events we care about
  auto callback = std::bind(&CozmoEngine::HandleEvents, this, std::placeholders::_1);
  
  // Subscribe to desired messages
  _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::ConnectToRobot, callback));
  _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::ReadAnimationFile, callback));
  
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
  
void CozmoEngine::HandleEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  switch (event.GetData().GetTag())
  {
    case ExternalInterface::MessageGameToEngineTag::ConnectToRobot:
    {
      const ExternalInterface::ConnectToRobot& msg = event.GetData().Get_ConnectToRobot();
      const bool success = ConnectToRobot(msg.robotID);
      if(success) {
        PRINT_NAMED_INFO("CozmoEngine.HandleEvents", "Connected to robot %d!", msg.robotID);
      } else {
        PRINT_NAMED_ERROR("CozmoEngine.HandleEvents", "Failed to connect to robot %d!", msg.robotID);
      }
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::ReadAnimationFile:
    {
      PRINT_NAMED_INFO("CozmoGame.HandleEvents", "started animation tool");
      StartAnimationTool();
      break;
    }
    default:
    {
      PRINT_STREAM_ERROR("CozmoEngine.HandleEvents",
                         "Subscribed to unhandled event of type "
                         << ExternalInterface::MessageGameToEngineTagToString(event.GetData().GetTag()) << "!");
    }
  }
}
  

} // namespace Cozmo
} // namespace Anki