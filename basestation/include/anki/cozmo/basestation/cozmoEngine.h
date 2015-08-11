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

#ifndef ANKI_COZMO_BASESTATION_COZMO_ENGINE_H
#define ANKI_COZMO_BASESTATION_COZMO_ENGINE_H

#include "anki/cozmo/basestation/comms/robot/robotMessages.h"
#include "util/logging/printfLoggerProvider.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/cameraCalibration.h"
#include "json/json.h"
#include "anki/cozmo/shared/cozmoTypes.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {
  
// Forward declarations
class Robot;
class CozmoEngineImpl;
class CozmoEngineHostImpl;
class CozmoEngineClientImpl;
class IExternalInterface;
template <typename Type>
class AnkiEvent;
namespace ExternalInterface {
class MessageGameToEngine;
}
namespace Data {
class DataPlatform;
}

// Abstract base engine class
class CozmoEngine
{
public:

  CozmoEngine(IExternalInterface* externalInterface, Data::DataPlatform* dataPlatform);
  virtual ~CozmoEngine();

  virtual bool IsHost() const = 0;

  Result Init(const Json::Value& config);

  // Hook this up to whatever is ticking the game "heartbeat"
  Result Update(const float currTime_sec);

  // Provide an image from the device's camera for processing with the engine's
  // DeviceVisionProcessor
  void ProcessDeviceImage(const Vision::Image& image);

  // Removing this now that robot availability emits a signal.
  // Get list of available robots
  //void GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots);

  // The advertising robot could specify more information eventually, but for
  // now, it's just the Robot's ID.
  using AdvertisingRobot = RobotID_t;

  // Request a connection to a specific robot / UI device from the list returned above.
  // Returns true on successful connection, false otherwise.
  // NOTE: This is virtual for now so derived Host can do something different for force-added robots.
  virtual bool ConnectToRobot(AdvertisingRobot whichRobot);

  void StartAnimationTool();
  virtual Robot* GetFirstRobot() { return nullptr; };

  void DisconnectFromRobot(RobotID_t whichRobot);

  // TODO: Add IsConnected methods
  // Check to see if a specified robot / UI device is connected
  // bool IsRobotConnected(AdvertisingRobot whichRobot) const;

  virtual bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime) = 0;

  virtual void SetImageSendMode(RobotID_t robotID, Cozmo::ImageSendMode_t newMode) {}

protected:

  virtual void HandleEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);

  // This will just point at either the host or client impl pointer in a
  // derived class
  CozmoEngineImpl* _impl;
  IExternalInterface* _externalInterface;

  Anki::Util::PrintfLoggerProvider _loggerProvider;
  
  std::vector<::Signal::SmartHandle> _signalHandles;
  
  Data::DataPlatform* _dataPlatform;
}; // class CozmoEngine
  

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_COZMO_ENGINE_H
