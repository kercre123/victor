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

namespace Anki {
namespace Cozmo {
  

CozmoEngineHost::CozmoEngineHost()
: CozmoEngine(new SimpleExternalInterface())
{
  _hostImpl = new CozmoEngineHostImpl(_externalInterface);
  assert(_hostImpl != nullptr);
  _impl = _hostImpl;
}

CozmoEngineHost::~CozmoEngineHost()
{
  delete _externalInterface;
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

bool CozmoEngineHost::GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime)
{
  return _hostImpl->GetCurrentRobotImage(robotId, img, newerThanTime);
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

} // namespace Cozmo
} // namespace Anki