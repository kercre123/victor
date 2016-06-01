/**
 * File: behaviorReactToNewBlock.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-21
 *
 * Description: React to a block which has never been seen before
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

template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
struct RobotObservedObject;
}

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
    PostReactionDispatch,
    AskingForBlock,
    AskingLoop, // NOTE: Pickup occurs here if it happens
  };

  State _state;

  void TransitionToDoingInitialReaction(Robot& robot);
  void TransitionToPostReactionDispatch(Robot& robot);
  void TransitionToAskingForBlock(Robot& robot);
  void TransitionToAskingLoop(Robot& robot);

  void SetState_internal(State state, const std::string& stateName);

  static bool ShouldAskForBlock(const Robot& robot, ObjectID blockID);
  
  static bool CanPickUpBlock(const Robot& robot, const ObservableObject* object);
  
  void HandleObjectObserved(const Robot& robot, const ExternalInterface::RobotObservedObject& msg);
  void HandleObjectChanged(ObjectID objectID);
  
  ObjectID    _targetBlock;
  TimeStamp_t _targetBlockLastSeen = 0;
  
  std::set< ObjectID > _reactedBlocks;

};

}
}

#endif
