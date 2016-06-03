/**
 * File: behaviorAdmireStack.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-12
 *
 * Description: Behavior to look at and admire a stack of blocks, then knock it over if a 3rd is added
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorAdmireStack_H__
#define __Cozmo_Basestation_Behaviors_BehaviorAdmireStack_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/vision/basestation/observableObject.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;
class Robot;

class BehaviorAdmireStack : public IBehavior
{
public:

  // for demo to know if this worked
  bool DidKnockOverStack() const { return _didKnockOverStack; }
  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAdmireStack(Robot& robot, const Json::Value& config);

  virtual bool IsRunnableInternal(const Robot& robot) const override;

  virtual Result InitInternal(Robot& robot) override;
  virtual Result ResumeInternal(Robot& robot) override { return Result::RESULT_FAIL; }
  virtual void   StopInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;

private:

  std::string _reactToThirdCubeAnimGroup = "knockOverStack_react";
  std::string _tryToGrabThirdCubeAnimGroup = "knockOverStack_grabAttempt";
  std::string _succesAnimGroup = "knockOverStack_success";
  
  enum class State {
    WatchingStack,
    ReactingToThirdBlock,
    TryingToGrabThirdBlock,
    KnockingOverStack,
    ReactingToTopple,
    SearchingForStack,
    KnockingOverStackFailed,
  };

  State _state = State::WatchingStack;

  void TransitionToWatchingStack(Robot& robot);
  void TransitionToReactingToThirdBlock(Robot& robot);
  void TransitionToTryingToGrabThirdBlock(Robot& robot);
  void TransitionToKnockingOverStack(Robot& robot);
  void TransitionToReactingToTopple(Robot& robot);
  void TransitionToSearchingForStack(Robot& robot);
  void TransitionToKnockingOverStackFailed(Robot& robot);

  void SetState_internal(State state, const std::string& stateName);
  void ResetBehavior(Robot& robot);  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  // virtual void HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot) override;

  // last time we saw the last block (based on the block ID in the whiteboard)
  TimeStamp_t _topBlockLastSeentime = 0;

  bool _didKnockOverStack = false;
  
  const int numFramesToWaitForBeforeFlip = 5;
  
  ObjectID _thirdBlockID;
  
};

}
}

#endif
