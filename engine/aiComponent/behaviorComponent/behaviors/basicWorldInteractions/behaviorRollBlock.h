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

#include "coretech/common/engine/objectIDs.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"

#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;

class BehaviorRollBlock : public ICozmoBehavior
{
protected:
  using base = ICozmoBehavior;
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorRollBlock(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

  virtual bool WantsToBeActivatedBehavior() const override;
    
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

  void TransitionToPerformingAction(bool isRetry = false);
  void TransitionToRollSuccess();
  void ResetBehavior();
  virtual void UpdateTargetBlock() const;
  
  void UpdateTargetsUpAxis();
  
  void SetState_internal(State state, const std::string& stateName);

};

}
}

#endif
