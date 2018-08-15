/**
 * File: beiConditionMessageHelper.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-12-07
 *
 * Description: Object to handle subscribing to messages and calling callbacks properly
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/beiConditions/beiConditionMessageHelper.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"
#include "engine/externalInterface/externalInterface.h"
#include "util/signals/simpleSignal.hpp"

namespace Anki {
namespace Vector {

BEIConditionMessageHelper::BEIConditionMessageHelper(IBEIConditionEventHandler* handler,
                                                                     BehaviorExternalInterface& bei)
  : _bei(bei)
  , _handler(handler)
{
}
  
BEIConditionMessageHelper::~BEIConditionMessageHelper()
{
}

void BEIConditionMessageHelper::SubscribeToTags(std::set<ExternalInterface::MessageEngineToGameTag>&& tags)
{
  if( ! _bei.GetRobotInfo().HasExternalInterface() ) {
    PRINT_NAMED_WARNING("BEIConditionMessageHelper.SubscribeToTags.NoRobotExternalInterface",
                        "Can't subscribe to messages because robot has no external interface");
    return;
  }

  auto* rei = _bei.GetRobotInfo().GetExternalInterface();

  auto handlerCallback = [this](const EngineToGameEvent& event) {
    _handler->HandleEvent(event, _bei);
  };
  
  for(auto tag : tags) {    
    _eventHandles.push_back(rei->Subscribe(tag, handlerCallback));
  }
}

void BEIConditionMessageHelper::SubscribeToTags(std::set<ExternalInterface::MessageGameToEngineTag>&& tags)
{
  if( ! _bei.GetRobotInfo().HasExternalInterface() ) {
    PRINT_NAMED_WARNING("BEIConditionMessageHelper.SubscribeToTags.NoRobotExternalInterface",
                        "Can't subscribe to messages because robot has no external interface");
    return;
  }

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
