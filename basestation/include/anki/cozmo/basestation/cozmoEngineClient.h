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

#ifndef __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineClient_H__
#define __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineClient_H__

#include "anki/cozmo/basestation/cozmoEngine.h"

namespace Anki {
namespace Cozmo {

// Simple derived Client class
class CozmoEngineClient : public CozmoEngine
{
public:

  CozmoEngineClient();

  virtual bool IsHost() const override { return false; }

  // Currently just a stub: can't get a robot's image on a client because all
  // the images are still going to the host device right now. So this will
  // just return false for now.
  virtual bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime) override;

protected:
  CozmoEngineClientImpl* _clientImpl;

}; // class CozmoEngineClient
  
  
} // namespace Cozmo
} // namespace Anki


#endif // __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineClient_H__
