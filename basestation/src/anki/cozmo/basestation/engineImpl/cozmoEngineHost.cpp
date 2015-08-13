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

#include "anki/cozmo/basestation/cozmoEngineHost.h"
#include "anki/cozmo/basestation/engineImpl/cozmoEngineHostImpl.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

#include "clad/externalInterface/messageGameToEngine.h"

namespace Anki {
namespace Cozmo {
  

CozmoEngineHost::CozmoEngineHost(IExternalInterface* externalInterface, Data::DataPlatform* dataPlatform)
: CozmoEngine(externalInterface, dataPlatform)
{
  _hostImpl = new CozmoEngineHostImpl(_externalInterface, dataPlatform);
  assert(_hostImpl != nullptr);
  _impl = _hostImpl;
  
  // We'll use this callback for all the events we care about - note this uses polymorphism to HandleEvents with sublcass first
  auto callback = std::bind(&CozmoEngineHost::HandleEvents, this, std::placeholders::_1);
  
  // Subscribe to desired messages
  _signalHandles.push_back(_externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::ForceAddRobot, callback));
}

CozmoEngineHost::~CozmoEngineHost()
{
}

void CozmoEngineHost::ForceAddRobot(AdvertisingRobot robotID,
                                    const char*      robotIP,
                                    bool             robotIsSimulated)
{
  return _hostImpl->ForceAddRobot(robotID, robotIP, robotIsSimulated);
}

void CozmoEngineHost::ListenForRobotConnections(bool listen)
{
  _hostImpl->ListenForRobotConnections(listen);
}

bool CozmoEngineHost::GetCurrentRobotImage(RobotID_t robotID, Vision::Image& img, TimeStamp_t newerThanTime)
{
  return _hostImpl->GetCurrentRobotImage(robotID, img, newerThanTime);
}

void CozmoEngineHost::SetImageSendMode(RobotID_t robotID, Cozmo::ImageSendMode_t newMode)
{
  _hostImpl->SetImageSendMode(robotID, newMode);
}

bool CozmoEngineHost::ConnectToRobot(AdvertisingRobot whichRobot)
{
  return _hostImpl->ConnectToRobot(whichRobot);
}

Robot* CozmoEngineHost::GetFirstRobot() {
  return _hostImpl->GetFirstRobot();
}

int    CozmoEngineHost::GetNumRobots() const {
  return _hostImpl->GetNumRobots();
}

Robot* CozmoEngineHost::GetRobotByID(const RobotID_t robotID) {
  return _hostImpl->GetRobotByID(robotID);
}

std::vector<RobotID_t> const& CozmoEngineHost::GetRobotIDList() const {
  return _hostImpl->GetRobotIDList();
}
  
void CozmoEngineHost::HandleEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  switch (event.GetData().GetTag())
  {
    case ExternalInterface::MessageGameToEngineTag::ForceAddRobot:
    {
      const ExternalInterface::ForceAddRobot& msg = event.GetData().Get_ForceAddRobot();
      char ip[16];
      assert(msg.ipAddress.size() <= 16);
      std::copy(msg.ipAddress.begin(), msg.ipAddress.end(), ip);
      ForceAddRobot(msg.robotID, ip, msg.isSimulated);
      break;
    }
    default:
    {
      CozmoEngine::HandleEvents(event);
    }
  }
}

} // namespace Cozmo
} // namespace Anki