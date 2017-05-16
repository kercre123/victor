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
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"

#include "anki/messaging/basestation/IComms.h"

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
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestChooserBehaviorList>();
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
std::vector<std::string> IBehaviorChooser::GetChooserBehaviors()
{
  std::vector<std::string> behaviorList;
  for(const auto& entry : _robot.GetBehaviorFactory().GetBehaviorMap()){
    behaviorList.push_back(entry.first);
  }
  return behaviorList;
}

template<>
void IBehaviorChooser::HandleMessage(const ExternalInterface::RequestChooserBehaviorList& msg)
{
  const char* chooserName = EnumToString(msg.behaviorChooserType);
  if (chooserName == GetName())
  {
    auto behaviorMap = GetChooserBehaviors();
    
    // Break behaviorMap up into multiple message since it is a vector of strings
    // and the max message size is MsgPacket::MAX_SIZE (2048)
    auto iter = behaviorMap.begin();
    std::vector<std::string> behaviorNames;
    u32 byteCount = 0;
    const u32 kMaxMessageSize = Comms::MsgPacket::MAX_SIZE;
    while(iter != behaviorMap.end())
    {
      const size_t size = iter->size();
      if(byteCount + size > kMaxMessageSize)
      {
        ExternalInterface::RespondChooserBehaviorList message(std::move(behaviorNames), false);
        _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
        
        // Safe to call after move since clear() has no preconditions
        // Also puts temp back to a valid state since move leaves it in an
        // invalid state
        behaviorNames.clear();
        byteCount = 0;
      }
      else
      {
        behaviorNames.push_back(*iter);
        byteCount += size + 1; // Plus one for one byte for string length
        ++iter;
      }
    }
    
    if(!behaviorNames.empty())
    {
      ExternalInterface::RespondChooserBehaviorList message(std::move(behaviorNames), true);
      _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
    }
    else
    {
      // We should always have one final message to send after exiting the above while loop
      PRINT_NAMED_WARNING("IBehaviorChooser.HandleRequestChooserBehaviorList.NotSendingFinalMessage", "");
    }
  }
}

Util::RandomGenerator& IBehaviorChooser::GetRNG() const
{
  return _robot.GetRNG();
}

  
} // namespace Cozmo
} // namespace Anki
