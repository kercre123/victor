/**
 *  File: behaviorBuildPyramidBase.h
 *  cozmoEngine
 *
 *  Author: Kevin M. Karol
 *  Created: 2016-08-09
 *
 *  Description: Behavior which places two blocks next to each other to form the base
 *  of a pyramid.
 *
 *  Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorBuildPyramidBase_H__
#define __Cozmo_Basestation_Behaviors_BehaviorBuildPyramidBase_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"

namespace Anki {
//forward declaration
class Pose3d;
namespace Cozmo {
class ObservableObject;
enum class AnimationTrigger;
  
class BehaviorBuildPyramidBase : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorBuildPyramidBase(Robot& robot, const Json::Value& config);
  
public:
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}

protected:
  virtual Result InitInternal(Robot& robot) override;
  void StopInternal(Robot& robot) override;
  
  void UpdatePyramidTargets(const Robot& robot) const;
  void ResetPyramidTargets(const Robot& robot) const;
  
  mutable ObjectID _staticBlockID;
  mutable ObjectID _baseBlockID;
  mutable ObjectID _topBlockID;
  
  
  // callback set by derived classes to continue performing actions after the base is built
  SimpleCallbackWithRobot _continuePastBaseCallback;
  
  void TransitionToDrivingToBaseBlock(Robot& robot);
  void TransitionToPlacingBaseBlock(Robot& robot);
  void TransitionToObservingBase(Robot& robot);
  
  
  bool AreAllBlockIDsUnique() const;

private:
  Robot& _robot;
  typedef std::vector<const ObservableObject*> BlockList;
  
  enum class DebugState {
    DrivingToBaseBlock,
    PlacingBaseBlock,
    ObservingBase,
    DrivingToTopBlock,
    AligningWithBase,
    PlacingTopBlock,
    ReactingToPyramid
  };

  ObjectID GetNearestBlockToPose(const Pose3d& pose, const BlockList& allBlocks) const;
  void SafeEraseBlockFromBlockList(ObjectID objectID, BlockList& blockList) const;
  
  // update the offsets for placing the block based on the nearest pose that
  // doesn't have a block in the way
  void UpdateBlockPlacementOffsets() const;
  bool CheckBaseBlockPoseIsFree(f32 xOffset, f32 yOffset) const;
  
  // offsets for placingBlock where the ground is clear
  mutable f32 _baseBlockOffsetX;
  mutable f32 _baseBlockOffsetY;
  
}; //class BehaviorBuildPyramidBase

}//namespace Cozmo
}//namespace Anki


#endif // __Cozmo_Basestation_Behaviors_BehaviorBuildPyramidBase_H__
