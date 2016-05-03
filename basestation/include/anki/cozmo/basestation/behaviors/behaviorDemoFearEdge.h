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

  virtual bool IsRunnable(const Robot& robot) const override {return true;}

protected:

  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

private:

  enum class State {
    DrivingForward
  };

  State _state = State::DrivingForward;

  void TransitionToDrivingForward(Robot& robot);

  void SetState_internal(State state, const std::string& stateName);
};

}
}


#endif
