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
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;
class Robot;

class BehaviorRollBlock : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorRollBlock(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  virtual void   StopInternalFromDoubleTap(Robot& robot) override;

  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
  virtual void UpdateTargetBlocksInternal(const Robot& robot) const override { UpdateTargetBlock(robot); }
  
  virtual std::set<AIWhiteboard::ObjectUseIntention> GetBehaviorObjectUseIntentions() const override { return {(_isBlockRotationImportant ? AIWhiteboard::ObjectUseIntention::RollObjectWithAxisCheck : AIWhiteboard::ObjectUseIntention::RollObjectNoAxisCheck)}; }
  
private:

  AnimationTrigger _putDownAnimTrigger = AnimationTrigger::Count;

  // TODO:(bn) a few behaviors have used this pattern now, maybe we should re-think having some kind of
  // UpdateWhileNotRunning
  mutable ObjectID _targetBlock;
  
  s32 _numRollActionRetries = 0;

  bool _isBlockRotationImportant;

  const Robot& _robot;
  
  enum class DebugState {
    SettingDownBlock,
    PerformingAction
  };

  void TransitionToSettingDownBlock(Robot& robot);
  void TransitionToPerformingAction(Robot& robot, bool isRetry = false);

  void SetupRetryAction(Robot& robot, const ExternalInterface::RobotCompletedAction& msg);
  
  void ResetBehavior(Robot& robot);

  virtual void UpdateTargetBlock(const Robot& robot) const;
};

}
}

#endif
