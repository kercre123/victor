/**
 * File: iBehaviorChooser.cpp
 *
 * Author: Kevin M. Karol
 * Created: 06/24/16
 *
 * Description: Class for handling picking of behaviors.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"

namespace Anki {
namespace Cozmo {

static const char* kSupportsObjectTapInteractionKey = "supportsObjectTapInteractions";

IBehaviorChooser::IBehaviorChooser(Robot& robot, const Json::Value& config)
  :_robot(robot)
{
  if(robot.HasExternalInterface()) {
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestEnabledBehaviorList>();
  }
  
  JsonTools::GetValueOptional(config, kSupportsObjectTapInteractionKey, _supportsObjectTapInteractions);
}
  
template<>
void IBehaviorChooser::HandleMessage(const ExternalInterface::RequestEnabledBehaviorList& msg)
{
  auto behaviorList = GetEnabledBehaviorList();
  
  ExternalInterface::RespondEnabledBehaviorList message(std::move(behaviorList));
  _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
  
}

Util::RandomGenerator& IBehaviorChooser::GetRNG() const
{
  return _robot.GetRNG();
}

  
} // namespace Cozmo
} // namespace Anki
