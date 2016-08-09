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
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
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

#define DEBUG_ADMIRE_STACK_BEHAVIOR 0

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

CONSOLE_VAR(f32, kBAS_LookDownToSecond_deg, "Behavior.AdmireStack", 10.0f);


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
  _topBlockLastSeentime = 0;
  _thirdBlockID.UnSet();

  return Result::RESULT_OK;
}

Result BehaviorAdmireStack::ResumeInternal(Robot& robot)
{
  switch(_state) {
    case State::WatchingStack:      
    case State::ReactingToThirdBlock:
    case State::TryingToGrabThirdBlock:
    case State::SearchingForStack:
    case State::LookDownAndUp:
      // restart from the beginning
      return InitInternal(robot);
      
    case State::PreparingToKnockOverStack: {
      // We got an interruption (likely cliff) on the way to our pre-dock pose. Blindly retry for now. This is
      // bad long term because it could end up with an infinite loop of trying over and over again
      PRINT_NAMED_INFO("BehaviorAdmireStack.Resume.PreparingToKnockOver",
                       "Resumed while behavior was preparing to knock over stack. Retrying from this state");
      TransitionToPreparingToKnockOverStack(robot);
      return Result::RESULT_OK;
    }
      
    case State::KnockingOverStack:
    case State::ReactingToTopple:
    case State::KnockingOverStackFailed: {
      // in these cases, we got interrupted while doing the actual knock, or reacting to it, so just make sure
      // we set that we knocked it over (to unblock the demo scene) and fail
      PRINT_NAMED_INFO("BehaviorAdmireStack.Resume.AfterTopple",
                       "Resuming in a state that is too late, just setting knocked over to true and bailing");
      
      robot.GetBehaviorManager().GetWhiteboard().ClearHasStackToAdmire();
      _didKnockOverStack = true;
      _state = State::WatchingStack;
      return Result::RESULT_FAIL;
    }
  }
}

void BehaviorAdmireStack::StopInternal(Robot& robot)
{
  if( _state == State::TryingToGrabThirdBlock ) {
    //Enable Cliff Reaction
    robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToCliff, true);
  }
  
  // If we are being stopped while in the ReactingToTopple state we want to make sure to set _didKnockOverStack to true
  // so we can transition out of this behavior
  if(_state == State::ReactingToTopple)
  {
    PRINT_NAMED_INFO("BehaviorAdmireStack.StopInternal", "Stopping in state ReactToTopple, setting did knock over stack");
    _didKnockOverStack = true;
  }
  
  // don't change _state here because we want to be able to Resume this state
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
      DriveStraightAction* driveAction = new DriveStraightAction(robot, -distToDrive, kBAS_backupForSearchSpeed_mmps, false);
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

  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::AdmireStackReactToThirdCube),
              &BehaviorAdmireStack::TransitionToTryingToGrabThirdBlock);
  IncreaseScoreWhileActing(kBAS_ScoreIncreaseForAction);
}

void BehaviorAdmireStack::TransitionToTryingToGrabThirdBlock(Robot& robot)
{
  SET_STATE(TryingToGrabThirdBlock);

  //Disable Cliff Reaction
  robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToCliff, false);

  ObjectID topPlacedBlock = robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireTopBlockID();
  ObservableObject* secondBlock = robot.GetBlockWorld().GetObjectByID(topPlacedBlock);
  if( nullptr == secondBlock ) {
    PRINT_NAMED_WARNING("BehaviorAdmireStack.NullBlock",
                        "Robot should admire block %d, but can't get pointer",
                        topPlacedBlock.GetValue());
    return;
  }
  
  CompoundActionSequential* action = new CompoundActionSequential(robot);

  Pose3d poseWrtRobot;
  if( secondBlock->GetPose().GetWithRespectTo(robot.GetPose(), poseWrtRobot) ) {
    const float fudgeFactor = 10.0f;
    if( poseWrtRobot.GetTranslation().x() + fudgeFactor > kBAS_distanceToTryToGrabFrom_mm) {
      float distToDrive = poseWrtRobot.GetTranslation().x() - kBAS_distanceToTryToGrabFrom_mm;
      action->AddAction(new DriveStraightAction(robot, distToDrive, kBAS_backupForSearchSpeed_mmps));
    }
  }

  action->AddAction(new TriggerAnimationAction(robot, AnimationTrigger::AdmireStackAttemptGrabThirdCube));

  StartActing(action, &BehaviorAdmireStack::TransitionToPreparingToKnockOverStack);
  IncreaseScoreWhileActing(kBAS_ScoreIncreaseForAction);
}

