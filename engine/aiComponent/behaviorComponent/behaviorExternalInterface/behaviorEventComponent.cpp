/**
* File: behaviorEventComponent.h
*
* Author: Kevin M. Karol
* Created: 10/6/17
*
* Description: Component which contains information about changes
* and events that behaviors care about which have come in during the last tick
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/robot.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
namespace{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEventComponent::BehaviorEventComponent()
: IDependencyManagedComponent(this, BCComponentID::BehaviorEventComponent)
, _messageSubscriber(nullptr)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEventComponent::InitDependent(Robot* robot, const BCCompMap& dependentComps)
{
  Init(robot->GetAIComponent().GetComponent<BehaviorComponent>());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEventComponent::Init(IBehaviorMessageSubscriber& messageSubscriber)
{
  _messageSubscriber = std::make_unique<SubscriberWrapper>(messageSubscriber);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEventComponent::SubscribeToTags(IBehavior* subscriber,
                                           std::set<ExternalInterface::MessageGameToEngineTag>&& tags) const
{
  if(ANKI_VERIFY(_messageSubscriber != nullptr,
                 "BehaviorEventComponent.SubscribeToTags.NoMessageSubscriber",
                 "")){
    _messageSubscriber->_ref.SubscribeToTags(subscriber, std::move(tags));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEventComponent::SubscribeToTags(IBehavior* subscriber,
                                           std::set<ExternalInterface::MessageEngineToGameTag>&& tags) const
{
  if(ANKI_VERIFY(_messageSubscriber != nullptr,
                 "BehaviorEventComponent.SubscribeToTags.NoMessageSubscriber",
                 "")){
    _messageSubscriber->_ref.SubscribeToTags(subscriber, std::move(tags));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEventComponent::SubscribeToTags(IBehavior* subscriber,
                                           std::set<RobotInterface::RobotToEngineTag>&& tags) const
{
  if(ANKI_VERIFY(_messageSubscriber != nullptr,
                 "BehaviorEventComponent.SubscribeToTags.NoMessageSubscriber",
                 "")){
    _messageSubscriber->_ref.SubscribeToTags(subscriber, std::move(tags));
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEventComponent::SubscribeToTags(IBehavior* subscriber,
                                             std::set<AppToEngineTag>&& tags) const
{
  if(ANKI_VERIFY(_messageSubscriber != nullptr,
                 "BehaviorEventComponent.SubscribeToTags.NoMessageSubscriber",
                 "")){
    _messageSubscriber->_ref.SubscribeToTags(subscriber, std::move(tags));
  }
}


} // namespace Cozmo
} // namespace Anki
