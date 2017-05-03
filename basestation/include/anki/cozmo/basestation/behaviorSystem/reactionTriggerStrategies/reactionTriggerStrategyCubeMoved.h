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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {

//Forward declarations
class ReactionObjectData;
class Robot;
  
class ReactionTriggerStrategyCubeMoved : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategyCubeMoved(Robot& robot, const Json::Value& config);

  virtual bool ShouldResumeLastBehavior() const override { return true;}
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }
  virtual bool CanInterruptSelf() const override { return true; }
  
protected:
  virtual void EnabledStateChanged(bool enabled) override;
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;

  virtual bool ShouldTriggerBehaviorInternal(const Robot& robot, const IBehavior* behavior) override;
  virtual void SetupForceTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;
  
private:
  Robot& _robot;
  std::vector<ReactionObjectData> _reactionObjects; //Tracking for all active objects
  
  typedef std::vector<ReactionObjectData>::iterator Reaction_iter;
  
  void HandleObjectMoved(const Robot& robot, const ObjectMoved& msg);
  void HandleObjectStopped(const Robot& robot, const ObjectStoppedMoving& msg);
  void HandleObjectUpAxisChanged(const Robot& robot, const ObjectUpAxisChanged& msg);
  void HandleObservedObject(const Robot& robot, const ExternalInterface::RobotObservedObject& msg);
  void HandleRobotDelocalized();
  Reaction_iter GetReactionaryIterator(ObjectID objectID);
  
};


} // namespace Cozmo
} // namespace Anki

#endif //