void BehaviorAdmireStack::TransitionToPreparingToKnockOverStack(Robot& robot)
{
  SET_STATE(PreparingToKnockOverStack);

  //Enable Cliff Reaction
  robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToCliff, true);
  CompoundActionSequential* action = new CompoundActionSequential(robot);

  ObjectID bottomBlockID = robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireBottomBlockID();
  
  // Push angry driving animations
  robot.GetDrivingAnimationHandler().PushDrivingAnimations({AnimationTrigger::DriveStartAngry,
        AnimationTrigger::DriveLoopAngry,
        AnimationTrigger::DriveEndAngry});
  
  // Backup
  DriveStraightAction* backupAction = new DriveStraightAction(robot, -kBAS_backupDist_mm, kBAS_backupSpeed_mmps);
  backupAction->SetAccel(kBAS_backupAccel_mmps2);
  backupAction->SetDecel(kBAS_backupDecel_mmps2);
  action->AddAction(backupAction);
  
  // Drive to preDockPose for flipping
  action->AddAction(new DriveToFlipBlockPoseAction(robot, bottomBlockID));
  
  // Look at face
  action->AddAction(new TurnTowardsFaceWrapperAction(robot,
                                                     new TriggerAnimationAction(robot, AnimationTrigger::AdmireStackFocusEyes)));
  
  // Turn towards bottom block of stack
  TurnTowardsObjectAction* turnTowardsObjectAction = new TurnTowardsObjectAction(robot,
                                                                                 bottomBlockID,
                                                                                 Radians(PI_F),
                                                                                 false, // verify when done?
                                                                                 false);
  turnTowardsObjectAction->SetRefinedTurnAngleTol(kTurnTowardsObjectAngleTol_rad);
  action->AddAction(turnTowardsObjectAction);

  // retry this action, then give up
  action->SetNumRetries(1);

  StartActing(action, [this, &robot](ActionResult res) {
      if( res == ActionResult::SUCCESS ) {
        TransitionToKnockingOverStack(robot);
      }
      else {
        // couldn't get into position to knock over the stack, Try one last time
        TransitionToKnockingOverStackFailed(robot);
      }
    });
  
  IncreaseScoreWhileActing(kBAS_ScoreIncreaseForAction);
}

void BehaviorAdmireStack::TransitionToKnockingOverStack(Robot& robot)
{
  SET_STATE(KnockingOverStack);

  ObjectID bottomBlockID = robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireBottomBlockID();
  
  FlipBlockAction* flipBlockAction = new FlipBlockAction(robot, bottomBlockID);

  StartActing(flipBlockAction, [this, &robot](ActionResult res) {
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

  // Only consider the stack knocked over when the knock over success animation has completed
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::AdmireStackKnockOverSuccess), [this]()
  {
    PRINT_NAMED_INFO("BehaviorAdmireStack.TransitionToReactingToTopple.Callback", "Setting did knock over stack");
    _didKnockOverStack = true;
  });
  IncreaseScoreWhileActing(kBAS_ScoreIncreaseForAction);

  robot.GetBehaviorManager().GetWhiteboard().ClearHasStackToAdmire();
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
        robot.GetBehaviorManager().GetWhiteboard().ClearHasStackToAdmire();
        PRINT_NAMED_DEBUG("BehaviorAdmireStack.Searching.Failed",
                          "couldn't find stack, leaving behavior");
      }
    });
}

void BehaviorAdmireStack::TransitionToKnockingOverStackFailed(Robot& robot)
{
  SET_STATE(KnockingOverStackFailed);
  
  ObjectID bottomBlockID = robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireBottomBlockID();
  // When retrying the FlipBlockAction we want to use the closest preAction pose and not drive anywhere
  DriveAndFlipBlockAction* action = new DriveAndFlipBlockAction(robot, bottomBlockID);
  action->ShouldDriveToClosestPreActionPose(true);
  StartActing(action, [this, &robot](ActionResult res) {
    if(res == ActionResult::SUCCESS)
    {
      TransitionToReactingToTopple(robot);
    }

    // this was a hail mary anyway, so just assume it worked. This is a bit of a hack for the PR demo so we
    // don't get stuck thinking this behavior didn't run (and fail to advance to the game request)
    _didKnockOverStack = true;
  });
}

