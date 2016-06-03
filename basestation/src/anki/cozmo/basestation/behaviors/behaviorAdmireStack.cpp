/**
 * File: behaviorAdmireStack.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-05-12
 *
 * Description: Behavior to look at and admire a stack of blocks, then knock it over if a 3rd is added
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorAdmireStack.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/flipBlockAction.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/blockWorldFilter.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/console/consoleInterface.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

// Whether or not we need to have recently seen the 2nd block when watching stack for 3rd block
#define NEED_TO_SEE_SECOND_BLOCK 0

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(u32, kBAS_maxBlockAge_ms, "Behavior.AdmireStack", 500);

CONSOLE_VAR(f32, kBAS_backupAccel_mmps2, "Behavior.AdmireStack", 100.0f);
CONSOLE_VAR(f32, kBAS_backupDecel_mmps2, "Behavior.AdmireStack", 500.0f);
CONSOLE_VAR(f32, kBAS_backupDist_mm, "Behavior.AdmireStack", 100.0f);
CONSOLE_VAR(f32, kBAS_backupForSearchDist_mm, "Behavior.AdmireStack", 50.0f);
CONSOLE_VAR(f32, kBAS_backupForSearchSpeed_mmps, "Behavior.AdmireStack", 60.0f);
CONSOLE_VAR(f32, kBAS_backupSpeed_mmps, "Behavior.AdmireStack", 50.0f);
CONSOLE_VAR(f32, kBAS_distanceToTryToGrabFrom_mm, "Behavior.AdmireStack", 85.0f);
CONSOLE_VAR(f32, kBAS_driveThroughAccel_mmps2, "Behavior.AdmireStack", 500.0f);
CONSOLE_VAR(f32, kBAS_driveThroughDecel_mmps2, "Behavior.AdmireStack", 100.0f);
CONSOLE_VAR(f32, kBAS_driveThroughDist_mm, "Behavior.AdmireStack", 300.0f);
CONSOLE_VAR(f32, kBAS_driveThroughSpeed_mmps, "Behavior.AdmireStack", 180.0f);
CONSOLE_VAR(f32, kBAS_headAngleForKnockOver_deg, "Behavior.AdmireStack", -14.0f);
CONSOLE_VAR(f32, kBAS_minHeadAngleForSearch_deg, "Behavior.AdmireStack", 10.0f);
CONSOLE_VAR(f32, kBAS_maxHeadAngleForSearch_deg, "Behavior.AdmireStack", 25.0f);
CONSOLE_VAR(f32, kBAS_betweenHeadAnglePause_s, "Behavior.AdmireStack", 1.0f);
CONSOLE_VAR(f32, kBAS_minDistanceFromStack_mm, "Behavior.AdmireStack", 130.0f);

CONSOLE_VAR(f32, kBAS_ScoreIncreaseForAction, "Behavior.AdmireStack", 0.8f);

const char* const kFocusEyesForKnockOverAnim = "anim_ReactToBlock_focusedEyes_01";

const std::string kDrivingAngryStartAnim = "ag_driving_upset_start";
const std::string kDrivingAngryLoopAnim  = "ag_driving_upset_Loop";
const std::string kDrivingAngryEndAnim   = "ag_driving_upset_end";

BehaviorAdmireStack::BehaviorAdmireStack(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  SetDefaultName("AdmireStack");

  SubscribeToTags({
    EngineToGameTag::RobotObservedObject
  });  
}

bool BehaviorAdmireStack::IsRunnableInternal(const Robot& robot) const
{
  if( robot.GetBehaviorManager().GetWhiteboard().HasStackToAdmire() ) {
    ObjectID bottomBlockID = robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireBottomBlockID();
    const ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(bottomBlockID);
    if( nullptr != obj ) {
      // run if we have a stack to admire and we know where it is
      return obj->IsPoseStateKnown();
    }
  }

  return false;
}

Result BehaviorAdmireStack::InitInternal(Robot& robot)
{
  //_topPlacedBlock = robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireTopBlockID();
  TransitionToWatchingStack(robot);
  _didKnockOverStack = false;

  return Result::RESULT_OK;
}

void BehaviorAdmireStack::StopInternal(Robot& robot)
{
  if( _state == State::TryingToGrabThirdBlock ) {
    robot.GetBehaviorManager().GetWhiteboard().RequestEnableCliffReaction(this);
  }
  
  ResetBehavior(robot);
}

IBehavior::Status BehaviorAdmireStack::UpdateInternal(Robot& robot)
{
  if( NEED_TO_SEE_SECOND_BLOCK && _state == State::WatchingStack ) {
    // make sure we have seen the second cube recently enough
    TimeStamp_t currTime = robot.GetLastImageTimeStamp();
    if( currTime > _topBlockLastSeentime + kBAS_maxBlockAge_ms ) {
      PRINT_NAMED_DEBUG("BehaviorAdmireStack.LostSecondBlock",
                        "haven't seen second block (id %d) since %d (curr=%d), searching",
                        robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireTopBlockID().GetValue(),
                        _topBlockLastSeentime,
                        currTime);
      StopActing(false);
      TransitionToSearchingForStack(robot);
    }
  }

  return IBehavior::UpdateInternal(robot);
}

void BehaviorAdmireStack::TransitionToWatchingStack(Robot& robot)
{
  SET_STATE(WatchingStack);

  ObjectID topPlacedBlock = robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireTopBlockID();
  ObservableObject* secondBlock = robot.GetBlockWorld().GetObjectByID(topPlacedBlock);
  if( nullptr == secondBlock ) {
    PRINT_NAMED_WARNING("BehaviorAdmireStack.NullBlock",
                        "Robot should admire block %d, but can't get pointer",
                        topPlacedBlock.GetValue());
    return;
  }


  CompoundActionSequential* action = new CompoundActionSequential(robot);
  
  // if we're too close to the cube, back up
  Pose3d poseWrtRobot;
  if( secondBlock->GetPose().GetWithRespectTo(robot.GetPose(), poseWrtRobot) ) {
    const float fudgeFactor = 10.0f;
    if( poseWrtRobot.GetTranslation().x() < kBAS_minDistanceFromStack_mm + fudgeFactor) {
      float distToDrive = kBAS_minDistanceFromStack_mm - poseWrtRobot.GetTranslation().x();
      DriveStraightAction* driveAction = new DriveStraightAction(robot, -distToDrive, kBAS_backupForSearchSpeed_mmps);
      driveAction->SetShouldPlayDrivingAnimation(false);
      action->AddAction(driveAction);
    }
  }

  // look right in between where the current top (2nd) block is and where the 3rd would go
  
  Pose3d lookPose = secondBlock->GetPose();
  lookPose.SetTranslation( { secondBlock->GetPose().GetTranslation().x(),
        secondBlock->GetPose().GetTranslation().y(),
        secondBlock->GetPose().GetTranslation().z() + 0.5f * secondBlock->GetSize().z() });

  action->AddAction(new TurnTowardsPoseAction(robot, lookPose, Radians(PI_F)));
  
  // TODO:(bn) looping animation?
  action->AddAction(new HangAction(robot));

  // will hang in this state until we see the third cube
  StartActing(action, kBAS_ScoreIncreaseForAction);
}

void BehaviorAdmireStack::TransitionToReactingToThirdBlock(Robot& robot)
{
  SET_STATE(ReactingToThirdBlock);

  StartActing(new PlayAnimationGroupAction(robot, _reactToThirdCubeAnimGroup),
              &BehaviorAdmireStack::TransitionToTryingToGrabThirdBlock);
  IncreaseScoreWhileActing(kBAS_ScoreIncreaseForAction);
}

void BehaviorAdmireStack::TransitionToTryingToGrabThirdBlock(Robot& robot)
{
  SET_STATE(TryingToGrabThirdBlock);

  robot.GetBehaviorManager().GetWhiteboard().DisableCliffReaction(this);
    
  CompoundActionSequential* action = new CompoundActionSequential(robot);

  ObjectID topPlacedBlock = robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireTopBlockID();
  ObservableObject* secondBlock = robot.GetBlockWorld().GetObjectByID(topPlacedBlock);
  if( nullptr == secondBlock ) {
    PRINT_NAMED_WARNING("BehaviorAdmireStack.NullBlock",
                        "Robot should admire block %d, but can't get pointer",
                        topPlacedBlock.GetValue());
    return;
  }

  Pose3d poseWrtRobot;
  if( secondBlock->GetPose().GetWithRespectTo(robot.GetPose(), poseWrtRobot) ) {
    const float fudgeFactor = 10.0f;
    if( poseWrtRobot.GetTranslation().x() + fudgeFactor > kBAS_distanceToTryToGrabFrom_mm) {
      float distToDrive = poseWrtRobot.GetTranslation().x() - kBAS_distanceToTryToGrabFrom_mm;
      action->AddAction(new DriveStraightAction(robot, distToDrive, kBAS_backupForSearchSpeed_mmps));
    }
  }

  action->AddAction(new PlayAnimationGroupAction(robot, _tryToGrabThirdCubeAnimGroup));

  StartActing(action, &BehaviorAdmireStack::TransitionToKnockingOverStack);
  IncreaseScoreWhileActing(kBAS_ScoreIncreaseForAction);
}

void BehaviorAdmireStack::TransitionToKnockingOverStack(Robot& robot)
{
  SET_STATE(KnockingOverStack);

  robot.GetBehaviorManager().GetWhiteboard().RequestEnableCliffReaction(this);

  CompoundActionSequential* action = new CompoundActionSequential(robot);

  ObjectID bottomBlockID = robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireBottomBlockID();
  
  // Push angry driving animations
  robot.GetDrivingAnimationHandler().PushDrivingAnimations({kDrivingAngryStartAnim,
                                                            kDrivingAngryLoopAnim,
                                                            kDrivingAngryEndAnim});
  
  // Backup
  DriveStraightAction* backupAction = new DriveStraightAction(robot, -kBAS_backupDist_mm, kBAS_backupSpeed_mmps);
  backupAction->SetAccel(kBAS_backupAccel_mmps2);
  backupAction->SetDecel(kBAS_backupDecel_mmps2);
  action->AddAction(backupAction);
  
  // Drive to preDockPose for flipping
  action->AddAction(new DriveToFlipBlockPoseAction(robot, bottomBlockID));
  
  // Look at face
  action->AddAction(new TurnTowardsFaceWrapperAction(robot,
                                                     new PlayAnimationAction(robot, kFocusEyesForKnockOverAnim)));
  
  // Turn towards bottom block of stack
  action->AddAction(new TurnTowardsObjectAction(robot,
                                                bottomBlockID,
                                                Radians(PI_F),
                                                false, // verify when done?
                                                false));
  
  // Move head for knock over
  action->AddAction(new MoveHeadToAngleAction(robot, DEG_TO_RAD(kBAS_headAngleForKnockOver_deg)));
  
  // Wait for a few frames for block pose to settle down and try turning towards the object again incase
  // our first turn was slightly off
  action->AddAction(new WaitForImagesAction(robot, numFramesToWaitForBeforeFlip));
  
  // Turn towards bottom block of stack
  action->AddAction(new TurnTowardsObjectAction(robot,
                                                bottomBlockID,
                                                Radians(PI_F),
                                                false, // verify when done?
                                                false));
  
  // Knock over
  FlipBlockAction* flipBlockAction = new FlipBlockAction(robot, bottomBlockID);
  action->AddAction(flipBlockAction);

  StartActing(action, [this, &robot](ActionResult res) {
      if( res == ActionResult::SUCCESS ) {
        TransitionToReactingToTopple(robot);
      } else {
        TransitionToKnockingOverStackFailed(robot);
      }
    
      // Pop the angry driving animations
      robot.GetDrivingAnimationHandler().PopDrivingAnimations();
    });
  IncreaseScoreWhileActing(kBAS_ScoreIncreaseForAction);
}

void BehaviorAdmireStack::TransitionToReactingToTopple(Robot& robot)
{
  SET_STATE(ReactingToTopple);
  
  robot.GetBlockWorld().ClearObject(robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireBottomBlockID());
  robot.GetBlockWorld().ClearObject(robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireTopBlockID());
  robot.GetBlockWorld().ClearObject(_thirdBlockID);

  StartActing(new PlayAnimationGroupAction(robot, _succesAnimGroup));
  IncreaseScoreWhileActing(kBAS_ScoreIncreaseForAction);

  robot.GetBehaviorManager().GetWhiteboard().ClearHasStackToAdmire();
  _didKnockOverStack = true;
}

void BehaviorAdmireStack::TransitionToSearchingForStack(Robot& robot)
{
  SET_STATE(SearchingForStack);

  TimeStamp_t currLastSeenTime = _topBlockLastSeentime;

  StartActing(new CompoundActionSequential(robot, {
        new MoveHeadToAngleAction(robot, DEG_TO_RAD( kBAS_minHeadAngleForSearch_deg )),
        new WaitAction(robot, kBAS_betweenHeadAnglePause_s),
        new MoveHeadToAngleAction(robot, DEG_TO_RAD( kBAS_maxHeadAngleForSearch_deg )),
        new DriveStraightAction(robot, -kBAS_backupForSearchDist_mm, kBAS_backupForSearchSpeed_mmps),
        new SearchSideToSideAction(robot) }),
    [this, currLastSeenTime](Robot& robot) {
      // check if we've seen the cube more recently than before
      if( _topBlockLastSeentime > currLastSeenTime ) {
        PRINT_NAMED_DEBUG("BehaviorAdmireStack.Searching.Success",
                          "found a top block, resetting behavior");
        TransitionToWatchingStack(robot);
      }
      else {
        PRINT_NAMED_DEBUG("BehaviorAdmireStack.Searching.Failed",
                          "couldn't find stack, leaving behavior");
      }
    });
}

void BehaviorAdmireStack::TransitionToKnockingOverStackFailed(Robot& robot)
{
  SET_STATE(KnockingOverStackFailed);
  
  ObjectID bottomBlockID = robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireBottomBlockID();
  DriveAndFlipBlockAction* action = new DriveAndFlipBlockAction(robot, bottomBlockID);
  StartActing(action, [this, &robot](ActionResult res) {
    if(res == ActionResult::SUCCESS)
    {
      TransitionToReactingToTopple(robot);
    }
  });
}

void BehaviorAdmireStack::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  ASSERT_NAMED(event.GetData().GetTag() == EngineToGameTag::RobotObservedObject,
               "BehaviorAdmireStack.InvalidMessageTag");

  if( ! robot.GetBehaviorManager().GetWhiteboard().HasStackToAdmire() ) {
    return;
  }  

  const auto& msg = event.GetData().Get_RobotObservedObject();

  ObjectID secondBlockID = robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireTopBlockID();

  ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(msg.objectID);
  if( nullptr == obj  ) {
    PRINT_NAMED_ERROR("BehaviorAdmireStack.HandleBlockUpdate.NoBlockPointer",
                      "object id %d sent a message but can't get it's pointer",
                      msg.objectID);
    return;
  }

  if( _state == State::WatchingStack ){
    // check if this could possibly be the 3rd stacked block
    if( msg.markersVisible &&
       msg.objectFamily == ObjectFamily::LightCube ) {
      
      ObservableObject* secondBlock = robot.GetBlockWorld().GetObjectByID( secondBlockID );
      if( nullptr == secondBlock ) {
        PRINT_NAMED_ERROR("BehaviorAdmireStack.HandleBlockUpdate.NoSecondBlockPointer",
                          "couldn't get object id %d pointer",
                          secondBlockID.GetValue());
        return;
      }
      
      const float kZTolerance_mm = 15.0f;
      
      if( robot.GetBlockWorld().FindObjectOnTopOf( *secondBlock, kZTolerance_mm ) == obj ) {
        // TODO:(bn) should check ! IsMoving here, but it's slow / unreliable
        PRINT_NAMED_INFO("BehaviorAdmireStack.HandleObjectObserved.FoundThirdBlock",
                         "Found that block %d appears to be the top of a 3 stack",
                         msg.objectID);
        _thirdBlockID = msg.objectID;
        StopActing(false);
        TransitionToReactingToThirdBlock(robot);
      }
    }
  }
  else if( msg.objectID == robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireTopBlockID() &&
      msg.markersVisible ) {
    _topBlockLastSeentime = msg.timestamp;

    const bool upAxisOk = ! robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::CubeRollAction) ||
      obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS;

    if( ! upAxisOk ) {
      PRINT_NAMED_DEBUG("BehaviorAdmireStack.HandleBlockUpdate.StackMessedWith",
                        "Top block rotated, stopping behavior");
      // TODO:(bn) play animation here?
      robot.GetBehaviorManager().GetWhiteboard().ClearHasStackToAdmire();
      StopActing(false);
      return;
    }
    else if ( _state == State::SearchingForStack ) {
      PRINT_NAMED_DEBUG("BehaviorAdmireStack.FoundDuringSearch",
                        "found top block while searching");
      StopActing(false);
      TransitionToWatchingStack(robot);
    }
  }
}


void BehaviorAdmireStack::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorAdmireStack.TransitionTo", "%s", stateName.c_str());
  SetStateName(stateName);
}

void BehaviorAdmireStack::ResetBehavior(Robot& robot)
{
  _state = State::WatchingStack;
}

}
}

