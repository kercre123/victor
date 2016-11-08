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

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

namespace{
const int kCubeSizeBuffer_mm = 12;
const int kRetryCount = 3;
const int kDriveBackDistance = -50;
const int kDriveBackSpeed = 50;

RetryWrapperAction::RetryCallback retryCallback = [](const ExternalInterface::RobotCompletedAction& completion, const u8 retryCount, AnimationTrigger& animTrigger)
{
  animTrigger = AnimationTrigger::Count;
  return true;
};
}
  
BehaviorBuildPyramidBase::BehaviorBuildPyramidBase(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _continuePastBaseCallback(nullptr)
, _robot(robot)
, _baseBlockOffsetX(0)
, _baseBlockOffsetY(0)
{
  SetDefaultName("BuildPyramidBase");
}

bool BehaviorBuildPyramidBase::IsRunnableInternal(const Robot& robot) const
{
  UpdatePyramidTargets(robot);

  bool allSet = _staticBlockID.IsSet() && _baseBlockID.IsSet();
  if(allSet && AreAllBlockIDsUnique()){
    // Ensure a base does not already exist in the world
    using namespace BlockConfigurations;
    auto pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
    return pyramidBases.empty();
  }
  
  return false;
}

Result BehaviorBuildPyramidBase::InitInternal(Robot& robot)
{
  if(!robot.IsCarryingObject()){
    TransitionToDrivingToBaseBlock(robot);
  }else{
    TransitionToPlacingBaseBlock(robot);
  }
  
  return Result::RESULT_OK;
}
  
void BehaviorBuildPyramidBase::StopInternal(Robot& robot)
{
  ResetPyramidTargets(robot);
}

void BehaviorBuildPyramidBase::TransitionToDrivingToBaseBlock(Robot& robot)
{
  DEBUG_SET_STATE(DrivingToBaseBlock);
  DriveToPickupObjectAction* driveAction = new DriveToPickupObjectAction(robot, _baseBlockID);
  RetryWrapperAction* wrapper = new RetryWrapperAction(robot, driveAction, retryCallback, kRetryCount);

  StartActing(wrapper,
              [this, &robot](const ActionResult& result){
                if(result == ActionResult::SUCCESS)
                {
                  TransitionToPlacingBaseBlock(robot);
                }
              });
  
}
  
  
IBehavior::Status BehaviorBuildPyramidBase::UpdateInternal(Robot& robot)
{
  using namespace BlockConfigurations;
  
  auto pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  if(pyramidBases.size() > 0){
    for(auto basePtr: pyramidBases){
      if((basePtr->GetBaseBlockID() != _baseBlockID && basePtr->GetStaticBlockID() != _baseBlockID) ||
         (basePtr->GetBaseBlockID() != _staticBlockID && basePtr->GetStaticBlockID() != _staticBlockID)){
        StopWithoutImmediateRepetitionPenalty();
      }
    }
  }
  
  IBehavior::Status ret = IBehavior::UpdateInternal(robot);
  
  return ret;
}
  
void BehaviorBuildPyramidBase::TransitionToPlacingBaseBlock(Robot& robot)
{
  DEBUG_SET_STATE(PlacingBaseBlock);
  
  // if a block has moved into the area we want to place the block, update where we are going to place the block
  if(!CheckBaseBlockPoseIsFree(_baseBlockOffsetX, _baseBlockOffsetY)){
    UpdateBlockPlacementOffsets();
  }
  
  const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.TransitionToPlacingBaseBlock.NullObject",
                        "Object %d is NULL", _staticBlockID.GetValue());
    _staticBlockID.UnSet();
    return;
  }
  
  const bool relativeCurrentMarker = false;
  DriveToPlaceRelObjectAction* driveAction = new DriveToPlaceRelObjectAction(robot, _staticBlockID, true, _baseBlockOffsetX, _baseBlockOffsetY, false, 0, false, 0.f, false, relativeCurrentMarker);
  RetryWrapperAction* wrapper = new RetryWrapperAction(robot, driveAction, retryCallback, kRetryCount);
  
  StartActing(wrapper,
              [this, &robot](const ActionResult& result){
                if(result == ActionResult::SUCCESS)
                {
                  TransitionToObservingBase(robot);
                }
              });

}
  
void BehaviorBuildPyramidBase::TransitionToObservingBase(Robot& robot)
{
  DEBUG_SET_STATE(ObservingBase);
  
  StartActing(new DriveStraightAction(robot, kDriveBackDistance, kDriveBackSpeed),
              [this, &robot]{
                if(_continuePastBaseCallback != nullptr){
                  _continuePastBaseCallback(robot);
                }
              });
}

