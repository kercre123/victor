/**
 *  File: BehaviorBuildPyramid.hpp
 *  cozmoEngine
 *
 *  Author: Kevin M. Karol
 *  Created: 2016-08-09
 *
 *  Description: Behavior which allows cozmo to build a pyramid from 3 blocks
 *
 *  Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorBuildPyramid_H__
#define __Cozmo_Basestation_Behaviors_BehaviorBuildPyramid_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
//forward declaration
class Pose3d;
namespace Cozmo {
class ObservableObject;
enum class AnimationTrigger;
  
class BehaviorBuildPyramid : public IBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorBuildPyramid(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  
protected:
  
  virtual Result InitInternal(Robot& robot) override;
  void StopInternal(Robot& robot) override;
  
private:
  typedef std::vector<const ObservableObject*> BlockList;
  
  enum class State {
    DrivingToBaseBlock,
    PlacingBaseBlock,
    ObservingBase,
    DrivingToTopBlock,
    AligningWithBase,
    PlacingTopBlock,
    ReactingToPyramid
  };
  State _state = State::DrivingToBaseBlock;
  
  mutable ObjectID _staticBlockID;
  mutable ObjectID _baseBlockID;
  mutable ObjectID _topBlockID;
  
  Pose3d _tempDockingPose;
  bool _tempEnsureNotLoopingForever;
  
  void TransitionToDrivingToBaseBlock(Robot& robot);
  void TransitionToPlacingBaseBlock(Robot& robot);
  void TransitionToObservingBase(Robot& robot);
  void TransitionToDrivingToTopBlock(Robot& robot);
  void TransitionToAligningWithBase(Robot& robot);
  void TransitionToPlacingTopBlock(Robot& robot);
  void TransitionToReactingToPyramid(Robot& robot);
  
  void UpdatePyramidTargets(const Robot& robot) const;
  void ResetPyramidTargets(const Robot& robot) const;
  ObjectID GetNearestBlockToPose(const Pose3d& pose, const BlockList& allBlocks) const;
  void SetState_internal(State state, const std::string& stateName);
  
  template<typename T>
  bool EnsurePoseStateUsable(Robot& robot, ObjectID objectID, void(T::*callback)(Robot&));
  
}; //class BehaviorBuildPyramid

}//namespace Cozmo
}//namespace Anki


#endif // __Cozmo_Basestation_Behaviors_BehaviorBuildPyramid_H__
