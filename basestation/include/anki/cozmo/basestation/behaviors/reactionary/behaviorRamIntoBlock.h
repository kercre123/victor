/**
 * File: behaviorRamIntoBlock.h
 *
 * Author: Kevin M. Karol
 * Created: 2/21/17
 *
 * Description: Behavior to ram into a block
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorRamIntoBlock_H__
#define __Cozmo_Basestation_Behaviors_BehaviorRamIntoBlock_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

namespace ExternalInterface {
struct RobotObservedObject;
}

class BehaviorRamIntoBlock : public IBehavior
{
  
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorRamIntoBlock(Robot& robot, const Json::Value& config);


public:
  virtual bool IsRunnableInternal(const BehaviorPreReqAcknowledgeObject& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true;}

protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual void StopInternal(Robot& robot) override;
  virtual Result ResumeInternal(Robot& robot) override;

private:
  mutable s32 _targetID;
  
  void TransitionToPuttingDownBlock(Robot& robot);
  void TransitionToTurningToBlock(Robot& robot);
  void TransitionToRammingIntoBlock(Robot& robot);

  
}; // class BehaviorRamIntoBlock

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorRamIntoBlock_H__
