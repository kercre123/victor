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
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

const int retryCount = 3;
  
namespace{
  RetryWrapperAction::RetryCallback retryCallback = [](const ExternalInterface::RobotCompletedAction& completion, const u8 retryCount, AnimationTrigger& animTrigger)
  {
    animTrigger = AnimationTrigger::Count;
    return true;
  };
}
  
BehaviorBuildPyramid::BehaviorBuildPyramid(Robot& robot, const Json::Value& config)
: BehaviorBuildPyramidBase(robot, config)
{
  SetDefaultName("BuildPyramid");
  _continuePastBaseCallback =  std::bind(&BehaviorBuildPyramid::TransitionToDrivingToTopBlock, (this), std::placeholders::_1);
}

bool BehaviorBuildPyramid::IsRunnableInternal(const Robot& robot) const
{
  UpdatePyramidTargets(robot);
  
  const bool allSet = _staticBlockID.IsSet() && _baseBlockID.IsSet() && _topBlockID.IsSet();
  return allSet && AreAllBlockIDsUnique();
}

Result BehaviorBuildPyramid::InitInternal(Robot& robot)
{
  using namespace BlockConfigurations;
  auto pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager()
                           .GetConfigurationsForType(ConfigurationType::PyramidBase);
  
  if(pyramidBases.size() > 0){
    if(!robot.IsCarryingObject()){
      TransitionToDrivingToTopBlock(robot);
    }else{
      TransitionToPlacingTopBlock(robot);
    }
  }else{
    if(!robot.IsCarryingObject()){
      TransitionToDrivingToBaseBlock(robot);
    }else{
      TransitionToPlacingBaseBlock(robot);
    }
  }
  
  return Result::RESULT_OK;
}
  
void BehaviorBuildPyramid::StopInternal(Robot& robot)
{
  ResetPyramidTargets(robot);
}

void BehaviorBuildPyramid::TransitionToDrivingToTopBlock(Robot& robot)
{
  DEBUG_SET_STATE(DrivingToTopBlock);

  DriveToPickupObjectAction* driveAction = new DriveToPickupObjectAction(robot, _topBlockID);
  
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
  
  // Figure out the pyramid base block offset to place the top block appropriately
  using namespace BlockConfigurations;
  auto pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager()
                           .GetConfigurationsForType(ConfigurationType::PyramidBase);

  for(const auto& configPtr: pyramidBases){
    if(auto sharedPtr = configPtr.lock()){
      if(sharedPtr->ContainsBlock(_baseBlockID) && sharedPtr->ContainsBlock(_staticBlockID)){
        auto basePtr = BlockConfiguration::AsPyramidBaseWeakPtr(configPtr);
        Point2f baseOffset = basePtr.lock()->GetBaseBlockOffsetValues(robot);
  
        const bool relativeCurrentMarker = false;
        DriveToPlaceRelObjectAction* action = new DriveToPlaceRelObjectAction(robot, _staticBlockID, false, baseOffset.x()/2, baseOffset.y()/2, false, 0, false, 0.f, false, relativeCurrentMarker);
        
        RetryWrapperAction* wrapper = new RetryWrapperAction(robot, action, retryCallback, retryCount);
        
        StartActing(wrapper,
                    [this, &robot](const ActionResult& result){
                      if (result == ActionResult::SUCCESS) {
                        TransitionToReactingToPyramid(robot);
                      }
                    });
      }
    }
  }
}

  
void BehaviorBuildPyramid::TransitionToReactingToPyramid(Robot& robot)
{
  DEBUG_SET_STATE(ReactingToPyramid);

  StartActing( new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::BuildPyramidSuccess));
  BehaviorObjectiveAchieved(BehaviorObjective::BuiltPyramid);
  
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

} //namespace Cozmo
} //namespace Anki
