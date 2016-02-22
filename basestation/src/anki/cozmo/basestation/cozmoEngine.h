/*
 * File:          cozmoEngine.h
 * Date:          12/23/2014
 *
 * Description:   A platform-independent container for spinning up all the pieces
 *                required to run Cozmo on a device. We can derive classes to be host or client
 *                for now host/client has been removed until we know more about multi-robot requirements.
 *
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

#include "clad/types/imageTypes.h"
#include "clad/types/engineState.h"
#include "anki/cozmo/basestation/debug/debugConsoleManager.h"

#include <memory>


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
class MultiClientChannel;
class UiMessageHandler;
  
template <typename Type>
class AnkiEvent;

namespace ExternalInterface {
  class MessageGameToEngine;
}

namespace SpeechRecognition {
  class KeyWordRecognizer;
}

class CozmoEngine
{
public:

  CozmoEngine(Util::Data::DataPlatform* dataPlatform);
  virtual ~CozmoEngine();


  Result Init(const Json::Value& config);

  // Hook this up to whatever is ticking the game "heartbeat"
  Result Update(const float currTime_sec);

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
  
  bool                                                      _isInitialized = false;
  bool                                                      _isListeningForRobots = false;
  Json::Value                                               _config;
  std::unique_ptr<MultiClientChannel>                       _robotChannel;
  std::unique_ptr<UiMessageHandler>                         _uiMsgHandler;
  std::unique_ptr<SpeechRecognition::KeyWordRecognizer>     _keywordRecognizer;
  std::unique_ptr<CozmoContext>                             _context;
  std::map<AdvertisingRobot, bool>                          _forceAddedRobots;
  Anki::Cozmo::DebugConsoleManager                          _debugConsoleManager;
  
  virtual Result InitInternal();
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleStartEngine(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void SetEngineState(EngineState newState);
  
  Result AddRobot(RobotID_t robotID);
  
  EngineState _engineState = EngineState::Stopped;
  
}; // class CozmoEngine
  

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_COZMO_ENGINE_H
