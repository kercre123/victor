/**
 *  File: BehaviorBuildPyramid.cpp
 *
 *  Author: Kevin M. Karol
 *  Created: 2016-08-09
 *
 *  Description: Behavior which allows cozmo to build a pyramid from 3 blocks
 *
 *  Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorBuildPyramid.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

const int cubeSizeBuffer_mm = 7;
const int retryCount = 3;
const int driveBackDistance = -50;
const int driveBackSpeed = 50;
  
namespace{
  RetryWrapperAction::RetryCallback retryCallback = [](const ExternalInterface::RobotCompletedAction& completion, const u8 retryCount, AnimationTrigger& animTrigger)
  {
    animTrigger = AnimationTrigger::Count;
    return true;
  };
}
  
BehaviorBuildPyramid::BehaviorBuildPyramid(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("BuildPyramid");
}

bool BehaviorBuildPyramid::IsRunnableInternal(const Robot& robot) const
{
  UpdatePyramidTargets(robot);
  
  //Possibly add other limits later
  bool allSet = _staticBlockID.IsSet() && _baseBlockID.IsSet() && _topBlockID.IsSet();
  bool allOnGround = true;
  if(allSet)
  {
    const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
    allOnGround = allOnGround && (nullptr != object && object->IsRestingAtHeight(0, ON_GROUND_HEIGHT_TOL_MM));
    
    object = robot.GetBlockWorld().GetObjectByID(_baseBlockID);
    allOnGround = allOnGround && (nullptr != object && object->IsRestingAtHeight(0, ON_GROUND_HEIGHT_TOL_MM));
    
    object = robot.GetBlockWorld().GetObjectByID(_topBlockID);
    allOnGround = allOnGround && (nullptr != object && object->IsRestingAtHeight(0, ON_GROUND_HEIGHT_TOL_MM));
    
  }
  return allSet && allOnGround;
}

Result BehaviorBuildPyramid::InitInternal(Robot& robot)
{
  SmartDisableReactionaryBehavior(BehaviorType::ReactToUnexpectedMovement);
  _tempEnsureNotLoopingForever = false;

  if(!robot.IsCarryingObject()){
    TransitionToDrivingToBaseBlock(robot);
  }else{
    TransitionToPlacingBaseBlock(robot);
  }
  
  return Result::RESULT_OK;
}
  
void BehaviorBuildPyramid::StopInternal(Robot& robot)
{
  ResetPyramidTargets(robot);
}

void BehaviorBuildPyramid::TransitionToDrivingToBaseBlock(Robot& robot)
{
  DEBUG_SET_STATE(DrivingToBaseBlock);
  DriveToPickupObjectAction* driveAction = new DriveToPickupObjectAction(robot, _baseBlockID);
  RetryWrapperAction* wrapper = new RetryWrapperAction(robot, driveAction, retryCallback, retryCount);

  StartActing(wrapper,
              [this, &robot](const ActionResult& result){
                if(result == ActionResult::SUCCESS)
                {
                  TransitionToPlacingBaseBlock(robot);
                }
              });
  
}
  
void BehaviorBuildPyramid::TransitionToPlacingBaseBlock(Robot& robot)
{
  DEBUG_SET_STATE(PlacingBaseBlock);

  bool isUsable = EnsurePoseStateUsable(robot, _staticBlockID, &BehaviorBuildPyramid::TransitionToPlacingBaseBlock);
  if(!isUsable){
    return;
  }
  
  const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramid.TransitionToPlacingBaseBlock.NullObject",
                        "Object %d is NULL", _staticBlockID.GetValue());
    _staticBlockID.UnSet();
    return;
  }
  
  f32 blockSize = object->GetSize().y() + cubeSizeBuffer_mm;
  DriveToPlaceRelObjectAction* driveAction = new DriveToPlaceRelObjectAction(robot, _staticBlockID, true, 0, blockSize);
  RetryWrapperAction* wrapper = new RetryWrapperAction(robot, driveAction, retryCallback, retryCount);
  
  StartActing(wrapper,
              [this, &robot](const ActionResult& result){
                if(result == ActionResult::SUCCESS)
                {
                  TransitionToObservingBase(robot);
                }
              });

}
  
  
void BehaviorBuildPyramid::TransitionToObservingBase(Robot& robot)
{
  DEBUG_SET_STATE(ObservingBase);

  _tempDockingPose = robot.GetPose().GetWithRespectToOrigin();
  
  StartActing(new DriveStraightAction(robot, driveBackDistance, driveBackSpeed),
              &BehaviorBuildPyramid::TransitionToDrivingToTopBlock);
}

void BehaviorBuildPyramid::TransitionToDrivingToTopBlock(Robot& robot)
{
  DEBUG_SET_STATE(DrivingToTopBlock);
  
  bool isUsable = EnsurePoseStateUsable(robot, _topBlockID, &BehaviorBuildPyramid::TransitionToDrivingToTopBlock);
  if(!isUsable){
    return;
  }

  DriveToPickupObjectAction* driveAction = new DriveToPickupObjectAction(robot, _topBlockID);
  
  RetryWrapperAction* wrapper = new RetryWrapperAction(robot, driveAction, retryCallback, retryCount);
  
  StartActing(wrapper,
              [this, &robot](const ActionResult& result){
                if(result == ActionResult::SUCCESS)
                {
                  TransitionToAligningWithBase(robot);
                }else{
                  if(!_tempEnsureNotLoopingForever){
                    EnsurePoseStateUsable(robot, _topBlockID, &BehaviorBuildPyramid::TransitionToDrivingToTopBlock);
                  }
                }
              });
}
  
void BehaviorBuildPyramid::TransitionToAligningWithBase(Robot& robot)
{
  DEBUG_SET_STATE(AligningWithBase);

  bool isUsable = EnsurePoseStateUsable(robot, _staticBlockID, &BehaviorBuildPyramid::TransitionToAligningWithBase);
  if(!isUsable){
    return;
  }
  /**const Pose3d& pyramidPose = robot.GetBlockWorld().GetObjectByID(_staticBlockID)->GetPose().GetWithRespectToOrigin();
  const Pose3d& robotPose = robot.GetPose().GetWithRespectToOrigin();
  Pose3d alignPoseOne = Pose3d(0, Z_AXIS_3D(), {150.f, 0.f,0.f}, &pyramidPose).GetWithRespectToOrigin();
  Pose3d alignPoseTwo = Pose3d(0, Z_AXIS_3D(), {-150.f, 0.f,0.f}, &pyramidPose).GetWithRespectToOrigin();

  f32 alignDistanceOne = 0;
  f32 alignDistanceTwo = 0;
  ComputeDistanceBetween(robotPose, alignPoseOne, alignDistanceOne);
  ComputeDistanceBetween(robotPose, alignPoseTwo, alignDistanceTwo);
  Pose3d& closerAlignmentPose = alignDistanceOne > alignDistanceTwo ? alignPoseTwo : alignPoseOne;**/
  
  
  DriveToPoseAction* driveAction = new DriveToPoseAction(robot, _tempDockingPose);
  
  RetryWrapperAction* wrapper = new RetryWrapperAction(robot, driveAction, retryCallback, retryCount);
  
  StartActing(wrapper,
              [this, &robot](const ActionResult& result){
                if(result == ActionResult::SUCCESS)
                {
                  TransitionToPlacingTopBlock(robot);
                }
              });
}
  
