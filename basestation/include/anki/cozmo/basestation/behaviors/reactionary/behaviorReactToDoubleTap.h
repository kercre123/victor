/**
 * File: behaviorReactToDoubleTap.h
 *
 * Author: Al Chaussee
 * Created: 11/9/2016
 *
 * Description: Behavior to react when a unknown or dirty cube is double tapped
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToDoubleTap_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToDoubleTap_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorReactToDoubleTap : public IBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToDoubleTap(Robot& robot, const Json::Value& config);
  
public:
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  
  // Returns true if the double tapped object is valid to be reacted to
  bool IsTappedObjectValid(const Robot& robot) const;
  
  void TransitionToDriveToCube(Robot& robot);
  
  void TransitionToSearchForCube(Robot& robot);
  
private:
  ObjectID _lastObjectReactedTo;
  
  std::unique_ptr<ObservableObject> _ghostObject;
  
  // Whether or not we are turning towards a ghost object
  bool _turningTowardsGhostObject = false;
  
  // Whether or not we should leaveObjectTapInteraction when this behavior stops
  bool _leaveTapInteractionOnStop = false;
};

}
}

#endif