void BehaviorAdmireStack::TransitionToLookDownAndUp(Robot& robot)
{
  SET_STATE(LookDownAndUp);
  
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  
  // Moves head to look down at second block in stack
  action->AddAction(new MoveHeadToAngleAction(robot, robot.GetHeadAngle()-DEG_TO_RAD_F32(kBAS_LookDownToSecond_deg)));
  
  // Verify we are seeing the second block, if we aren't then something is very wrong
  action->AddAction(new VisuallyVerifyObjectAction(robot, robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireTopBlockID()));
  
  // Move head back to starting position to react to third block
  action->AddAction(new MoveHeadToAngleAction(robot, robot.GetHeadAngle()));
  
  // If this compound action fails it will more than likely be due to the visual verify in
  // which case we are probably not able to see the second block in which case something is wrong
  // and we will search for the stack
  // Otherwise we verified the second block is there and updated its pose so we should go back to
  // watching the stack so the GetBlockOnTopOf check can hopefully succeed
  StartActing(action, kBAS_ScoreIncreaseForAction, [this, &robot](ActionResult res)
  {
    if(res == ActionResult::SUCCESS)
    {
      TransitionToWatchingStack(robot);
    }
    else
    {
      TransitionToSearchingForStack(robot);
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

  if( ! msg.markersVisible ) {
    return;
  }

  ObjectID secondBlockID = robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireTopBlockID();

  ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(msg.objectID);
  if( nullptr == obj  ) {
    PRINT_NAMED_ERROR("BehaviorAdmireStack.HandleBlockUpdate.NoBlockPointer",
                      "object id %d sent a message but can't get it's pointer",
                      msg.objectID);
    return;
  }
  
  // If we're in the states following or during the knock-over-stack actions, ignore objects we're observing
  if (State::PreparingToKnockOverStack == _state ||
      State::KnockingOverStack == _state ||     
      State::KnockingOverStackFailed == _state ||
      State::ReactingToTopple == _state)
  {
    PRINT_NAMED_INFO("BehaviorAdmireStack.HandleBlockUpdate.NotRightState","Not right state to respond to object observed");
    return;
  }

  if( _state == State::WatchingStack )
  {
    // check if this could possibly be the 3rd stacked block
    if(msg.objectFamily == ObjectFamily::LightCube &&
       msg.objectID != secondBlockID &&
       msg.objectID != robot.GetBehaviorManager().GetWhiteboard().GetStackToAdmireBottomBlockID())
    {
      if(DEBUG_ADMIRE_STACK_BEHAVIOR && secondBlockID.IsUnknown())
      {
        PRINT_NAMED_INFO("BehaviorAdmireStack.HandleObjectObserved.SecondBlockUnSet",
                          "In WatchingStack state with no second block ID set");
      }

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
      } else {
        // We are seeing the third block but for whatever reason we don't think it is on top of the second
        // we should look down and up to update the pose of the second and verify it is there
        StopActing(false);
        TransitionToLookDownAndUp(robot);
        
        if(DEBUG_ADMIRE_STACK_BEHAVIOR) {
          PRINT_NAMED_INFO("BehaviorAdmireStack.HandleBlockUpdate.NewBlockNotOnTopBlock",
                            "%s with ID %d at (%.1f,%.1f,%.1f) not on top of second block %s %d at(%.1f,%.1f,%.1f) looking down at second block",
                            EnumToString(msg.objectType), msg.objectID,
                            obj->GetPose().GetTranslation().x(),
                            obj->GetPose().GetTranslation().y(),
                            obj->GetPose().GetTranslation().z(),
                            EnumToString(secondBlock->GetType()), secondBlockID.GetValue(),
                            secondBlock->GetPose().GetTranslation().x(),
                            secondBlock->GetPose().GetTranslation().y(),
                            secondBlock->GetPose().GetTranslation().z());
        
        }
      }
    }
    else if(DEBUG_ADMIRE_STACK_BEHAVIOR) {
      PRINT_NAMED_INFO("BehaviorAdmireStack.HandleBlockUpdate.SawNewBlockWhileWatching",
                        "Saw %s with ID %d with markers visible while watching stack",
                        EnumToString(msg.objectType), msg.objectID);
    }
  }
  else if( msg.objectID == secondBlockID )
  {
    _topBlockLastSeentime = msg.timestamp;

    const bool upAxisOk = ! robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::RollCube) ||
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

}
}

