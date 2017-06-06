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
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/aiComponent/objectInteractionInfoCache.h"


namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;
class Robot;

class BehaviorStackBlocks : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorStackBlocks(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  virtual void   StopInternalFromDoubleTap(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;

  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true;}
  
  virtual void UpdateTargetBlocksInternal(const Robot& robot) const override { BehaviorStackBlocks::UpdateTargetBlocks(robot); }
  
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
  const Robot& _robot;
  State _behaviorState;
  
  mutable ObjectID _targetBlockTop;
  mutable ObjectID _targetBlockBottom;

  bool _stackInAnyOrientation;
  bool _hasBottomTargetSwitched;

  void TransitionToPickingUpBlock(Robot& robot);
  void TransitionToStackingBlock(Robot& robot);
  void TransitionToFailedToStack(Robot& robot);
  void TransitionToPlayingFinalAnim(Robot& robot);

  
  
  
  // Utility functions
  ObjectID GetClosestValidBottom(Robot& robot, ObjectInteractionIntention bottomIntention) const;
  void UpdateTargetBlocks(const Robot& robot) const;
  bool CanUseNonUprightBlocks(const Robot& robot) const;
  void ResetBehavior(const Robot& robot);
  void SetState_internal(State state, const std::string& stateName);

  // prints some useful stuff about the block
  void PrintCubeDebug(const char* event, const ObservableObject* obj) const;
};


}
}


#endif
