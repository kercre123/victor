/**
 * File: behaviorReactAcknowledgeCubeMoved.h
 *
 * Author: Kevin M. Karol
 * Created: 7/26/16
 *
 * Description: Behavior to acknowledge when a localized cube has been moved
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactAcknowledgeCubeMoved_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactAcknowledgeCubeMoved_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

//Forward declarations
class ReactionObjectData;
  
namespace ExternalInterface {
  struct RobotObservedObject;
}

  

class BehaviorReactAcknowledgeCubeMoved : public IReactionaryBehavior
{
  
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactAcknowledgeCubeMoved(Robot& robot, const Json::Value& config);


public:
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool ShouldResumeLastBehavior() const override { return true; }
  virtual bool ShouldComputationallySwitch(const Robot& robot) override;

protected:
  virtual void StopInternalReactionary(Robot& robot) override;
  virtual Result InitInternalReactionary(Robot& robot) override;
  
  
private:
  enum class State {
    PlayingSenseReaction,
    TurningToLastLocationOfBlock,
    ReactingToBlockAbsence
  };
  
  typedef std::vector<ReactionObjectData>::iterator Reaction_iter;
    
  State _state;
  ObjectID _activeObjectID; //Most recent move - object to turn towards
  ObjectID _switchObjectID; //Allow one cube moved to interupt another
  std::vector<ReactionObjectData> _reactionObjects; //Tracking for all active objects
  
  void TransitionToPlayingSenseReaction(Robot& robot);
  void TransitionToTurningToLastLocationOfBlock(Robot& robot);
  void TransitionToReactingToBlockAbsence(Robot& robot);
  
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  void HandleObjectMoved(const Robot& robot, const ObjectMoved& msg);
  void HandleObjectStopped(const Robot& robot, const ObjectStoppedMoving& msg);
  void HandleObservedObject(const Robot& robot, const ExternalInterface::RobotObservedObject& msg);
  Reaction_iter GetReactionaryIterator(ObjectID objectID);

  void SetState_internal(State state, const std::string& stateName);
  
}; // class BehaviorReactAcknowledgeCubeMoved

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactAcknowledgeCubeMoved_H__
