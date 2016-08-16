/**
 * File: behaviorDemoFearEdge.h
 *
 * Author: Brad Neuman
 * Created: 2016-04-27
 *
 * Description: Behavior to drive to the edge of the table and react (for the announce demo)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDemoFearEdge_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDemoFearEdge_H__

#include "anki/common/basestation/math/pose.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

class BehaviorDemoFearEdge : public IBehavior
{
protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorDemoFearEdge(Robot& robot, const Json::Value& config);

public:

  virtual bool IsRunnableInternal(const Robot& robot) const override {return true;}
  virtual bool CarryingObjectHandledInternally() const override { return false;}
  
  // true when we have finished driving to the edge, reacting, and backing up
  bool HasFinished() const { return _state == State::Finished; }

protected:

  virtual Result InitInternal(Robot& robot) override;
  virtual Result ResumeInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

private:

  enum class State {
    DrivingForward,
    BackingUpForPounce,
    Finished
  };

  State _state = State::DrivingForward;

  void TransitionToDrivingForward(Robot& robot);
  void TransitionToBackingUpForPounce(Robot& robot);
  void TransitionToFinished();
  
  void SetState_internal(State state, const std::string& stateName);
};

}
}


#endif
