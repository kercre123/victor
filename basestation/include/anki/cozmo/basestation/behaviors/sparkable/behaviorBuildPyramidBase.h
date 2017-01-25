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

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"

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
  // Enforce creation through BehaviorFactory
  friend class BuildPyramidPersistentUpdate;
  friend class BehaviorFactory;
  BehaviorBuildPyramidBase(Robot& robot, const Json::Value& config);

  
public:
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  
  // Match music Rounds to enum values - starts at 1 to match rounds set up
  // in the current audio sound banks
  enum class MusicState{
    SearchingForCube = 1,
    InitialCubeCarry,
    BaseFormed,
    TopBlockCarry,
    PyramidCompleteFlourish
  };
  
  virtual bool CarryingObjectHandledInternally() const override {return true;}

  
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
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  
  template<typename T>
  void TransitionToSearchingWithCallback(Robot& robot,  const ObjectID& objectID,  void(T::*callback)(Robot&));
  
  /// Light functions
  static void SetPyramidBaseLightsByID(Robot& robot,
                                       const ObjectID& staticID,
                                       const ObjectID& baseID);
  static ObjectLights GetBaseFormedBaseLightsModifier(Robot& robot,
                                                      const ObjectID& staticID,
                                                      const ObjectID& baseID);
  static ObjectLights GetBaseFormedStaticLightsModifier(Robot& robot,
                                                        const ObjectID& staticID,
                                                        const ObjectID& baseID);
  
  void SetPickupInitialBlockLights();
  void SetPyramidBaseLights();
  void SetFullPyramidLights();
  void SetPyramidFlourishLights();
  bool AreAllBlockIDsUnique() const;
  
  ObjectLights GetDenouementTopLightsModifier() const;
  
  /// Attributes
  mutable ObjectID _staticBlockID;
  mutable ObjectID _baseBlockID;
  mutable ObjectID _topBlockID;
  
  // track how many pyramid bases are known for updating the behavior when they are
  // created or destroyed
  int _lastBasesCount;
  
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

  ObjectID GetNearestBlockToPose(const Pose3d& pose, const BlockList& allBlocks) const;
  void SafeEraseBlockFromBlockList(const ObjectID& objectID, BlockList& blockList) const;
  
  // update the offsets for placing the block based on the nearest pose that
  // doesn't have a block in the way
  void UpdateBlockPlacementOffsets() const;
  bool CheckBaseBlockPoseIsFree(f32 xOffset, f32 yOffset) const;
  
  Robot& _robot;
  MusicState _musicState;

  // offsets for placingBlock where the ground is clear
  mutable f32 _baseBlockOffsetX;
  mutable f32 _baseBlockOffsetY;
  
}; //class BehaviorBuildPyramidBase

}//namespace Cozmo
}//namespace Anki


#endif // __Cozmo_Basestation_Behaviors_BehaviorBuildPyramidBase_H__
