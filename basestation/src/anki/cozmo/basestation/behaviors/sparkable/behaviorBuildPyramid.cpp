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

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperFactory.h"
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace{
RetryWrapperAction::RetryCallback retryCallback = [](const ExternalInterface::RobotCompletedAction& completion, const u8 retryCount, AnimationTrigger& animTrigger)
{
  animTrigger = AnimationTrigger::Count;
  return true;
};
  
static const float kBackupDistCheckTopBlock_mm = 20.0f;
static const float kBackupSpeedCheckTopBlock_mm_s = 20.0f;
static const float kHeadAngleCheckTopBlock_rad = DEG_TO_RAD(25);
static const float kLiftHeightCheckTopBlock_mm = 0;
static const float kWaitForVisualTopBlock_sec = 1;
static const float kHeadBottomCheckTopBlock_rad = DEG_TO_RAD(15);

}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBuildPyramid::BehaviorBuildPyramid(Robot& robot, const Json::Value& config)
: BehaviorBuildPyramidBase(robot, config)
{
  SetDefaultName("BuildPyramid");
  _continuePastBaseCallback =  std::bind(&BehaviorBuildPyramid::TransitionToDrivingToTopBlock, (this), std::placeholders::_1);
  
}
    
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBuildPyramid::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const Robot& robot = preReqData.GetRobot();
  UpdatePyramidTargets(robot);

  bool allSet = _staticBlockID.IsSet() && _baseBlockID.IsSet() && _topBlockID.IsSet();  
  return allSet;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorBuildPyramid::InitInternal(Robot& robot)
{
  using namespace BlockConfigurations;
  ResetMemberVars();
  
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
  if(!pyramidBases.empty() || !pyramids.empty()){
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

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramid::TransitionToDrivingToTopBlock(Robot& robot)
{
  SET_STATE(DrivingToTopBlock);
  
  auto success = [this](Robot& robot){
    TransitionToPlacingTopBlock(robot);
  };
  
  auto& factory = robot.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  PickupBlockParamaters params;
  params.animBeforeDock = AnimationTrigger::BuildPyramidThirdBlockUpright;
  HelperHandle pickupHelper = factory.CreatePickupBlockHelper(robot, *this,
                                _topBlockID, params);
  SmartDelegateToHelper(robot, pickupHelper, success);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramid::TransitionToPlacingTopBlock(Robot& robot)
{
  SET_STATE(PlacingTopBlock);
  SmartDisableReactionTrigger(ReactionTrigger::ObjectPositionUpdated);
  
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
      
      const Pose3d& idealUnrotatedPose = staticBlock->GetZRotatedPointAboveObjectCenter(0.f);
      Pose3d idealPlacementWRTUnrotatedStatic;
      if(!idealTopPlacementWRTWorld.GetWithRespectTo(idealUnrotatedPose, idealPlacementWRTUnrotatedStatic)){
        return;
      }

      // if we've already tried to place the block, see if we can visually verify it now
      const bool relativeCurrentMarker = false;
      
      auto removeSoonFailure = [this](Robot& robot){
        if(!robot.IsCarryingObject()){
          _checkForFullPyramidVisualVerifyFailure = true;
          // This will be removed by a helper soon - hopefully....
          CompoundActionParallel* checkForTopBlock =
          new CompoundActionParallel(robot,{
            new DriveStraightAction(robot, -kBackupDistCheckTopBlock_mm,
                                    kBackupSpeedCheckTopBlock_mm_s, false),
            new MoveHeadToAngleAction(robot, kHeadAngleCheckTopBlock_rad),
            new MoveLiftToHeightAction(robot, kLiftHeightCheckTopBlock_mm)
          });
          CompoundActionSequential* scanForPyramid = new CompoundActionSequential(robot);
          scanForPyramid->AddAction(checkForTopBlock);
          scanForPyramid->AddAction(new WaitAction(robot, kWaitForVisualTopBlock_sec));
          scanForPyramid->AddAction(new MoveHeadToAngleAction(robot, kHeadBottomCheckTopBlock_rad));
          StartActing(scanForPyramid);
        }
      };
      
      auto success = [this](Robot& robot){
        TransitionToReactingToPyramid(robot);
      };
      
      auto& factory = robot.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
      PlaceRelObjectParameters params;
      params.placementOffsetX_mm = idealPlacementWRTUnrotatedStatic.GetTranslation().x();
      params.placementOffsetY_mm = idealPlacementWRTUnrotatedStatic.GetTranslation().y();
      params.relativeCurrentMarker = relativeCurrentMarker;
      
      
      HelperHandle placeRelHelper = factory.CreatePlaceRelObjectHelper(
                                        robot, *this, _staticBlockID, false, params);
      SmartDelegateToHelper(robot, placeRelHelper, success, removeSoonFailure);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBuildPyramid::TransitionToReactingToPyramid(Robot& robot)
{
  SET_STATE(ReactingToPyramid);
  BehaviorObjectiveAchieved(BehaviorObjective::BuiltPyramid);
  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::BuildPyramidSuccess));
}


} //namespace Cozmo
} //namespace Anki
