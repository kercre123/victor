/**
 * File: behaviorReactToNewBlock.h
 *
 * Author: Brad Neuman / Andrew Stein
 * Created: 2016-05-21
 *
 * Description:  React to "new" blocks:
 *     1. If block is out of reach, Cozmo asks for it and waits to be given it (held/placed low so he can pick it up)
 *     2. If block is off ground and reachable, Cozmo reacts and then picks it up
 *     3. If block is on the ground and not already reacted to, Cozmo reacts to it
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToNewBlock_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToNewBlock_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

// Forward declarations:
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
struct RobotObservedObject;
}

class CompoundActionSequential;
class ObservableObject;

  
class BehaviorReactToNewBlock : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToNewBlock(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

  virtual bool IsRunnableInternal(const Robot& robot) const override;

  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  
private:

  std::string _bigReactAnimGroup      = "ag_reactToNewBlock";
  std::string _smallReactAnimGroup    = "ag_reactToNewBlockOnGround";
  std::string _askForBlockAnimGroup   = "ag_askForBlock";
  std::string _askingLoopGroup        = "ag_askForBlockLookLoop";
  std::string _pickupSuccessAnimGroup = "ag_pickUpCube_success";
  std::string _retryPickeupAnimGroup  = "rollCube_retry";
  
  enum class State
  {
    // Main reaction states
    DoingInitialReaction,
    AskingLoop,
    LookingDown,
    PickingUp,
  };

  State _state;

  void TransitionToDoingInitialReaction(Robot& robot);
  void TransitionToAskingLoop(Robot& robot);
  void TransitionToLookingDown(Robot& robot);
  void TransitionToPickingUp(Robot& robot);

  void SetState_internal(State state, const std::string& stateName);
  
  // Returns whether we _can_ pick up the block, and whehter we want to for this
  // behavior
  bool CanPickUpBlock(const Robot& robot, ObjectID objectID, bool& wantToPickup);
  
  void HandleObjectObserved(const Robot& robot, const ExternalInterface::RobotObservedObject& msg);
  void HandleObjectChanged(ObjectID objectID);
  IActionRunner* CreateAskForAction(Robot& robot);
  
  ObjectID    _targetBlock;
  TimeStamp_t _targetBlockLastSeen = 0;
  TimeStamp_t _lookDownTime_ms = 0;
  
  std::set< ObjectID > _reactedBlocks;

}; // class BehaviorReactToNewBlock

} // namespace Cozmo
} // namespace Anki

#endif