void BehaviorBuildPyramid::TransitionToPlacingTopBlock(Robot& robot)
{
  DEBUG_SET_STATE(PlacingTopBlock);

  const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramid.TransitionToPlacingTopBlock.NullObject",
                        "Object %d is NULL", _staticBlockID.GetValue());
    _staticBlockID.UnSet();
    return;
  }
  
  f32 halfBlockSize = (object->GetSize().y())/2 + cubeSizeBuffer_mm;
  DriveToPlaceRelObjectAction* action = new DriveToPlaceRelObjectAction(robot, _staticBlockID, false, 0, halfBlockSize);
  
  RetryWrapperAction* wrapper = new RetryWrapperAction(robot, action, retryCallback, retryCount);
  
  StartActing(wrapper,
              [this, &robot](const ActionResult& result){
                if (result == ActionResult::SUCCESS) {
                  TransitionToReactingToPyramid(robot);
                }
              });
}

  
void BehaviorBuildPyramid::TransitionToReactingToPyramid(Robot& robot)
{
  DEBUG_SET_STATE(ReactingToPyramid);

  StartActing( new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::BuildPyramidSuccess));
  BehaviorObjectiveAchieved(BehaviorObjective::BuiltPyramid);
  
}

void BehaviorBuildPyramid::UpdatePyramidTargets(const Robot& robot) const
{
  BlockList allBlocks;
  BlockWorldFilter bottomBlockFilter;
  bottomBlockFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  robot.GetBlockWorld().FindMatchingObjects(bottomBlockFilter, allBlocks);
  
  if(allBlocks.size() >= 3){
    const Pose3d& robotPose = robot.GetPose().GetWithRespectToOrigin();
    
    if(robot.IsCarryingObject()){
      _baseBlockID = robot.GetCarryingObject();
    }else{
      _baseBlockID = GetNearestBlockToPose(robotPose, allBlocks);
    }
    
    const ObservableObject* baseBlock = robot.GetBlockWorld().GetObjectByID(_baseBlockID);
    if(nullptr == baseBlock)
    {
      PRINT_NAMED_WARNING("BehaviorBuildPyramid.UpdatePyramidTargets.BaseBlockNull",
                          "BaseBlockID=%d", _baseBlockID.GetValue());
      _baseBlockID.UnSet();
      return;
    }
    
    auto baseBlockPose = baseBlock->GetPose().GetWithRespectToOrigin();
    auto baseIter = std::find(allBlocks.begin(), allBlocks.end(), baseBlock);
    allBlocks.erase(baseIter);
    
    _staticBlockID = GetNearestBlockToPose(baseBlockPose, allBlocks);
    const ObservableObject* staticBlock = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
    if(nullptr == staticBlock)
    {
      PRINT_NAMED_WARNING("BehaviorBuildPyramid.UpdatePyramidTargets.StaticBlockNull",
                          "StaticBlockID=%d", _staticBlockID.GetValue());
      _staticBlockID.UnSet();
      return;
    }
    
    auto staticBlockPose = staticBlock->GetPose().GetWithRespectToOrigin();
    auto staticIter = std::find(allBlocks.begin(), allBlocks.end(), staticBlock);
    allBlocks.erase(staticIter);
    
    
    _topBlockID = GetNearestBlockToPose(staticBlockPose, allBlocks);

  }else{
    ResetPyramidTargets(robot);
  }
}
  
