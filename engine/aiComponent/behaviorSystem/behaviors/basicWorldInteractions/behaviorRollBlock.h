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
#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"

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
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorRollBlock(const Json::Value& config);

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual bool IsRunnableInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
  virtual void UpdateTargetBlocksInternal(BehaviorExternalInterface& behaviorExternalInterface) const override { UpdateTargetBlock(behaviorExternalInterface); }
  
  virtual std::set<ObjectInteractionIntention>
        GetBehaviorObjectInteractionIntentions() const override {
          return {(_isBlockRotationImportant ?
                   ObjectInteractionIntention::RollObjectWithDelegateAxisCheck :
                   ObjectInteractionIntention::RollObjectWithDelegateNoAxisCheck)};
        }
  
private:
  enum class State {
    RollingBlock,
    CelebratingRoll
  };
  
  // TODO:(bn) a few behaviors have used this pattern now, maybe we should re-think having some kind of
  // UpdateWhileNotRunning
  mutable ObjectID _targetID;
  bool _isBlockRotationImportant;
  
  bool _didCozmoAttemptDock;
  AxisName _upAxisOnBehaviorStart;
  State    _behaviorState;

  void TransitionToPerformingAction(BehaviorExternalInterface& behaviorExternalInterface, bool isRetry = false);
  void TransitionToRollSuccess(BehaviorExternalInterface& behaviorExternalInterface);
  void ResetBehavior(BehaviorExternalInterface& behaviorExternalInterface);
  virtual void UpdateTargetBlock(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  void UpdateTargetsUpAxis(BehaviorExternalInterface& behaviorExternalInterface);
  
  void SetState_internal(State state, const std::string& stateName);

};

}
}

#endif
