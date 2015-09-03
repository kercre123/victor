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
  

CozmoEngineHost::CozmoEngineHost(IExternalInterface* externalInterface,
                                 Util::Data::DataPlatform* dataPlatform)
: CozmoEngine(externalInterface, dataPlatform)
{
  _hostImpl = new CozmoEngineHostImpl(_externalInterface, dataPlatform);
  assert(_hostImpl != nullptr);
  _impl = _hostImpl;
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

bool CozmoEngineHost::ConnectToRobot(AdvertisingRobot whichRobot)
{
  return _hostImpl->ConnectToRobot(whichRobot);
}

Robot* CozmoEngineHost::GetFirstRobot() {
  return _hostImpl->GetFirstRobot();
}

int CozmoEngineHost::GetNumRobots() const {
  return _hostImpl->GetNumRobots();
}

Robot* CozmoEngineHost::GetRobotByID(const RobotID_t robotID) {
  return _hostImpl->GetRobotByID(robotID);
}

std::vector<RobotID_t> const& CozmoEngineHost::GetRobotIDList() const {
  return _hostImpl->GetRobotIDList();
}
  

} // namespace Cozmo
} // namespace Anki