/**
 *  File: BehaviorBuildPyramidBase.cpp
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

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorBuildPyramidBase.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/audio/behaviorAudioClient.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/namedColors/namedColors.h"
#include "anki/cozmo/basestation/robot.h"

#include "clad/types/unlockTypes.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const int kMaxSearchCount = 1;
const int kCubeSizeBuffer_mm = 12;
const float kDriveBackDistance_mm = 40.f;
const float kDriveBackSpeed_mm_s = 40.f;

RetryWrapperAction::RetryCallback retryCallback = [](const ExternalInterface::RobotCompletedAction& completion, const u8 retryCount, AnimationTrigger& animTrigger)
{
  animTrigger = AnimationTrigger::Count;
  return true;
};

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBuildPyramidBase::BehaviorBuildPyramidBase(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _lastBasesCount(0)
, _searchingForNearbyBaseBlockCount(0)
, _searchingForNearbyStaticBlockCount(0)
, _continuePastBaseCallback(nullptr)
, _behaviorState(State::DrivingToBaseBlock)
, _checkForFullPyramidVisualVerifyFailure(false)
, _robot(robot)
, _baseBlockOffsetX(0)
, _baseBlockOffsetY(0)
{
  SetDefaultName("BuildPyramidBase");
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const Robot& robot = preReqData.GetRobot();
  UpdatePyramidTargets(robot);

  bool allSet = _staticBlockID.IsSet() && _baseBlockID.IsSet() && !_topBlockID.IsSet();
  if(allSet && AreAllBlockIDsUnique()){
    // Ensure a base does not already exist in the world
    using namespace BlockConfigurations;
    auto pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
    return pyramidBases.empty();
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorBuildPyramidBase::InitInternal(Robot& robot)
{
  _lastBasesCount = 0;
  _searchingForNearbyBaseBlockCount = 0;
  _searchingForNearbyStaticBlockCount = 0;
  if(!robot.IsCarryingObject()){
    TransitionToDrivingToBaseBlock(robot);
  }else{
    TransitionToPlacingBaseBlock(robot);
  }
  
  return Result::RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::StopInternal(Robot& robot)
{
  ResetPyramidTargets(robot);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorBuildPyramidBase::UpdateInternal(Robot& robot)
{
  using namespace BlockConfigurations;
  const auto& pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  const auto& pyramids = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();

  if(!pyramidBases.empty() && _behaviorState < State::ObservingBase){
    StopWithoutImmediateRepetitionPenalty();
    return IBehavior::Status::Complete;
  }
  
  // if a pyramid base was destroyed, stop the behavior to build the base again
  const bool baseDestroyed = pyramidBases.empty() &&
                             pyramids.empty() &&
                             _lastBasesCount > 0;
  
  // We don't want to cancel the action if cozmo is in the middle of reacting to
  // a confirmed pyramid or is commited to placing a block
  const bool placingOrReacting = robot.GetMoveComponent().IsMoving() ||
                                  _behaviorState == State::ReactingToPyramid;
  
  // we may want to react to the base being destroyed once the movement is over
  // so don't update this just now
  if(!placingOrReacting){
    _lastBasesCount = static_cast<int>(pyramidBases.size());
  }
  
  if(baseDestroyed && !placingOrReacting){
    StopWithoutImmediateRepetitionPenalty();
    return IBehavior::Status::Complete;
  }
  
  // prevent against visual verify failures
  if(_checkForFullPyramidVisualVerifyFailure){
    if(!pyramids.empty()){
      StopWithoutImmediateRepetitionPenalty();
      return IBehavior::Status::Complete;
    }
  }
  
  
  IBehavior::Status ret = IBehavior::UpdateInternal(robot);
  
  return ret;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::TransitionToDrivingToBaseBlock(Robot& robot)
{
  SET_STATE(DrivingToBaseBlock);
  DriveToPickupObjectAction* driveAction = new DriveToPickupObjectAction(robot, _baseBlockID);
  
  StartActing(driveAction,
              [this, &robot](const ActionResult& result){
                if(result == ActionResult::SUCCESS)
                {
                  TransitionToPlacingBaseBlock(robot);
                }else{
                  if(_searchingForNearbyBaseBlockCount < kMaxSearchCount){
                    _searchingForNearbyBaseBlockCount++;
                    TransitionToSearchingWithCallback(robot, _baseBlockID, &BehaviorBuildPyramidBase::TransitionToDrivingToBaseBlock);
                  }
                }
              });
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::TransitionToPlacingBaseBlock(Robot& robot)
{
  SET_STATE(PlacingBaseBlock);
  
  // if a block has moved into the area we want to place the block, update where we are going to place the block
  if(!CheckBaseBlockPoseIsFree(_baseBlockOffsetX, _baseBlockOffsetY)){
    UpdateBlockPlacementOffsets();
  }
  
  const ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_staticBlockID);
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.TransitionToPlacingBaseBlock.NullObject",
                        "Object %d is NULL", _staticBlockID.GetValue());
    _staticBlockID.UnSet();
    return;
  }
  
  const bool relativeCurrentMarker = false;
  DriveToPlaceRelObjectAction* driveAction = new DriveToPlaceRelObjectAction(robot, _staticBlockID, true, _baseBlockOffsetX, _baseBlockOffsetY, false, 0, false, 0.f, false, relativeCurrentMarker);
  
  StartActing(driveAction,
              [this, &robot](const ActionResult& result){
                if(result == ActionResult::SUCCESS)
                {
                  TransitionToObservingBase(robot);
                }else{
                  if(_searchingForNearbyStaticBlockCount < kMaxSearchCount){
                    _searchingForNearbyStaticBlockCount++;
                    TransitionToSearchingWithCallback(robot, _staticBlockID, &BehaviorBuildPyramidBase::TransitionToPlacingBaseBlock);
                  }
                }
              });

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::TransitionToObservingBase(Robot& robot)
{
  SET_STATE(ObservingBase);
  
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  action->AddAction(new DriveStraightAction(robot, -kDriveBackDistance_mm, kDriveBackSpeed_mm_s));
  action->AddAction(new TriggerAnimationAction(robot, AnimationTrigger::BuildPyramidReactToBase));

  StartActing(action,
              [this, &robot]{
                if(_continuePastBaseCallback != nullptr){
                  _continuePastBaseCallback(robot);
                }
              });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::GetBaseBlockID(ObjectID& id) const
{
  if(_baseBlockID.IsSet()){
    id = _baseBlockID;
  }
  
  return _baseBlockID.IsSet();
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::GetStaticBlockID(ObjectID& id) const
{
  if(_staticBlockID.IsSet()){
    id = _staticBlockID;
  }
  
  return _staticBlockID.IsSet();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::GetTopBlockID(ObjectID& id) const
{
  if(_topBlockID.IsSet()){
    id = _topBlockID;
  }
  
  return _topBlockID.IsSet();
}
  
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
void BehaviorBuildPyramidBase::TransitionToSearchingWithCallback(Robot& robot,  const ObjectID& objectID,  void(T::*callback)(Robot&))
{
  SET_STATE(SearchingForObject);
  CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
  compoundAction->AddAction(new TurnTowardsObjectAction(robot, objectID, M_PI), true);
  compoundAction->AddAction(new SearchForNearbyObjectAction(robot, objectID));
  StartActing(compoundAction, callback);
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::UpdatePyramidTargets(const Robot& robot) const
{
  ClearInvalidBlockIDs(robot);
  
  using namespace BlockConfigurations;
  
  /// Check block config caches for pyramids and bases for fast forwarding
  
  auto pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  auto pyramids = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();
  
  if(pyramids.size() > 0){
    auto pyramid = pyramids[0];
    _baseBlockID = pyramid->GetPyramidBase().GetBaseBlockID();
    _staticBlockID = pyramid->GetPyramidBase().GetStaticBlockID();
    _topBlockID = pyramid->GetTopBlockID();
    return;
  }
  
  if(pyramidBases.size() > 0){
    auto pyramidBase = pyramidBases[0];
    _baseBlockID = pyramidBase->GetBaseBlockID();
    _staticBlockID = pyramidBase->GetStaticBlockID();
    _topBlockID.UnSet();
  }

  
  BlockList allBlocks;
  BlockWorldFilter bottomBlockFilter;
  bottomBlockFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  robot.GetBlockWorld().FindLocatedMatchingObjects(bottomBlockFilter, allBlocks);
  
  // set the base blocks for either pyramid base or full pyramid
  if(allBlocks.size() < 2){
    ResetPyramidTargets(robot);
  }else if(!_baseBlockID.IsSet() ||
           !_staticBlockID.IsSet()){
    
    _baseBlockID = GetBestBaseBlock(robot, allBlocks);
    
    const ObservableObject* baseBlock = robot.GetBlockWorld().GetLocatedObjectByID(_baseBlockID);
    if(nullptr == baseBlock)
    {
      PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.UpdatePyramidTargets.BaseBlockNull",
                          "BaseBlockID=%d", _baseBlockID.GetValue());
      _baseBlockID.UnSet();
      return;
    }
    SafeEraseBlockFromBlockList(_baseBlockID, allBlocks);
    
    
    _staticBlockID = GetBestStaticBlock(robot, allBlocks);
    const ObservableObject* staticBlock = robot.GetBlockWorld().GetLocatedObjectByID(_staticBlockID);
    if(nullptr == staticBlock)
    {
      PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.UpdatePyramidTargets.StaticBlockNull",
                          "StaticBlockID=%d", _staticBlockID.GetValue());
      _staticBlockID.UnSet();
      return;
    }
    UpdateBlockPlacementOffsets();
    
  }
  
  // If the base blocks are set, remove them as possibilites for the top block
  if(_baseBlockID.IsSet() && _staticBlockID.IsSet()){
    SafeEraseBlockFromBlockList(_baseBlockID, allBlocks);
    SafeEraseBlockFromBlockList(_staticBlockID, allBlocks);
  }
  
  // find the top block so the full pyramid construction can run
  if(!allBlocks.empty() && !_topBlockID.IsSet()){
    const ObservableObject* staticBlock = robot.GetBlockWorld().GetLocatedObjectByID(_staticBlockID);
    if(staticBlock){
      auto staticBlockPose = staticBlock->GetPose().GetWithRespectToOrigin();
      _topBlockID = GetNearestBlockToPose(robot, staticBlockPose, allBlocks);
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::UpdateBlockPlacementOffsets() const
{
  const ObservableObject* object = _robot.GetBlockWorld().GetLocatedObjectByID(_staticBlockID);
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.UpdateBlockPlacementOffset.NullObject",
                        "Static block with id %d is NULL", _staticBlockID.GetValue());
    return;
  }
  
  // Construct a list of valid Placement poses next to the block
  f32 nearestDistanceSQ = FLT_MAX;
  const Point3f blockSize = object->GetSizeInParentFrame();
  const f32 blockXSize = blockSize.x() + kCubeSizeBuffer_mm;
  const f32 blockYSize = blockSize.y() + kCubeSizeBuffer_mm;
  
  using Offsets =  std::pair<float,float>;
  std::vector<Offsets> offsetList;
  offsetList.emplace_back(blockXSize, 0);
  offsetList.emplace_back(0, blockYSize);
  offsetList.emplace_back(0, -blockYSize);
  offsetList.emplace_back(-blockXSize, 0);
  
  // assign default values, just in case
  _baseBlockOffsetX = offsetList[0].first;
  _baseBlockOffsetY = offsetList[0].second;

  for(const auto& entry: offsetList){
    if(CheckBaseBlockPoseIsFree(entry.first, entry.second)){
      Pose3d zRotatedPose = object->GetZRotatedPointAboveObjectCenter(0.f);
      Pose3d potentialPose(0, Z_AXIS_3D(),
                           {entry.first, entry.second, -blockSize.z()},
                           &zRotatedPose);
      f32 newDistSquared;
      ComputeDistanceSQBetween(_robot.GetPose(), potentialPose, newDistSquared);
      
      if(newDistSquared < nearestDistanceSQ){
        nearestDistanceSQ = newDistSquared;
        _baseBlockOffsetX = entry.first;
        _baseBlockOffsetY = entry.second;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::CheckBaseBlockPoseIsFree(f32 xOffset, f32 yOffset) const
{
  const ObservableObject* object = _robot.GetBlockWorld().GetLocatedObjectByID(_staticBlockID);
  const ObservableObject* placingObject = _robot.GetBlockWorld().GetLocatedObjectByID(_baseBlockID);
  if(nullptr == object || nullptr == placingObject)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.CHeckBaseBlockPoseIsFree.NullObject",
                        "Static block with id %d or base block with id %d is NULL",
                        _staticBlockID.GetValue(), _baseBlockID.GetValue());
    return false;
  }
  
  BlockWorldFilter filter;
  filter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  filter.SetIgnoreIDs({_staticBlockID});
  
  // get the size of the placing object
  const Point3f rotatedSize = placingObject->GetSizeInParentFrame();
  
  const Pose3d zRotatedPose = object->GetZRotatedPointAboveObjectCenter(0.f);
  const Pose3d placePose(0, Z_AXIS_3D(), {rotatedSize.x(), rotatedSize.y(), 0}, &zRotatedPose);
  
  ObservableObject* closestObject =
    _robot.GetBlockWorld().FindLocatedObjectClosestTo(placePose.GetWithRespectToOrigin(), rotatedSize, filter);
  return (closestObject == nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::ResetPyramidTargets(const Robot& robot) const
{
  _staticBlockID.UnSet();
  _baseBlockID.UnSet();
  _topBlockID.UnSet();
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID BehaviorBuildPyramidBase::GetBestBaseBlock(const Robot& robot, const BlockList& availableBlocks) const
{
  const Pose3d& robotPose = robot.GetPose().GetWithRespectToOrigin();
  
  // If there's a stacked block it looks good to remove that and place it as the base
  // of the new pyramid
  for(auto& block: availableBlocks){
    const ObservableObject* onTop = robot.GetBlockWorld().FindObjectOnTopOf(*block, BlockWorld::kOnCubeStackHeightTolerence);
    if(onTop != nullptr){
      return onTop->GetID();
    }
  }
  
  if(robot.IsCarryingObject()){
    return robot.GetCarryingObject();
  }else{
    return GetNearestBlockToPose(robot, robotPose, availableBlocks);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID BehaviorBuildPyramidBase::GetBestStaticBlock(const Robot& robot, const BlockList& availableBlocks) const
{
  const Pose3d& robotPose = robot.GetPose().GetWithRespectToOrigin();
  
  if(robot.IsCarryingObject()){
    return robot.GetCarryingObject();
  }
  
  // Static blocks shouldn't have any blocks on top of them and should be on the ground
  BlockList validStaticBlocks;
  for(auto& block: availableBlocks){
    const ObservableObject* onTop = robot.GetBlockWorld().FindObjectOnTopOf(*block, BlockWorld::kOnCubeStackHeightTolerence);

    if(onTop == nullptr &&
       block->IsRestingAtHeight(0, BlockWorld::kOnCubeStackHeightTolerence)){
      validStaticBlocks.push_back(block);
    }
  }
  
  if(validStaticBlocks.size() > 0){
    return GetNearestBlockToPose(robot, robotPose, validStaticBlocks);
  }else{
    return GetNearestBlockToPose(robot, robotPose, availableBlocks);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID BehaviorBuildPyramidBase::GetNearestBlockToPose(const Robot& robot, const Pose3d& pose, const BlockList& availableBlocks) const
{
  f32 shortestDistanceSQ = -1.f;
  f32 currentDistance = -1.f;
  ObjectID nearestObject;
  
  //Find nearest block to pickup
  for(auto block: availableBlocks){
    auto blockPose = block->GetPose().GetWithRespectToOrigin();
    f32 newDistSquared;
    ComputeDistanceSQBetween(pose, blockPose, newDistSquared);

    if(currentDistance < shortestDistanceSQ || shortestDistanceSQ < 0){
      shortestDistanceSQ = currentDistance;
      nearestObject = block->GetID();
    }
  }
  
  return nearestObject;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::SafeEraseBlockFromBlockList(const ObjectID& objectID, BlockList& blockList) const
{
  const ObservableObject* object = _robot.GetBlockWorld().GetLocatedObjectByID(objectID);
  auto iter = std::find(blockList.begin(), blockList.end(), object);
  if(iter != blockList.end()){
    iter = blockList.erase(iter);
  }
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::ClearInvalidBlockIDs(const Robot& robot) const
{
  BlockList allBlocks;
  BlockWorldFilter allBlocksFilter;
  allBlocksFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  robot.GetBlockWorld().FindLocatedMatchingObjects(allBlocksFilter, allBlocks);
  
  bool baseIsValid = false;
  bool staticIsValid = false;
  bool topIsValid = false;
  
  for(const auto& entry: allBlocks){
    if(_baseBlockID.IsSet() &&
       entry->GetID() == _baseBlockID){
      baseIsValid = true;
    }
    
    if(_staticBlockID.IsSet() &&
       entry->GetID() == _staticBlockID){
      staticIsValid = true;
    }
    
    if(_topBlockID.IsSet() &&
       entry->GetID() == _topBlockID){
      topIsValid = true;
    }
  }
  
  if(_baseBlockID.IsSet() && !baseIsValid){
    _baseBlockID.UnSet();
  }
  
  if(_staticBlockID.IsSet() && !staticIsValid){
    _staticBlockID.UnSet();
  }
  
  if(_topBlockID.IsSet() && !topIsValid){
    _topBlockID.UnSet();
  }
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramidBase::AreAllBlockIDsUnique() const
{
  return (_topBlockID != _staticBlockID &&
          _topBlockID != _baseBlockID &&
          _staticBlockID != _baseBlockID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramidBase::SetState_internal(State state, const std::string& stateName){
  _behaviorState = state;
  SetDebugStateName(stateName);
}

} //namespace Cozmo
} //namespace Anki
