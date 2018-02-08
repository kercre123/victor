/**
 * File: behaviorStackBlocks.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-11
 *
 * Description: Behavior to pick up one cube and stack it on another
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorStackBlocks_H__
#define __Cozmo_Basestation_Behaviors_BehaviorStackBlocks_H__

#include "coretech/common/engine/objectIDs.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"


namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;

class BehaviorStackBlocks : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorStackBlocks(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

  virtual bool WantsToBeActivatedBehavior() const override;
    
  virtual std::set<ObjectInteractionIntention>
        GetBehaviorObjectInteractionIntentions() const override {
          return {(_stackInAnyOrientation ?
                   ObjectInteractionIntention::PickUpObjectNoAxisCheck :
                   ObjectInteractionIntention::PickUpObjectAxisCheck)};
        }
  
private:
  enum class State {
    PickingUpBlock,
    StackingBlock,
    PlayingFinalAnim
  };
  State _behaviorState;
  
  mutable ObjectID _targetBlockTop;
  mutable ObjectID _targetBlockBottom;

  bool _stackInAnyOrientation;
  bool _hasBottomTargetSwitched;

  void TransitionToPickingUpBlock();
  void TransitionToStackingBlock();
  void TransitionToFailedToStack();
  void TransitionToPlayingFinalAnim();
  
  
  // Utility functions
  ObjectID GetClosestValidBottom(ObjectInteractionIntention bottomIntention) const;
  void UpdateTargetBlocks() const;
  bool CanUseNonUprightBlocks() const;
  void ResetBehavior();
  void SetState_internal(State state, const std::string& stateName);

  // prints some useful stuff about the block
  void PrintCubeDebug(const char* event, const ObservableObject* obj) const;
};


}
}


#endif
