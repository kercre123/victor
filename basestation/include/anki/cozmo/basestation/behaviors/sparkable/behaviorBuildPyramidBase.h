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

  void TransitionToDrivingToBaseBlock(Robot& robot);
  void TransitionToPlacingBaseBlock(Robot& robot);
  void TransitionToObservingBase(Robot& robot);

  // utility functions
  void SetState_internal(State state, const std::string& stateName);
  void ResetMemberVars();
  // Returns true if any pyramid targets have changed
  bool UpdatePyramidTargets(const Robot& robot) const;
  
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
  float _timeLastBaseDestroyed;
  
  // callback set by derived classes to continue performing actions after the base is built
  SimpleCallbackWithRobot _continuePastBaseCallback;
  
  // track the behavior state for update state checks
  State _behaviorState;
  
  // set if failed to place the top block - allows seeing a pyramid to jump stairght
  // to the flourish on the cubes instead of re-placing the top block
  bool _checkForFullPyramidVisualVerifyFailure;
  
private:
  // update the offsets for placing the block based on the nearest pose that
  // doesn't have a block in the way
  void UpdateBlockPlacementOffsets(f32& xOffset, f32& yOffset) const;
  bool CheckBaseBlockPoseIsFree(f32 xOffset, f32 yOffset) const;
  bool AreAllBlockIDsUnique() const;

  
  Robot& _robot;
  
}; //class BehaviorBuildPyramidBase

}//namespace Cozmo
}//namespace Anki


#endif // __Cozmo_Basestation_Behaviors_BehaviorBuildPyramidBase_H__
