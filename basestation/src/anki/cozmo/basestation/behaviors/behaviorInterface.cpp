/**
 * File: behaviorInterface.cpp
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Defines interface for a Cozmo "Behavior".
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

namespace Anki {
namespace Cozmo {

  // Static initializations
  const ActionList::SlotHandle IBehavior::sActionSlot = 0;
  Util::RandomGenerator IBehavior::sRNG;
  
  IBehavior::IBehavior(Robot& robot, const Json::Value& config)
  : _robot(robot)
  , _isRunning(false)
  {
  
  }
  
  void IBehavior::SubscribeToTags(std::vector<GameToEngineTag> &&tags)
  {
    if(_robot.HasExternalInterface()) {
      auto handlerCallback = [this](const GameToEngineEvent& event) {
        this->HandleEvent(event);
      };
      
      for(auto tag : tags) {
        _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(tag, handlerCallback));
      }
    }
  }
  
  void IBehavior::SubscribeToTags(std::vector<EngineToGameTag> &&tags)
  {
    if(_robot.HasExternalInterface()) {
      auto handlerCallback = [this](const EngineToGameEvent& event) {
        this->HandleEvent(event);
      };
      
      for(auto tag : tags) {
        _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(tag, handlerCallback));
      }
    }
  }

  
} // namespace Cozmo
} // namespace Anki