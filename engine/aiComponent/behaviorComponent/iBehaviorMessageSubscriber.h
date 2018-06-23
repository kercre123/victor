/**
* File: iBehaviorMessageSubscriber.h
*
* Author: Kevin M. Karol
* Created: 10/13/17
*
* Description: Decorator interface for behaviors to subscribe to messages
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorComponent_IBehaviorMessageSubscriber_H__
#define __Cozmo_Basestation_BehaviorComponent_IBehaviorMessageSubscriber_H__

#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"

#include <set>

namespace Anki {
namespace Cozmo {

// Forward Declaration
class BehaviorSystemManager;
class IBehavior;

class IBehaviorMessageSubscriber {
public:
  virtual void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageGameToEngineTag>&& tags) const = 0;
  virtual void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageEngineToGameTag>&& tags) const = 0;
  virtual void SubscribeToTags(IBehavior* subscriber, std::set<RobotInterface::RobotToEngineTag>&& tags) const = 0;
  virtual void SubscribeToTags(IBehavior* subscriber, std::set<AppToEngineTag>&& tags) const = 0;
};
  
  


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorComponent_IBehaviorMessageSubscriber_H__
