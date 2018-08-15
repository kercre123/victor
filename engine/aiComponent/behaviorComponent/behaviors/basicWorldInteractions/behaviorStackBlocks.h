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
#include "clad/types/behaviorComponent/behaviorTypes.h"

namespace Anki {
namespace Vector {

// forward declarations
class BehaviorPickUpCube;
class BlockWorldFilter;
class ObservableObject;

class BehaviorStackBlocks : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorStackBlocks(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

private:
  enum class State {
    PickingUpBlock,
    StackingBlock,
    PlayingFinalAnim
  };

  struct InstanceConfig{
    InstanceConfig();
    std::shared_ptr<BehaviorPickUpCube> pickupBehavior;
    bool       stackInAnyOrientation;
    int        placeRetryCount;
    BehaviorID pickupID;
  };

  struct DynamicVariables{
    DynamicVariables();
    ObjectID targetBlockTop;
    ObjectID targetBlockBottom;

    State behaviorState;
    bool  hasBottomTargetSwitched;
    int   placeRetryCount;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToPickingUpBlock();
  void TransitionToStackingBlock();
  void TransitionToFailedToStack();
  void TransitionToPlayingFinalAnim();
  
  
  // Utility functions
  ObjectID GetClosestValidBottom(ObjectInteractionIntention bottomIntention) const;
  void CalculateTargetBlocks(ObjectID& bottomBlock, ObjectID& topBlock) const;
  bool CanUseNonUprightBlocks() const;
  void SetState_internal(State state, const std::string& stateName);

  // prints some useful stuff about the block
  void PrintCubeDebug(const char* event, const ObservableObject* obj) const;
};


}
}


#endif