void BehaviorBuildPyramidBase::UpdatePyramidTargets(const Robot& robot) const
{
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
  robot.GetBlockWorld().FindMatchingObjects(bottomBlockFilter, allBlocks);
  
  // set the base blocks for either pyramid base or full pyramid
  if(allBlocks.size() < 2){
    ResetPyramidTargets(robot);
  }
  else if(!_baseBlockID.IsSet() ||
     !_staticBlockID.IsSet()){
    const Pose3d& robotPose = robot.GetPose().GetWithRespectToOrigin();
    
    if(robot.IsCarryingObject()){
      _baseBlockID = robot.GetCarryingObject();
    }else{
      _baseBlockID = GetNearestBlockToPose(robotPose, allBlocks);
    }
    
    const ObservableObject* baseBlock = robot.GetBlockWorld().GetObjectByID(_baseBlockID);
    if(nullptr == baseBlock)
    {
      PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.UpdatePyramidTargets.BaseBlockNull",
                          "BaseBlockID=%d", _baseBlockID.GetValue());
      _baseBlockID.UnSet();
      return;
    }
    
    SafeEraseBlockFromBlockList(_baseBlockID, allBlocks);
    auto baseBlockPose = baseBlock->GetPose().GetWithRespectToOrigin();
    
    _staticBlockID = GetNearestBlockToPose(baseBlockPose, allBlocks);
    const ObservableObject* staticBlock = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
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
    const ObservableObject* staticBlock = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
    if(staticBlock){
      auto staticBlockPose = staticBlock->GetPose().GetWithRespectToOrigin();
      _topBlockID = GetNearestBlockToPose(staticBlockPose, allBlocks);
    }
  }
}
  
void BehaviorBuildPyramidBase::UpdateBlockPlacementOffsets() const
{
  const ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(_staticBlockID);
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.UpdateBlockPlacementOffset.NullObject",
                        "Static block with id %d is NULL", _staticBlockID.GetValue());
    return;
  }
  
  // Construct a list of valid Placement poses next to the block
  f32 nearestDistanceSQ = FLT_MAX;
  f32 blockSize = object->GetSize().y() + kCubeSizeBuffer_mm;
  using Offsets =  std::pair<float,float>;
  std::vector<Offsets> offsetList;
  offsetList.emplace_back(blockSize, 0);
  offsetList.emplace_back(0, blockSize);
  offsetList.emplace_back(0, -blockSize);
  offsetList.emplace_back(-blockSize, 0);

  for(const auto& entry: offsetList){
    if(CheckBaseBlockPoseIsFree(entry.first, entry.second)){
      Pose3d validPose = Pose3d(0.f, Z_AXIS_3D(), {entry.first, entry.second, 0.f}, &object->GetPose());
      f32 newDistSquared;
      ComputeDistanceSQBetween(_robot.GetPose(), validPose, newDistSquared);
      
      if(newDistSquared < nearestDistanceSQ){
        nearestDistanceSQ = newDistSquared;
        _baseBlockOffsetX = entry.first;
        _baseBlockOffsetY = entry.second;
      
      }
    }
  }
}

bool BehaviorBuildPyramidBase::CheckBaseBlockPoseIsFree(f32 xOffset, f32 yOffset) const
{
  const ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(_staticBlockID);
  const ObservableObject* placingObject = _robot.GetBlockWorld().GetObjectByID(_baseBlockID);
  if(nullptr == object || nullptr == placingObject)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramidBase.CHeckBaseBlockPoseIsFree.NullObject",
                        "Static block with id %d or base block with id %d is NULL", _staticBlockID.GetValue(), _baseBlockID.GetValue());
    return false;
  }
  
  
  BlockWorldFilter filter;
  filter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  filter.SetIgnoreIDs({_staticBlockID});
  
  // get the size of the placing object
  const float xSize = placingObject->GetSize().x();
  const float ySize = placingObject->GetSize().y();
  const float zSize = placingObject->GetSize().z();
  
  Pose3d placePose = Pose3d(0.f, Z_AXIS_3D(), {xOffset, yOffset,0.f}, &object->GetPose());
  ObservableObject* closestObject = _robot.GetBlockWorld().FindObjectClosestTo(placePose, {xSize, ySize, zSize}, filter);
  return closestObject == nullptr;
}

  
void BehaviorBuildPyramidBase::ResetPyramidTargets(const Robot& robot) const
{
  _staticBlockID.UnSet();
  _baseBlockID.UnSet();
  _topBlockID.UnSet();
}

  
ObjectID BehaviorBuildPyramidBase::GetNearestBlockToPose(const Pose3d& pose, const BlockList& allBlocks) const
{
  f32 shortestDistanceSQ = -1.f;
  f32 currentDistance = -1.f;
  ObjectID nearestObject;
  
  //Find nearest block to pickup
  for(auto block: allBlocks){
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
  
void BehaviorBuildPyramidBase::SafeEraseBlockFromBlockList(ObjectID objectID, BlockList& blockList) const
{
  const ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(objectID);
  auto iter = std::find(blockList.begin(), blockList.end(), object);
  if(iter != blockList.end()){
    iter = blockList.erase(iter);
  }
}
                    
bool BehaviorBuildPyramidBase::AreAllBlockIDsUnique() const
{
  return (_topBlockID != _staticBlockID &&
          _topBlockID != _baseBlockID &&
          _staticBlockID != _baseBlockID);
}

} //namespace Cozmo
} //namespace Anki