void BehaviorBuildPyramid::ResetPyramidTargets(const Robot& robot) const
{
  _staticBlockID.UnSet();
  _baseBlockID.UnSet();
  _topBlockID.UnSet();
}

  
ObjectID BehaviorBuildPyramid::GetNearestBlockToPose(const Pose3d& pose, const BlockList& allBlocks) const
{
  f32 shortestDistance = -1.f;
  f32 currentDistance = -1.f;
  ObjectID nearestObject;
  
  //Find nearest block to pickup
  for(auto block: allBlocks){
    auto blockPose = block->GetPose().GetWithRespectToOrigin();
    ComputeDistanceBetween(pose, blockPose, currentDistance);
    if(currentDistance < shortestDistance || shortestDistance < 0){
      shortestDistance = currentDistance;
      nearestObject = block->GetID();
    }
  }
  
  return nearestObject;
}
  
template<typename T>
bool BehaviorBuildPyramid::EnsurePoseStateUsable(Robot& robot, ObjectID objectID, void(T::*callback)(Robot&))
{
  const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(objectID);
  if(object != nullptr
     && object->IsPoseStateUnknown()){
    StartActing(new TurnTowardsPoseAction(robot, object->GetPose(), M_PI),
                [this, &robot, objectID, callback](const ActionResult& result){
                  if(result == ActionResult::SUCCESS)
                  {
                    const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(objectID);
                    if(object != nullptr
                       && object->IsPoseStateUnknown()){
                      const Pose3d& lastKnownPose =  object->GetPose();
                      StartActing(new DriveToPoseAction(robot, lastKnownPose),
                                  [this, &robot, objectID, callback](const ActionResult& result){
                                    const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(objectID);
                                    if(object != nullptr
                                       && !object->IsPoseStateUnknown()){
                                          std::bind(callback, static_cast<T*>(this), std::placeholders::_1)(robot);
                                    }
                      });
                    }else if(object != nullptr){
                      std::bind(callback, static_cast<T*>(this), std::placeholders::_1)(robot);
                    }
                  }
                });
    return false;
  }
  
  return true;
}

} //namespace Cozmo
} //namespace Anki
