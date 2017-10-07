/**
 * File: reactionTriggerStrategyCubeMoved.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to a cube moving
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyCubeMoved_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyCubeMoved_H__

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {

//Forward declarations
class ReactionObjectData;
class Robot;
  
class ReactionTriggerStrategyCubeMoved : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategyCubeMoved(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);

  virtual bool ShouldResumeLastBehavior() const override { return true;}
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }
  virtual bool CanInterruptSelf() const override { return true; }
  
protected:
  virtual void EnabledStateChanged(BehaviorExternalInterface& behaviorExternalInterface, bool enabled) override;
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual bool ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior) override;
  
private:
  std::vector<ReactionObjectData> _reactionObjects; //Tracking for all active objects
  
  typedef std::vector<ReactionObjectData>::iterator Reaction_iter;
  
  void HandleObjectMoved(BehaviorExternalInterface& behaviorExternalInterface, const ObjectMoved& msg);
  void HandleObjectStopped(BehaviorExternalInterface& behaviorExternalInterface, const ObjectStoppedMoving& msg);
  void HandleObjectUpAxisChanged(BehaviorExternalInterface& behaviorExternalInterface, const ObjectUpAxisChanged& msg);
  void HandleObservedObject(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::RobotObservedObject& msg);
  void HandleRobotDelocalized();
  Reaction_iter GetReactionaryIterator(ObjectID objectID);
  
};


} // namespace Cozmo
} // namespace Anki

#endif //
