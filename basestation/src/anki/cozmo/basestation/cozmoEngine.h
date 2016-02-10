/*
 * File:          cozmoEngine.h
 * Date:          12/23/2014
 *
 * Description:   A platform-independent container for spinning up all the pieces
 *                required to run Cozmo on a device. We can derive classes to be host or client
 *                for now host/client has been removed until we know more about multi-robot requirements.
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

#include "util/logging/multiFormattedLoggerProvider.h"
#include "anki/vision/basestation/image.h"
#include "json/json.h"
#include "util/signals/simpleSignal_fwd.h"
#include "anki/cozmo/basestation/multiClientChannel.h"

#include "clad/types/imageTypes.h"
#include "anki/cozmo/basestation/debug/debugConsoleManager.h"
#include "anki/cozmo/basestation/robotManager.h"


#define DEVICE_VISION_MODE_OFF   0
#define DEVICE_VISION_MODE_SYNC  1
#define DEVICE_VISION_MODE_ASYNC 2

#define DEVICE_VISION_MOIDE DEVICE_VISION_MODE_OFF

namespace Anki {
  
  // Forward declaration:
  namespace Util {
  namespace Data {
    class DataPlatform;
  }
  }
  
namespace Cozmo {
  
// Forward declarations
class Robot;
class IExternalInterface;
class CozmoContext;
  
template <typename Type>
class AnkiEvent;

namespace ExternalInterface {
  class MessageGameToEngine;
}

namespace SpeechRecognition {
  class KeyWordRecognizer;
}

// Abstract base engine class
class CozmoEngine
{
public:

  CozmoEngine(IExternalInterface* externalInterface, Util::Data::DataPlatform* dataPlatform);
  virtual ~CozmoEngine();


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


  virtual void ReadAnimationsFromDisk();

  void DisconnectFromRobot(RobotID_t whichRobot);

  // TODO: Add IsConnected methods
  // Check to see if a specified robot / UI device is connected
  // bool IsRobotConnected(AdvertisingRobot whichRobot) const;

  virtual bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime);
  
  // For adding a real robot to the list of availale ones advertising, using its
  // known IP address. This is only necessary until we have real advertising
  // capability on real robots.
  // TODO: Remove this once we have sorted out the advertising process for real robots
  void ForceAddRobot(AdvertisingRobot robotID,
                     const char*      robotIP,
                     bool             robotIsSimulated);
  
  void ListenForRobotConnections(bool listen);
  
  Robot* GetFirstRobot();
  int    GetNumRobots() const;
  Robot* GetRobotByID(const RobotID_t robotID); // returns nullptr for invalid ID
  std::vector<RobotID_t> const& GetRobotIDList() const;
  
  virtual bool ConnectToRobot(AdvertisingRobot whichRobot);
  
  void SetImageSendMode(RobotID_t robotID, ImageSendMode newMode);
  void SetRobotImageSendMode(RobotID_t robotID, ImageSendMode newMode, ImageResolution resolution);

protected:

  Anki::Util::MultiFormattedLoggerProvider _loggerProvider;
  
  std::vector<::Signal::SmartHandle> _signalHandles;
  
  bool                      _isInitialized;
  Json::Value               _config;
  MultiClientChannel        _robotChannel;
  
# if DEVISION_VISION_MODE != DEVICE_VISION_MODE_OFF
  VisionProcessingThread    _deviceVisionThread;
# endif
  
  virtual Result InitInternal();
  virtual Result UpdateInternal(const BaseStationTime_t currTime_ns);
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
  Result AddRobot(RobotID_t robotID);
  
  bool                         _isListeningForRobots;
  SpeechRecognition::KeyWordRecognizer* _keywordRecognizer;
  
  std::map<AdvertisingRobot, bool> _forceAddedRobots;
  
  Anki::Cozmo::DebugConsoleManager _debugConsoleManager;
  
  std::unique_ptr<CozmoContext>    _context;
  
}; // class CozmoEngine
  

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_COZMO_ENGINE_H
