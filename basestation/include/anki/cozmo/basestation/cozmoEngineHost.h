/*
 * File:          cozmoEngine.h
 * Date:          12/23/2014
 *
 * Description:   A platform-independent container for spinning up all the pieces
 *                required to run Cozmo on a device. The base class has all the pieces
 *                needed by a device functioning as either host or client. The derived
 *                classes add host- or client-specific functionality.
 *
 *                 - Device Vision Processor (for processing images from the host device's camera)
 *                 - Robot Vision Processor (for processing images from a physical robot's camera)
 *                 - RobotComms
 *                 - GameComms and GameMsgHandler
 *                 - RobotVisionMsgHandler
 *                   - Uses Robot Comms to receive image msgs from robot.
 *                   - Passes images onto Robot Vision Processor
 *                   - Sends processed image markers to the basestation's port on
 *                     which it receives messages from the robot that sent the image.
 *                     In this way, the processed image markers appear to come directly
 *                     from the robot to the basestation.
 *                   - While we only have TCP support on robot, RobotVisionMsgHandler 
 *                     will also forward non-image messages from the robot on to the basestation.
 *                 - DeviceVisionMsgHandler
 *                   - Look into mailbox that Device Vision Processor dumps results 
 *                     into and sends them off to basestation over GameComms.
 *
 * Author: Andrew Stein / Kevin Yoon
 *
 * Modifications:
 */

#ifndef __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineHost_H__
#define __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineHost_H__

#include "anki/cozmo/basestation/cozmoEngine.h"

namespace Anki {
namespace Cozmo {
  
// Derived Host class to run on the host device and deal with advertising and
// world state.
class CozmoEngineHost : public CozmoEngine
{
public:

  CozmoEngineHost();

  virtual bool IsHost() const override { return true; }

  // For adding a real robot to the list of availale ones advertising, using its
  // known IP address. This is only necessary until we have real advertising
  // capability on real robots.
  // TODO: Remove this once we have sorted out the advertising process for real robots
  void ForceAddRobot(AdvertisingRobot robotID,
                     const char*      robotIP,
                     bool             robotIsSimulated);

  void ListenForRobotConnections(bool listen);

  // TODO: Add ability to playback/record

  Robot* GetFirstRobot() override;
  int    GetNumRobots() const;
  Robot* GetRobotByID(const RobotID_t robotID); // returns nullptr for invalid ID
  std::vector<RobotID_t> const& GetRobotIDList() const;

  // Overload to specially handle robot added by ForceAddRobot
  // TODO: Remove once we no longer need forced adds
  virtual bool ConnectToRobot(AdvertisingRobot whichRobot) override;

  // TODO: Promote to base class when we pull robots' visionProcessingThreads out of basestation and distribute across devices
  virtual bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime) override;

protected:
  CozmoEngineHostImpl* _hostImpl;
}; // class CozmoEngineHost
  

} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineHost_H__
