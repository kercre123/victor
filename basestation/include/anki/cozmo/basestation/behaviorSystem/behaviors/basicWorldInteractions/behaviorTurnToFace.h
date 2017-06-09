/**
* File: behaviorTurnToFace.h
*
* Author: Kevin M. Karol
* Created: 6/6/17
*
* Description: Simple behavior to turn toward a face - face can either be passed
* in as part of IsRunnable, or selected using internal criteria using IsRunnable
* with a robot
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorTurnToFace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorTurnToFace_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/smartFaceId.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorTurnToFace : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorTurnToFace(Robot& robot, const Json::Value& config);
  
public:
  virtual ~BehaviorTurnToFace() override {}
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool IsRunnableInternal(const BehaviorPreReqAcknowledgeFace& preReqData) const override;

  virtual bool CarryingObjectHandledInternally() const override{ return false;}
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  
private:
  mutable SmartFaceID _targetFace;
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorTurnToFace_H__
