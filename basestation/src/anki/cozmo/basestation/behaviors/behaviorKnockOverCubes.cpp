/**
 * File: behaviorKnockOverCubes.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2016-08-01
 *
 * Description: Behavior to tip over a stack of cubes
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorKnockOverCubes.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/flipBlockAction.h"
#include "anki/cozmo/basestation/behaviors/behaviorPutDownBlock.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/console/consoleInterface.h"


#define SET_STATE(s) SetState_internal(State::s, #s)


static const char* const kReachForBlockTrigger = "reachForBlockTrigger";
static const char* const kKnockOverEyesTrigger = "knockOverEyesTrigger";
static const char* const kKnockOverSuccessTrigger = "knockOverSuccessTrigger";
static const char* const kPutDownTrigger = "knockOverPutDownTrigger";
static const char* const kMinimumStackHeight = "minimumStackHeight";
static const char* const kShouldStreamline = "shouldStreamline";

const int numFramesToWaitForBeforeFlip = 5;
CONSOLE_VAR(f32, kBKS_headAngleForKnockOver_deg, "Behavior.AdmireStack", -14.0f);
CONSOLE_VAR(f32, kBKS_distanceToTryToGrabFrom_mm, "Behavior.AdmireStack", 85.0f);
CONSOLE_VAR(f32, kBKS_searchSpeed_mmps, "Behavior.AdmireStack", 60.0f);

namespace Anki {
namespace Cozmo {
  
  
BehaviorKnockOverCubes::BehaviorKnockOverCubes(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  SetDefaultName("KnockOverCubes");
  
  LoadConfig(config);

}
  
void BehaviorKnockOverCubes::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  
  GetValueOptional(config, kReachForBlockTrigger, _reachForBlockTrigger);
  GetValueOptional(config, kKnockOverEyesTrigger,_knockOverEyesTrigger);
  GetValueOptional(config, kKnockOverSuccessTrigger,_knockOverSuccessTrigger);
  GetValueOptional(config, kPutDownTrigger, _putDownAnimTrigger);
  
  _minStackHeight = config.get(kMinimumStackHeight, 3).asInt();
  _shouldStreamline = config.get(kShouldStreamline, false).asBool();
  
}

bool BehaviorKnockOverCubes::IsRunnableInternal(const Robot& robot) const
{
  UpdateTargetStack(robot);
  
  return _baseBlockID.IsSet() && (_stackHeight >= _minStackHeight) ;
}

Result BehaviorKnockOverCubes::InitInternal(Robot& robot)
{
  if (robot.IsCarryingObject())
  {
    TranstitionToSettingDownBlock(robot);
  }
  else
  {
    if(!_shouldStreamline){
      TransitionToReachingForBlock(robot);
    }else{
      TransitionToDrivingToReadyPose(robot);
    }
  }
  
  return Result::RESULT_OK;
}
  
Result BehaviorKnockOverCubes::ResumeInternal(Robot& robot)
{
  if(IsRunnableInternal(robot)){
    TransitionToDrivingToReadyPose(robot);
    return Result::RESULT_OK;
  }
  return Result::RESULT_FAIL;
}

  
void BehaviorKnockOverCubes::StopInternal(Robot& robot)
{
  if(_state == State::KnockingOverStack){
    robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToUnexpectedMovement, true);
  }
  
  if(_state == State::PlayingReaction
     || _state == State::KnockingOverStack){
    robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::AcknowledgeObject, true);
  }
  
  ResetBehavior(robot);
  
}
  
void BehaviorKnockOverCubes::TransitionToReachingForBlock(Robot& robot)
{
  SET_STATE(ReachingForBlock);
  
  ObservableObject* lastObj = robot.GetBlockWorld().GetObjectByID(_baseBlockID);
  ObservableObject* nextObj;
  
  BOUNDED_WHILE(10, nextObj = robot.GetBlockWorld().FindObjectOnTopOf(*lastObj, STACKED_HEIGHT_TOL_MM)){
    lastObj = nextObj;
  }
  
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  
  Pose3d poseWrtRobot;
  if(lastObj->GetPose().GetWithRespectTo(robot.GetPose(), poseWrtRobot) ) {
    const float fudgeFactor = 10.0f;
    if( poseWrtRobot.GetTranslation().x() + fudgeFactor > kBKS_distanceToTryToGrabFrom_mm) {
      float distToDrive = poseWrtRobot.GetTranslation().x() - kBKS_distanceToTryToGrabFrom_mm;
      action->AddAction(new DriveStraightAction(robot, distToDrive, kBKS_searchSpeed_mmps));
    }
  }
  
  action->AddAction(new TriggerAnimationAction(robot, _reachForBlockTrigger));
  StartActing(action, &BehaviorKnockOverCubes::TransitionToDrivingToReadyPose);
  
}


  
  
void BehaviorKnockOverCubes::TransitionToDrivingToReadyPose(Robot& robot)
{
  SET_STATE(DrivingToReadyPose);
  
  auto nextFunction = &BehaviorKnockOverCubes::TransitionToTurningTowardsFace;
  
  if(_shouldStreamline){
    nextFunction = &BehaviorKnockOverCubes::TransitionToKnockingOverStack;
    
  }
  
  StartActing(new DriveToFlipBlockPoseAction(robot, _baseBlockID), nextFunction);
}

void BehaviorKnockOverCubes::TransitionToTurningTowardsFace(Robot& robot)
{
  SET_STATE(TurningTowardsFace);
  
  Pose3d facePose;
  size_t facesInMemory = robot.GetFaceWorld().GetKnownFaceIDs().size();
  
  
  if(facesInMemory != 0){
  
  StartActing( new TurnTowardsFaceWrapperAction(robot,
                                                new TriggerAnimationAction(robot, _knockOverEyesTrigger))
              , &BehaviorKnockOverCubes::TransitionToAligningWithStack);
  }else{
    TransitionToAligningWithStack(robot);
  }
  
}
  
  
void BehaviorKnockOverCubes::TransitionToAligningWithStack(Robot& robot)
{
  SET_STATE(AligningWithStack);
  
  CompoundActionSequential* action = new CompoundActionSequential(robot);

  // Turn towards bottom block of stack
  action->AddAction(new TurnTowardsObjectAction(robot,
                                                _baseBlockID,
                                                Radians(PI_F),
                                                false, // verify when done?
                                                false));
  // Move head for knock over
  action->AddAction(new MoveHeadToAngleAction(robot, DEG_TO_RAD(kBKS_headAngleForKnockOver_deg)));
  
  if(!_shouldStreamline){
    
    // Wait for a few frames for block pose to settle down and try turning towards the object again incase
    // our first turn was slightly off
    action->AddAction(new WaitForImagesAction(robot, numFramesToWaitForBeforeFlip));
    
    // Turn towards bottom block of stack
    action->AddAction(new TurnTowardsObjectAction(robot,
                                                  _baseBlockID,
                                                  Radians(PI_F)
                                                  ));
  }
  
  StartActing(action, &BehaviorKnockOverCubes::TransitionToKnockingOverStack);
}

  
void BehaviorKnockOverCubes::TransitionToKnockingOverStack(Robot& robot)
{
  SET_STATE(KnockingOverStack);
  
  robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToUnexpectedMovement, false);
  robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::AcknowledgeObject, false);

  StartActing(new DriveAndFlipBlockAction(robot, _baseBlockID)
              , [this, &robot](const ActionResult& result){
                if(result == ActionResult::SUCCESS){
                  TransitionToPlayingReaction(robot);
                }
              });
}
  
void BehaviorKnockOverCubes::TransitionToPlayingReaction(Robot& robot)
{
  SET_STATE(PlayingReaction);

  if(!_shouldStreamline){
    StartActing(new TriggerAnimationAction(robot, _knockOverSuccessTrigger),
                [this](Robot& robot){
                  robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::AcknowledgeObject, true);
                  SET_STATE(DrivingToStack);

                });
  }
}
  

void BehaviorKnockOverCubes::TranstitionToSettingDownBlock(Robot& robot)
{
  SET_STATE(SettingDownBlock);
  
  if( _putDownAnimTrigger == AnimationTrigger::Count) {
    constexpr float kAmountToReverse_mm = 90.f;
    IActionRunner* actionsToDo = new CompoundActionSequential(robot, {
      new DriveStraightAction(robot, -kAmountToReverse_mm, DEFAULT_PATH_MOTION_PROFILE.speed_mmps),
      new PlaceObjectOnGroundAction(robot)});
    if(!_shouldStreamline){
      StartActing(actionsToDo, &BehaviorKnockOverCubes::TransitionToReachingForBlock);
    }else{
      StartActing(actionsToDo, &BehaviorKnockOverCubes::TransitionToDrivingToReadyPose);
    }
  }
  else {
    StartActing(new TriggerAnimationAction(robot, _putDownAnimTrigger),
                [this](Robot& robot) {
                  // use same logic as put down block behavior
                  StartActing(BehaviorPutDownBlock::CreateLookAfterPlaceAction(robot, false),
                              [this, &robot]() {
                                if(robot.IsCarryingObject()) {
                                  // No matter what, even if we didn't see the object we were
                                  // putting down for some reason, mark the robot as not carrying
                                  // anything so we don't get stuck in a loop of trying to put
                                  // something down (i.e. assume the object is no longer on our lift)
                                  // TODO: We should really be using some kind of PlaceOnGroundAction instead of raw animation (see COZMO-2192)
                                  PRINT_NAMED_WARNING("BehaviorRollBlock.TransitionToSettingDownBlock.DidNotSeeBlock",
                                                      "Forcibly setting carried objects as unattached (See COZMO-2192)");
                                  robot.SetCarriedObjectAsUnattached();
                                }
                                if(!_shouldStreamline){
                                  TransitionToReachingForBlock(robot);
                                }else{
                                  TransitionToDrivingToReadyPose(robot);
                                }
                              });
                });
  }
}

  

  
void BehaviorKnockOverCubes::ResetBehavior(Robot& robot)
{
  _state = State::DrivingToStack;
  _baseBlockID.UnSet();
}

void BehaviorKnockOverCubes::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorRollBlock.TransitionTo", "%s", stateName.c_str());
  SetStateName(stateName);
}

void BehaviorKnockOverCubes::UpdateTargetStack(const Robot& robot) const
{
   _stackHeight = robot.GetBlockWorld().GetTallestStack(_baseBlockID);
}

}
}
