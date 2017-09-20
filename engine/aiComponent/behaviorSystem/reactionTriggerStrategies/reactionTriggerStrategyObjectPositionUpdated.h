/**
 * File: reactionTriggerStrategyObject.h
 *
 * Author: Andrew Stein :: Kevin M. Karol
 * Created: 2016-06-14 :: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to seeing an object
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyObject_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyObject_H__

#include "engine/aiComponent/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPositionUpdate.h"
#include "engine/aiComponent/behaviorSystem/behaviorListenerInterfaces/iReactToObjectListener.h"
#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyObjectPositionUpdated : public ReactionTriggerStrategyPositionUpdate, public IReactToObjectListener{
public:
  ReactionTriggerStrategyObjectPositionUpdated(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);

  void HandleObjectObserved(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::RobotObservedObject& msg);
  
  /// Implementation IReactToObjectListener
  virtual void ReactedToID(BehaviorExternalInterface& behaviorExternalInterface, s32 id) override;
  virtual void ClearDesiredTargets(BehaviorExternalInterface& behaviorExternalInterface) override;
  
protected:
  virtual void BehaviorThatStrategyWillTriggerInternal(IBehaviorPtr behavior) override;
  void AlwaysHandlePoseBasedInternal(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;
  
private:
  BehaviorClass _classTriggerMapsTo = BehaviorClass::Wait;
                  
};

} // namespace Cozmo
} // namespace Anki

#endif //
