/**
 * File: behaviorFindFaces.h
 *
 * Author: Brad Neuman
 * Created: 2016-08-31
 *
 * Description: Originally written by Lee, rewritten by Brad to be based on the "look in place" behavior
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorFindFaces_H__
#define __Cozmo_Basestation_Behaviors_BehaviorFindFaces_H__

#include "anki/cozmo/basestation/behaviors/exploration/behaviorExploreLookAroundInPlace.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorFindFaces : public BehaviorExploreLookAroundInPlace
{
  using BaseClass = BehaviorExploreLookAroundInPlace;
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorFindFaces(Robot& robot, const Json::Value& config);
  
public:

  virtual ~BehaviorFindFaces() override {}

  virtual bool IsRunnableInternal(const Robot& robot) const override;
    
protected:
  
  virtual void BeginStateMachine(Robot& robot) override;
  
private:

  void TransitionToLookUp(Robot& robot);
  void TransitionToLookAtLastFace(Robot& robot);
  void TransitionToBaseClass(Robot& robot);

  u32 _maxFaceAgeToLook_ms = 0;
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFindFaces_H__
