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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPositionUpdate.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorListenerInterfaces/iReactToObjectListener.h"
#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyObjectPositionUpdated : public ReactionTriggerStrategyPositionUpdate, public IReactToObjectListener{
public:
  ReactionTriggerStrategyObjectPositionUpdated(Robot& robot, const Json::Value& config);

  void HandleObjectObserved(const Robot& robot, const ExternalInterface::RobotObservedObject& msg);
  
  /// Implementation IReactToObjectListener
  virtual void ReactedToID(Robot& robot, s32 id) override;
  virtual void ClearDesiredTargets(Robot& robot) override;
  
protected:
  virtual void BehaviorThatStrategyWillTriggerInternal(IBehavior* behavior) override;
  void AlwaysHandlePoseBasedInternal(const EngineToGameEvent& event, const Robot& robot) override;
  virtual bool ShouldTriggerBehaviorInternal(const Robot& robot, const IBehavior* behavior) override;
  virtual void SetupForceTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;
  
private:
  BehaviorClass _classTriggerMapsTo = BehaviorClass::NoneBehavior;
                  
};

} // namespace Cozmo
} // namespace Anki

#endif //
