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
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
//forward declaration
class Pose3d;
namespace Cozmo {
class ObservableObject;
enum class AnimationTrigger;
struct ObjectLights;
  
class BehaviorBuildPyramidBase : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorBuildPyramidBase(Robot& robot, const Json::Value& config);
  
public:
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
  // Persistent function which can be ticked while sparked to handle
  // light and music states while a specific pyramid behavior isn't running
  static void SparksPersistantMusicLightControls(Robot& robot, int& lastPyramidBaseSeenCount, bool& wasRobotCarryingObject, bool& sparksPersistantCallbackSet);
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  void StopInternal(Robot& robot) override;
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;

  
  void UpdatePyramidTargets(const Robot& robot) const;
  void ResetPyramidTargets(const Robot& robot) const;
  
  mutable ObjectID _staticBlockID;
  mutable ObjectID _baseBlockID;
  mutable ObjectID _topBlockID;
  
  // track how many pyramid bases are known for updating the behavior when they are
  // created or destroyed
  int _lastBasesCount;
  
  // callback set by derived classes to continue performing actions after the base is built
  SimpleCallbackWithRobot _continuePastBaseCallback;
  
  void TransitionToDrivingToBaseBlock(Robot& robot);
  void TransitionToPlacingBaseBlock(Robot& robot);
  void TransitionToObservingBase(Robot& robot);
  

  enum class DebugState {
    DrivingToBaseBlock,
    PlacingBaseBlock,
    ObservingBase,
    DrivingToTopBlock,
    AligningWithBase,
    PlacingTopBlock,
    ReactingToPyramid
  };
  
  // Match music Rounds to enum values
  enum class MusicState{
    SearchingForCube = 1,
    InitialCubeCarry,
    BaseFormed,
    TopBlockCarry,
    PyramidCompleteFlourish
  };
  
  // Lights for build process
  static void SetPyramidBaseLightsByID(Robot& robot, const ObjectID& staticID, const ObjectID& baseID);

  void SetPickupInitialBlockLights();
  void SetPyramidBaseLights();
  void SetFullPyramidLights();
  void SetPyramidFlourishLights();
  bool AreAllBlockIDsUnique() const;
  
  static const ObjectLights& GetSingleStaticBlockLights();
  
  static const ObjectLights& GetBaseFormedLights();
  static ObjectLights GetBaseFormedBaseLights(Robot& robot, const ObjectID& staticID, const ObjectID& baseID);
  static ObjectLights GetBaseFormedStaticLights(Robot& robot, const ObjectID& staticID, const ObjectID& baseID);
  static const ObjectLights& GetBaseFormedTopLights();
  
  static const ObjectLights& GetFullPyramidLights(Robot& robot);
  
  const ObjectLights& GetFlourishBaseLights() const;
  const ObjectLights& GetFlourishStaticLights() const;
  const ObjectLights& GetFlourishTopLights() const;
    
private:
  Robot& _robot;
  typedef std::vector<const ObservableObject*> BlockList;
  
  MusicState _musicState;

  ObjectID GetNearestBlockToPose(const Pose3d& pose, const BlockList& allBlocks) const;
  void SafeEraseBlockFromBlockList(const ObjectID& objectID, BlockList& blockList) const;
  
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
