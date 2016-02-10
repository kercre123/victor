/**
 * File: behaviorRequestGameSimple.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-02-03
 *
 * Description: re-usable game request behavior which works with or without blocks
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/behaviors/gameRequest/behaviorRequestGameSimple.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

static const char* kInitialAnimationKey = "initial_animName";
static const char* kPreDriveAnimationKey = "preDrive_animName";
static const float kDistToMoveTowardsFace_mm = 120.0f;
static const float kBackupDistance_mm = 80.0f;
static const float kFaceVerificationTime_s = 0.5f;

BehaviorRequestGameSimple::BehaviorRequestGameSimple(Robot& robot, const Json::Value& config)
  : IBehaviorRequestGame(robot, config)
{
  SetDefaultName("BehaviorRequestGameSimple");

  if( config.isNull() ) {
    PRINT_NAMED_WARNING("BehaviorRequestGameSimple.Config.Error",
                        "Empty json config! This behavior will not function correctly");
  }
  else {
    {
      const Json::Value& val = config[kInitialAnimationKey];
      if( val.isString() ) {
        _initialAnimationName = val.asCString();
      }
      else {
        PRINT_NAMED_WARNING("BehaviorRequestGame.Config.MissingKey",
                            "Missing key '%s'",
                            kInitialAnimationKey);
      }
    }

    {
      const Json::Value& val = config[kPreDriveAnimationKey];
      if( val.isString() ) {
        _preDriveAnimationName = val.asCString();
      }
      else {
        PRINT_NAMED_WARNING("BehaviorRequestGame.Config.MissingKey",
                            "Missing key '%s'",
                            kPreDriveAnimationKey);
      }
    }
      
  }
}

Result BehaviorRequestGameSimple::RequestGame_InitInternal(Robot& robot,
                                                             double currentTime_sec)
{
  _verifyStartTime_s = std::numeric_limits<float>::max();
    
  if( ! IsActing() ) {
    if( GetNumBlocksRequired() == 0 ) {
      // skip the block stuff and go right to the face
      TransitionTo(robot, State::LookingAtFace);
    }
    else {
      TransitionTo(robot, State::PlayingInitialAnimation);
    }
  }

  return RESULT_OK;
}

IBehavior::Status BehaviorRequestGameSimple::UpdateInternal(Robot& robot, double currentTime_sec)
{
  if( IsActing() ) {
    return Status::Running;
  }

  PRINT_NAMED_DEBUG("BehaviorRequestGameSimple.Complete", "no current actions, so finishing");

  return Status::Complete;
}

Result BehaviorRequestGameSimple::InterruptInternal(Robot& robot, double currentTime_sec)
{
  // if we are playing an animation, we don't want to be interrupted
  if( IsActing() ) {
    if( _state == State::TrackingFace ) {
      if( GetRequestMinDelayComplete_s() >= 0.0f && currentTime_sec < GetRequestMinDelayComplete_s() ) {
        // we are doing a face track action, and it hasn't been long enough since the request, so hold in this
        // behavior for a while
        return Result::RESULT_FAIL;
      }
    }
    else {
      // we are doing something, wait until we are done
      return Result::RESULT_FAIL;
    }
  }

  StopInternal(robot, currentTime_sec);

  return Result::RESULT_OK;
}

void BehaviorRequestGameSimple::StopInternal(Robot& robot, double currentTime_sec)
{
  PRINT_NAMED_INFO("BehaviorRequestGameSimple.StopInternal", "");

  if( _state == State::TrackingFace ) {
    // this means we have send up the game request, so now we should send a Deny to cancel it
    PRINT_NAMED_INFO("BehaviorRequestGameSimple.DenyRequest",
                     "behavior is denying it's own request");

    SendDeny(robot);    
    CancelAction(robot);
  }

  // don't use transition to because we don't want to do anything
  _state = State::PlayingInitialAnimation;
}

void BehaviorRequestGameSimple::TransitionTo(Robot& robot, State state)
{
  std::string name = "UNKNOWN";
  bool transitioned = true;
  
  switch(state) {
    case State::PlayingInitialAnimation: {
      name = "PlayingInitialAnimation";
      IActionRunner* animationAction = new PlayAnimationAction(_initialAnimationName);            
      StartActing( robot, animationAction );
      break;
    }

    case State::FacingBlock: {
      name = "FacingBlock";
      ObjectID targetBlockID = GetRobotsBlockID(robot);
      StartActing(robot, new FaceObjectAction( targetBlockID, PI_F ) );
      break;
    }

    case State::PlayingPreDriveAnimation: {
      name = "PlayingPreDriveAnimation";
      IActionRunner* animationAction = new PlayAnimationAction(_preDriveAnimationName);
      StartActing( robot, animationAction );
      break;
    }

    case State::PickingUpBlock: {
      name = "PickingUpBlock";
      ObjectID targetBlockID = GetRobotsBlockID(robot);
      StartActing(robot, new DriveToPickupObjectAction(targetBlockID));
      break;
    }

    case State::DrivingToFace: {
      name = "DrivingToFace";
      Pose3d faceInteractionPose;
      if( GetFaceInteractionPose(robot, faceInteractionPose) ) {
        StartActing(robot, new DriveToPoseAction( faceInteractionPose ) );
      }
      else {
        transitioned = false;
      }
      break;
    }

    case State::PlacingBlock: {
      name = "PlacingBlock";
      StartActing(robot, new CompoundActionSequential({
                           new PlaceObjectOnGroundAction(),
                           // TODO:(bn) use same motion profile here
                           new DriveStraightAction(-kBackupDistance_mm, -80.0f)
                         }));
      break;
    }

    case State::LookingAtFace: {
      name = "LookingAtFace";
      Pose3d lastFacePose;
      if( GetFacePose( robot, lastFacePose ) ) {
        StartActing(robot, new FacePoseAction(lastFacePose, PI_F));
      }
      else {
        transitioned = false;
      }
      break;
    }

    case State::VerifyingFace: {
      name = "VerifyingFace";
      _verifyStartTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      StartActing(robot, new WaitAction( kFaceVerificationTime_s ) );
      break;
    }

    case State::PlayingRequstAnim: {
      name = "PlayingRequstAnim";
      StartActing(robot, new PlayAnimationAction(_requestAnimationName));
      break;
    }

    case State::TrackingFace: {
      name = "TrackingFace";
      if( GetFaceID() == Face::UnknownFace ) {
        PRINT_NAMED_WARNING("BehaviorRequestGameSimple.NoValidFace",
                            "Can't do face tracking because there is no valid face!");
        // use an action that just hangs to simulate the track face logic
        StartActing( robot, new HangAction() );
      }
      else {
        StartActing( robot, new TrackFaceAction(GetFaceID()) );
      }
      break;
    }

    case State::PlayingDenyAnim: {
      name = "PlayingDenyAnim";
      StartActing( robot, new PlayAnimationAction( _denyAnimationName ) );
      break;
    }

    // no default so the compiler yells at us if we forget a state
  }


  if( transitioned ) {
    _state = state;
    PRINT_NAMED_DEBUG("BehaviorRequestGameSimple.TransitionTo", "%s", name.c_str());
  }
  else {
    PRINT_NAMED_INFO("BehaviorRequestGameSimple.TransitionTo.FailedTransition",
                     "failed to transition to '%s'", name.c_str());
  }  
}  


void BehaviorRequestGameSimple::RequestGame_HandleActionCompleted(Robot& robot, ActionResult result)
{
  switch( _state ) {
    case State::PlayingInitialAnimation: {
      TransitionTo(robot, State::FacingBlock);
      break;
    }

    case State::FacingBlock: {
      TransitionTo(robot, State::PlayingPreDriveAnimation);
      break;
    }

    case State::PlayingPreDriveAnimation: {
      TransitionTo(robot, State::PickingUpBlock);
      break;
    }
      
    case State::PickingUpBlock: {
      
      if ( result == ActionResult::SUCCESS ) {
        TransitionTo(robot, State::DrivingToFace);
      }
      else if ( result == ActionResult::FAILURE_RETRY ) {
        TransitionTo(robot, State::PickingUpBlock);
      }
      else {
        // if its an abort failure, do nothing, which will cause the behavior to stop
        PRINT_NAMED_INFO("BehaviorRequestGameSimple.PickingUpBlock.Failed",
                         "failed to pick up block with no retry, so ending the behavior");
      }

      break;
    }
      
    case State::DrivingToFace: {

      if ( result == ActionResult::SUCCESS ) {
        TransitionTo(robot, State::PlacingBlock);
      }
      else if (result == ActionResult::FAILURE_RETRY) {
        TransitionTo(robot, State::DrivingToFace);
      }
      else {
        // if its an abort failure, do nothing, which will cause the behavior to stop
        PRINT_NAMED_INFO("BehaviorRequestGameSimple.DriveToFace.Failed",
                         "failed to drive to a spot to interact with the face, so ending the behavior");
      }
      
      break;
    }
      
    case State::PlacingBlock: {
      if ( result == ActionResult::SUCCESS ) {
        TransitionTo(robot, State::LookingAtFace);
      }
      else if (result == ActionResult::FAILURE_RETRY) {
        TransitionTo(robot, State::PlacingBlock);
      }
      else {

        // the place action can fail if visual verify fails (it doesn't see the cube after it drops it). For
        // now, if it fails, just keep going anyway
        
        // if its an abort failure, do nothing, which will cause the behavior to stop
        PRINT_NAMED_INFO("BehaviorRequestGameSimple.PlacingBlock.Failed",
                         "failed to place the block on the ground, but pretending it didn't");

        TransitionTo(robot, State::LookingAtFace);
      }
      break;
    }

    case State::LookingAtFace: {
      if( result == ActionResult::SUCCESS ) {
        TransitionTo(robot, State::VerifyingFace);
      }
      else if (result == ActionResult::FAILURE_RETRY) {
        TransitionTo(robot, State::LookingAtFace);
      }
      else {
        PRINT_NAMED_INFO("BehaviorRequestGameSimple.LookingAtFace.Failed",
                         "failed to look at face after dropping block, so ending the behavior");
      }
      break;
    }

    case State::VerifyingFace: {
      if( result == ActionResult::SUCCESS && GetLastSeenFaceTime() >= _verifyStartTime_s ) {
        TransitionTo(robot, State::PlayingRequstAnim);
      }
      else {
        // the face must not have been where we expected, so drop out of the behavior for now
        // TODO:(bn) try to bring the block to a different face if we have more than one?
        PRINT_NAMED_INFO("BehaviorRequestGameSimple.VerifyingFace.Failed",
                         "failed to verify the face, so considering this a rejection");
        TransitionTo(robot, State::PlayingDenyAnim);
      }
      break;
    }
      
      
    case State::PlayingRequstAnim: {
      SendRequest(robot);
      TransitionTo(robot, State::TrackingFace);
      break;
    }
      
    case State::TrackingFace: {
      PRINT_NAMED_INFO("BehaviorRequestGameSimple.HandleActionComplete.TrackingActionEnded",
                       "Someone ended the tracking action, behavior will terminate");
      break;
    }
      
    case State::PlayingDenyAnim: {
      break;
    }
  }
}


bool BehaviorRequestGameSimple::GetFaceInteractionPose(Robot& robot, Pose3d& targetPose)
{
  Pose3d facePose;
  if ( ! GetFacePose(robot, facePose ) ) {
    PRINT_NAMED_WARNING("BehaviorRequestGameSimple.NoFace",
                        "Face pose is invalid!");
    return false;
  }
  
  if( ! facePose.GetWithRespectTo(robot.GetPose(), facePose) ) {
    PRINT_NAMED_WARNING("BehaviorRequestGameSimple.NoFacePose",
                        "could not get face pose with respect to robot");
    return false;
  }

  float xyDistToFace = std::sqrt( std::pow( facePose.GetTranslation().x(), 2) +
                                  std::pow( facePose.GetTranslation().y(), 2) );
  float distanceRatio = kDistToMoveTowardsFace_mm / xyDistToFace;
  
  float relX = distanceRatio * facePose.GetTranslation().x();
  float relY = distanceRatio * facePose.GetTranslation().y();
  Radians targetAngle = std::atan2(relY, relX);

  targetPose = Pose3d{ targetAngle, Z_AXIS_3D(), {relX, relY, 0.0f}, &robot.GetPose() };

  return true;
}

void BehaviorRequestGameSimple::HandleGameDeniedRequest(Robot& robot)
{
  CancelAction(robot);

  TransitionTo(robot, State::PlayingDenyAnim);
}

}
}

