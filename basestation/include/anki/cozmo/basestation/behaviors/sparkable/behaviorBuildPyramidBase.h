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

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/common/basestation/objectIDs.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
//forward declaration
class Pose3d;
namespace Cozmo {
class ObservableObject;
class BuildPyramidPersistentUpdate;
enum class AnimationTrigger;
struct ObjectLights;
  
class BehaviorBuildPyramidBase : public IBehavior
{
protected:
  friend class BuildPyramidBehaviorChooser;
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorBuildPyramidBase(Robot& robot, const Json::Value& config);

  
public:
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
  // returns false if the base block id has not been set
  bool GetBaseBlockID(ObjectID& id) const;
  bool GetStaticBlockID(ObjectID& id) const;
  bool GetTopBlockID(ObjectID& id) const;
  
protected:
  enum class State {
    DrivingToBaseBlock = 0,
    PlacingBaseBlock,
    ObservingBase,
    DrivingToTopBlock,
    AligningWithBase,
    PlacingTopBlock,
    ReactingToPyramid,
    SearchingForObject
  };
  
  
  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  void StopInternal(Robot& robot) override;

  void TransitionToDrivingToBaseBlock(Robot& robot);
  void TransitionToPlacingBaseBlock(Robot& robot);
  void TransitionToObservingBase(Robot& robot);

  // utility functions
  void UpdatePyramidTargets(const Robot& robot) const;
  void ResetPyramidTargets(const Robot& robot) const;
  
  void SetState_internal(State state, const std::string& stateName);
  
  template<typename T>
  void TransitionToSearchingWithCallback(Robot& robot,  const ObjectID& objectID,  void(T::*callback)(Robot&));
  
  // Ensures that blocks IDs which become invalid are cleared out of
  // the assigned ObjectIDs below - not actually const
  void ClearInvalidBlockIDs(const Robot& robot) const;
  
  /// Attributes
  mutable ObjectID _staticBlockID;
  mutable ObjectID _baseBlockID;
  mutable ObjectID _topBlockID;
  
  // track how many pyramid bases are known for updating the behavior when they are
  // created or destroyed
  int _lastBasesCount;
  float _timeFirstBaseFormed;
  
  // track retrys with searches
  int _searchingForNearbyBaseBlockCount;
  int _searchingForNearbyStaticBlockCount;

  
  // callback set by derived classes to continue performing actions after the base is built
  SimpleCallbackWithRobot _continuePastBaseCallback;
  
  // track the behavior state for update state checks
  State _behaviorState;
  
  // set if failed to place the top block - allows seeing a pyramid to jump stairght
  // to the flourish on the cubes instead of re-placing the top block
  bool _checkForFullPyramidVisualVerifyFailure;
  
private:
  typedef std::vector<const ObservableObject*> BlockList;

  bool AreAllBlockIDsUnique() const;
  ObjectID GetBestBaseBlock(const Robot& robot, const BlockList& availableBlocks) const;
  ObjectID GetBestStaticBlock(const Robot& robot, const BlockList& availableBlocks) const;
  ObjectID GetNearestBlockToPose(const Robot& robot, const Pose3d& pose, const BlockList& availableBlocks) const;
  
  void SafeEraseBlockFromBlockList(const ObjectID& objectID, BlockList& blockList) const;
  
  // update the offsets for placing the block based on the nearest pose that
  // doesn't have a block in the way
  void UpdateBlockPlacementOffsets() const;
  bool CheckBaseBlockPoseIsFree(f32 xOffset, f32 yOffset) const;
  
  Robot& _robot;

  // offsets for placingBlock where the ground is clear
  mutable f32 _baseBlockOffsetX;
  mutable f32 _baseBlockOffsetY;
  
}; //class BehaviorBuildPyramidBase

}//namespace Cozmo
}//namespace Anki


#endif // __Cozmo_Basestation_Behaviors_BehaviorBuildPyramidBase_H__
