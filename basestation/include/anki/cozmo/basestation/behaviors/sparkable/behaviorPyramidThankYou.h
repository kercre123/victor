/**
 * File: behaviorPyramidThankYou.h
 *
 * Author: Kevin M. Karol
 * Created: 01/24/16
 *
 * Description: Behavior to thank users for putting a block upright
 * when it was on its side for build pyramid
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPyramidThankYou_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPyramidThankYou_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;
class Robot;

class BehaviorPyramidThankYou : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPyramidThankYou(Robot& robot, const Json::Value& config);
  virtual ~BehaviorPyramidThankYou();

  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

  virtual bool IsRunnableInternal(const BehaviorPreReqAcknowledgeObject& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false;}
    
private:
  const Robot& _robot;
  mutable s32 _targetID;
};


}
}


#endif
