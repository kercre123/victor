/**
 * File: stateConceptStrategyMessageHelper.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-12-07
 *
 * Description: Object to handle subscribing to messages and calling callbacks properly
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/stateConceptStrategies/stateConceptStrategyMessageHelper.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategyEventHandler.h"
#include "engine/externalInterface/externalInterface.h"
#include "util/signals/simpleSignal.hpp"

namespace Anki {
namespace Cozmo {

StateConceptStrategyMessageHelper::StateConceptStrategyMessageHelper(IStateConceptStrategyEventHandler* handler,
                                                                     BehaviorExternalInterface& bei)
  : _bei(bei)
  , _handler(handler)
{
}
  
StateConceptStrategyMessageHelper::~StateConceptStrategyMessageHelper()
{
}

void StateConceptStrategyMessageHelper::SubscribeToTags(std::set<ExternalInterface::MessageEngineToGameTag>&& tags)
{
  DEV_ASSERT(_bei.GetRobotInfo().HasExternalInterface(),
             "StateConceptStrategyMessageHelper.SubscribeToTags.NoRobotExernalInterface");
  auto* rei = _bei.GetRobotInfo().GetExternalInterface();

  auto handlerCallback = [this](const EngineToGameEvent& event) {
    _handler->HandleEvent(event, _bei);
  };
  
  for(auto tag : tags) {    
    _eventHandles.push_back(rei->Subscribe(tag, handlerCallback));
  }
}

void StateConceptStrategyMessageHelper::SubscribeToTags(std::set<ExternalInterface::MessageGameToEngineTag>&& tags)
{
  DEV_ASSERT(_bei.GetRobotInfo().HasExternalInterface(),
             "StateConceptStrategyMessageHelper.SubscribeToTags.NoRobotExernalInterface");
  auto* rei = _bei.GetRobotInfo().GetExternalInterface();

  auto handlerCallback = [this](const GameToEngineEvent& event) {
    _handler->HandleEvent(event, _bei);
  };
  
  for(auto tag : tags) {    
    _eventHandles.push_back(rei->Subscribe(tag, handlerCallback));
  }
}

}
}
