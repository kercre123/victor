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
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
const int kMaxSearchCount = 1;
const float kWaitForDenouement_sec = 0.75;

namespace{
  RetryWrapperAction::RetryCallback retryCallback = [](const ExternalInterface::RobotCompletedAction& completion, const u8 retryCount, AnimationTrigger& animTrigger)
  {
    animTrigger = AnimationTrigger::Count;
    return true;
  };
  
static const float kTimeObjectInvalidAfterAnyFailure_sec = 30.0f;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBuildPyramid::BehaviorBuildPyramid(Robot& robot, const Json::Value& config)
: BehaviorBuildPyramidBase(robot, config)
, _searchingForNearbyBaseCount(0)
, _searchingForTopBlockCount(0)
{
  SetDefaultName("BuildPyramid");
  _continuePastBaseCallback =  std::bind(&BehaviorBuildPyramid::TransitionToDrivingToTopBlock, (this), std::placeholders::_1);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramid::IsRunnableInternal(const Robot& robot) const
{
  UpdatePyramidTargets(robot);
  
  bool allSetAndUsable = _staticBlockID.IsSet() && _baseBlockID.IsSet() && _topBlockID.IsSet();
  if(allSetAndUsable && !_shouldStreamline){
    // Pyramid is a complicated behavior with a lot of driving around. If we've
    // failed to use any of the blocks and we're not sparked don't bother attempting
    // this behavior - it will probably end poorly
    auto staticBlock = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
    auto baseBlock = robot.GetBlockWorld().GetObjectByID(_baseBlockID);
    auto topBlock = robot.GetBlockWorld().GetObjectByID(_topBlockID);
    
    std::set<AIWhiteboard::ObjectUseAction> failToUseReasons =
                 {{AIWhiteboard::ObjectUseAction::StackOnObject,
                   AIWhiteboard::ObjectUseAction::PickUpObject,
                   AIWhiteboard::ObjectUseAction::RollOrPopAWheelie}};
    
    const bool hasStaticFailed = robot.GetBehaviorManager().GetWhiteboard().
                 DidFailToUse(_staticBlockID,
                              failToUseReasons,
                              kTimeObjectInvalidAfterAnyFailure_sec,
                              staticBlock->GetPose(),
                              DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
                              DefaultFailToUseParams::kAngleToleranceAfterFailure_radians);
    
    const bool hasBaseFailed = robot.GetBehaviorManager().GetWhiteboard().
    DidFailToUse(_baseBlockID,
                 failToUseReasons,
                 kTimeObjectInvalidAfterAnyFailure_sec,
                 baseBlock->GetPose(),
                 DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
                 DefaultFailToUseParams::kAngleToleranceAfterFailure_radians);
    
    const bool hasTopFailed = robot.GetBehaviorManager().GetWhiteboard().
    DidFailToUse(_topBlockID,
                 failToUseReasons,
                 kTimeObjectInvalidAfterAnyFailure_sec,
                 topBlock->GetPose(),
                 DefaultFailToUseParams::kObjectInvalidAfterFailureRadius_mm,
                 DefaultFailToUseParams::kAngleToleranceAfterFailure_radians);
    
    allSetAndUsable = allSetAndUsable && !hasStaticFailed && !hasBaseFailed && !hasTopFailed;
  }
  return allSetAndUsable && AreAllBlockIDsUnique();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorBuildPyramid::InitInternal(Robot& robot)
{
  using namespace BlockConfigurations;
  
  // check to see if a pyramid was built but failed to visually verify
  auto& pyramids = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();
  if(_checkForFullPyramidVisualVerifyFailure && !pyramids.empty()){
    _checkForFullPyramidVisualVerifyFailure = false;
    TransitionToReactingToPyramid(robot);
    return Result::RESULT_OK;
  }else{
    _checkForFullPyramidVisualVerifyFailure = false;
  }
  
  
  const auto& pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  _lastBasesCount = 0;
  _searchingForNearbyBaseCount = 0;
  _searchingForTopBlockCount = 0;
  if(pyramidBases.size() > 0){
    SetPyramidBaseLights();
    if(!robot.IsCarryingObject()){
      TransitionToDrivingToTopBlock(robot);
    }else{
      TransitionToPlacingTopBlock(robot);
    }
  }else{
    SetPickupInitialBlockLights();
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

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramid::TransitionToDrivingToTopBlock(Robot& robot)
{
  SET_STATE(DrivingToTopBlock);
  UpdateAudioState(std::underlying_type<MusicState>::type(MusicState::BaseFormed));

  DriveToPickupObjectAction* driveAction = new DriveToPickupObjectAction(robot, _topBlockID);
  
  StartActing(driveAction,
              [this, &robot](const ActionResult& result){
                if(result == ActionResult::SUCCESS)
                {
                  TransitionToPlacingTopBlock(robot);
                }else{
                  if(_searchingForTopBlockCount < kMaxSearchCount){
                    _searchingForTopBlockCount++;
                    TransitionToSearchingWithCallback(robot, _topBlockID,
                          &BehaviorBuildPyramid::TransitionToDrivingToTopBlock);
                  }
                }
              });
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramid::TransitionToPlacingTopBlock(Robot& robot)
{
  SET_STATE(PlacingTopBlock);
  UpdateAudioState(std::underlying_type<MusicState>::type(MusicState::TopBlockCarry));
  SmartDisableReactionaryBehavior(BehaviorType::AcknowledgeObject);
  
  const ObservableObject* staticBlock = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
  const ObservableObject* baseBlock = robot.GetBlockWorld().GetObjectByID(_baseBlockID);
  if(staticBlock  == nullptr || baseBlock == nullptr)
  {
    PRINT_NAMED_WARNING("BehaviorBuildPyramid.TransitionToPlacingTopBlock.NullObject",
                        "Object %d is NULL", _staticBlockID.GetValue());
    _staticBlockID.UnSet();
    return;
  }
  
  // Figure out the pyramid base block offset to place the top block appropriately
  using namespace BlockConfigurations;
  auto pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();

  for(const auto& basePtr: pyramidBases){
    if(basePtr->ContainsBlock(_baseBlockID) && basePtr->ContainsBlock(_staticBlockID)){
      
      using namespace BlockConfigurations;
      Pose3d idealTopPlacementWRTWorld;
      if(!PyramidBase::GetBaseInteriorMidpoint(robot, staticBlock, baseBlock, idealTopPlacementWRTWorld)){
        return;
      }
      
      const Pose3d& idealTopMarkerPose = staticBlock->GetZRotatedPointAboveObjectCenter();
      Pose3d idealPlacementWRTUnrotatedStatic;
      if(!idealTopPlacementWRTWorld.GetWithRespectTo(idealTopMarkerPose, idealPlacementWRTUnrotatedStatic)){
        return;
      }
      

      // if we've already tried to place the block, see if we can visually verify it now
      const bool relativeCurrentMarker = false;
      DriveToPlaceRelObjectAction* action = new DriveToPlaceRelObjectAction(robot, _staticBlockID, false,
                                                  idealPlacementWRTUnrotatedStatic.GetTranslation().x(),
                                                  idealPlacementWRTUnrotatedStatic.GetTranslation().y(),
                                                  false, 0, false, 0.f, false, relativeCurrentMarker);
      
      StartActing(action,
                  [this, &robot](const ActionResult& result){
                    if (result == ActionResult::SUCCESS) {
                      TransitionToReactingToPyramid(robot);
                    }else{
                      if(_searchingForNearbyBaseCount < kMaxSearchCount){
                        _searchingForNearbyBaseCount++;
                        _checkForFullPyramidVisualVerifyFailure = true;
                        TransitionToSearchingWithCallback(robot, _staticBlockID,
                              &BehaviorBuildPyramid::TransitionToPlacingTopBlock);
                      }
                    }
                  });
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramid::TransitionToReactingToPyramid(Robot& robot)
{
  SET_STATE(ReactingToPyramid);
  UpdateAudioState(static_cast<int>(MusicState::PyramidCompleteFlourish));
  
  SetFullPyramidLights();

  StartActing( new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::BuildPyramidSuccess),
              [this, &robot](const ActionResult& result){
                SetPyramidFlourishLights();
                StartActing(new WaitAction(robot, kWaitForDenouement_sec));
                BehaviorObjectiveAchieved(BehaviorObjective::BuiltPyramid);
              });
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
void BehaviorBuildPyramid::TransitionToSearchingWithCallback(Robot& robot,
                                                             const ObjectID& objectID,
                                                             void(T::*callback)(Robot&))
{
  SET_STATE(SearchingForObject);
  CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
  compoundAction->AddAction(new TurnTowardsObjectAction(robot, objectID, M_PI), true);
  compoundAction->AddAction(new SearchForNearbyObjectAction(robot, objectID));
  StartActing(compoundAction, callback);
}


} //namespace Cozmo
} //namespace Anki
