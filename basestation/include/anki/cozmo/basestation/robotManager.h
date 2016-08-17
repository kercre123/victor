//
//  robotManager.h
//  Products_Cozmo
//
//     RobotManager class for keeping up with available robots, by their ID.
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef ANKI_COZMO_BASESTATION_ROBOTMANAGER_H
#define ANKI_COZMO_BASESTATION_ROBOTMANAGER_H

#include "anki/cozmo/basestation/robotEventHandler.h"
#include "util/signals/simpleSignal.hpp"
#include "util/helpers/noncopyable.h"
#include "clad/types/animationTrigger.h"
#include <map>
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>


namespace Json {
  class Value;
}

namespace Anki {
  
namespace Util {
namespace Data {
  class DataPlatform;
}
}

namespace Cozmo {
  
// Forward declarations:
namespace RobotInterface {
class MessageHandler;
enum class EngineToRobotTag : uint8_t;
enum class RobotToEngineTag : uint8_t;
}
class Robot;
class IExternalInterface;
class CozmoContext;
class CannedAnimationContainer;
class AnimationGroupContainer;
enum class FirmwareType : uint8_t;
class FirmwareUpdater;
class AnimationTriggerResponsesContainer;
class RobotInitialConnection;

class RobotManager : Util::noncopyable
{
public:

  RobotManager(const CozmoContext* context);
  
  ~RobotManager();
  
  void Init(const Json::Value& config);
  
  // Get the list of known robot ID's
  std::vector<RobotID_t> const& GetRobotIDList() const;

  // for when you don't care and you just want a damn robot
  Robot* GetFirstRobot();

  // Get a pointer to a robot by ID
  Robot* GetRobotByID(const RobotID_t robotID);
  
  // Check whether a robot exists
  bool DoesRobotExist(const RobotID_t withID) const;
  
  // Add / remove robots
  void AddRobot(const RobotID_t withID);
  void RemoveRobot(const RobotID_t withID);
  void RemoveRobots();
  
  // Call each Robot's Update() function
  void UpdateAllRobots();
  
  // Update robot connection state
  void UpdateRobotConnection();
  
  // Attempt to begin updating firmware to specified version (return false if it cannot begin)
  bool InitUpdateFirmware(FirmwareType type, int version);
  
  // Update firmware (if appropriate) on every connected robot
  bool UpdateFirmware();
  
  // Return a
  // Return the number of availale robots
  size_t GetNumRobots() const;

  // Events
  using RobotDisconnectedSignal = Signal::Signal<void (RobotID_t)>;
  RobotDisconnectedSignal& OnRobotDisconnected() { return _robotDisconnectedSignal; }

  CannedAnimationContainer& GetCannedAnimations() { return *_cannedAnimations; }
  AnimationGroupContainer& GetAnimationGroups() { return *_animationGroups; }
  
  RobotEventHandler& GetRobotEventHandler() { return _robotEventHandler; }
  
  bool HasCannedAnimation(const std::string& animName);
  bool HasAnimationGroup(const std::string& groupName);
  bool HasAnimationForTrigger( AnimationTrigger ev );
  std::string GetAnimationForTrigger( AnimationTrigger ev );
  
  // Read the animations in a dir
  void ReadAnimationDir();

  // Iterate through the loaded animations and broadcast their names
  void BroadcastAvailableAnimations();
    
  // Iterate through the loaded animation groups and broadcast their names
  void BroadcastAvailableAnimationGroups();
  
  using RobotMap = std::map<RobotID_t,Robot*>;
  const RobotMap& GetRobotMap() const { return _robots; }
  RobotInterface::MessageHandler* GetMsgHandler() const { return _robotMessageHandler.get(); }

  bool ShouldFilterMessage(RobotID_t robotId, RobotInterface::RobotToEngineTag msgType) const;
  bool ShouldFilterMessage(RobotID_t robotId, RobotInterface::EngineToRobotTag msgType) const;

protected:
  RobotDisconnectedSignal _robotDisconnectedSignal;
  RobotMap _robots;
  std::vector<RobotID_t>     _IDs;
  const CozmoContext* _context;
  RobotEventHandler _robotEventHandler;
  CannedAnimationContainer* const _cannedAnimations;
  AnimationGroupContainer* const _animationGroups;
  AnimationTriggerResponsesContainer* const _animationTriggerResponses;
  std::unique_ptr<FirmwareUpdater> _firmwareUpdater;
  std::unique_ptr<RobotInterface::MessageHandler> _robotMessageHandler;
  std::vector<Signal::SmartHandle> _signalHandles;
  std::unordered_map<RobotID_t, RobotInitialConnection> _initialConnections;
  uint32_t _fwVersion;
  uint32_t _fwTime;

private:
  void ParseFirmwareHeader(const Json::Value& header);

}; // class RobotManager
  
} // namespace Cozmo
} // namespace Anki


#endif // ANKI_COZMO_BASESTATION_ROBOTMANAGER_H
