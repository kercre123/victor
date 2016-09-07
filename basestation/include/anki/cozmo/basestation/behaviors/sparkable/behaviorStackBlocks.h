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
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

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
  virtual Status UpdateInternal(Robot& robot) override;

  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true;}

  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
    
private:
  const f32   _distToBackupOnStackFailure_mm = 40;

  mutable ObjectID _targetBlockTop;
  mutable ObjectID _targetBlockBottom;

  std::unique_ptr<BlockWorldFilter>  _blockworldFilterForTop;
  std::unique_ptr<BlockWorldFilter>  _blockworldFilterForBottom;

  const Robot& _robot;
  
  enum class DebugState {
    PickingUpBlock,
    StackingBlock,
    PlayingFinalAnim,
    WaitForBlocksToBeValid
  };

  bool _waitingForBlockToBeValid;
  
  bool _stackInAnyOrientation = false;

  void TransitionToPickingUpBlock(Robot& robot);
  void TransitionToStackingBlock(Robot& robot);
  void TransitionToPlayingFinalAnim(Robot& robot);
  void TransitionToWaitForBlocksToBeValid(Robot& robot);

  void ResetBehavior(const Robot& robot);

  bool FilterBlocksForTop(const ObservableObject* obj) const;
  bool FilterBlocksForBottom(const ObservableObject* obj) const;
  bool FilterBlocksHelper(const ObservableObject* obj) const;

  bool AreBlocksAreStillValid(const Robot& robot);
  
  void UpdateTargetBlocks(const Robot& robot) const;

  // prints some useful stuff about the block
  void PrintCubeDebug(const char* event, const ObservableObject* obj) const;
};


}
}


#endif
