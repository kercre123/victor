/**
 * File: animationComponent.h
 *
 * Author: Kevin Yoon
 * Created: 2017-08-01
 *
 * Description: Control interface for animation process to manage execution of
 *              canned and idle animations
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Components_AnimationComponent_H__
#define __Cozmo_Basestation_Components_AnimationComponent_H__

#include "anki/common/types.h"
#include "engine/events/ankiEvent.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/signalHolder.h"

#include <unordered_map>

namespace Anki {
namespace Cozmo {

// Forward declarations
class Robot;
class CozmoContext;
  
class AnimationComponent : private Anki::Util::noncopyable, private Util::SignalHolder
{
public:

  AnimationComponent(Robot& robot, const CozmoContext* context);

  void Init();
  void Update();
  
  void DoleAvailableAnimations();
  
  // Returns true when the list of available animations has been received from animation process
  bool IsInitialized() { return _isInitialized; }

  Result PlayAnimByName(const std::string& animName);

  // Event/Message handling
  template<typename T>
  void HandleMessage(const T& msg);
  
  // Robot message handlers
  void HandleAnimationAvailable(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleAnimStarted(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleAnimEnded(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleEndOfMessage(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  
private:
  bool _isInitialized;
  
  Robot& _robot;

  std::unordered_map<std::string, u32> _animNameToID;
  
  bool _isDolingAnims;
  std::string _nextAnimToDole;
  
  Result PlayAnimByID(u32 animID);
};


} // namespace Cozmo
} // namespace Anki

#endif
