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

#include "anki/common/basestation/objectIDs.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"


namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;
class Robot;

class BehaviorStackBlocks : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorStackBlocks(const Json::Value& config);

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true;}
  
  virtual void UpdateTargetBlocksInternal(BehaviorExternalInterface& behaviorExternalInterface) const override { BehaviorStackBlocks::UpdateTargetBlocks(behaviorExternalInterface); }
  
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

  void TransitionToPickingUpBlock(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToStackingBlock(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToFailedToStack(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPlayingFinalAnim(BehaviorExternalInterface& behaviorExternalInterface);
  
  
  // Utility functions
  ObjectID GetClosestValidBottom(BehaviorExternalInterface& behaviorExternalInterface, ObjectInteractionIntention bottomIntention) const;
  void UpdateTargetBlocks(BehaviorExternalInterface& behaviorExternalInterface) const;
  bool CanUseNonUprightBlocks(BehaviorExternalInterface& behaviorExternalInterface) const;
  void ResetBehavior();
  void SetState_internal(State state, const std::string& stateName);

  // prints some useful stuff about the block
  void PrintCubeDebug(BehaviorExternalInterface& behaviorExternalInterface,
                      const char* event, const ObservableObject* obj) const;
};


}
}


#endif
