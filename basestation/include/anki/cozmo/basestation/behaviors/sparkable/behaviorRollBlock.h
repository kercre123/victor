/**
 * File: behaviorRollBlock.h
 *
 * Author: Brad Neuman
 * Created: 2016-04-05
 *
 * Description: This behavior rolls blocks when it see's them not upright
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorRollBlock_H__
#define __Cozmo_Basestation_Behaviors_BehaviorRollBlock_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;
class Robot;

class BehaviorRollBlock : public IBehavior
{
protected:
  using base = IBehavior;
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorRollBlock(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  virtual void   StopInternalFromDoubleTap(Robot& robot) override;

  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
  virtual void UpdateTargetBlocksInternal(const Robot& robot) const override { UpdateTargetBlock(robot); }
  
  virtual std::set<AIWhiteboard::ObjectUseIntention> GetBehaviorObjectUseIntentions() const override { return {(_isBlockRotationImportant ? AIWhiteboard::ObjectUseIntention::RollObjectWithDelegateAxisCheck : AIWhiteboard::ObjectUseIntention::RollObjectWithDelegateNoAxisCheck)}; }
  
private:
  const Robot& _robot;

  // TODO:(bn) a few behaviors have used this pattern now, maybe we should re-think having some kind of
  // UpdateWhileNotRunning
  mutable ObjectID _targetID;
  bool _isBlockRotationImportant;
  
  bool _didCozmoAttemptDock;
  AxisName _upAxisOnBehaviorStart;

  void TransitionToPerformingAction(Robot& robot, bool isRetry = false);
  void TransitionToRollSuccess(Robot& robot);
  void ResetBehavior(Robot& robot);
  virtual void UpdateTargetBlock(const Robot& robot) const;
};

}
}

#endif
