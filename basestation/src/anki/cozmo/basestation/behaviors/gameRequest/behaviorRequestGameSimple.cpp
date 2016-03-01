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
#include "anki/cozmo/basestation/pathMotionProfileHelpers.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

#define SET_STATE(s) SetState_internal(s, #s)

static const char* kInitialAnimationKey = "initial_animName";
static const char* kPreDriveAnimationKey = "preDrive_animName";
static const char* kRequestAnimNameKey = "request_animName";
static const char* kDenyAnimNameKey = "deny_animName";
static const char* kMinRequestDelayKey = "minRequestDelay_s";
static const char* kScoreFactorKey = "score_factor";
static const char* kZeroBlockGroupKey =  "zero_block_config";
static const char* kOneBlockGroupKey =  "one_block_config";
static const char* kAfterPlaceBackupDistance_mmKey = "after_place_backup_dist_mm";
static const char* kAfterPlaceBackupSpeed_mmpsKey = "after_place_backup_speed_mmps";
static const char* kPickupMotionProfileKey = "pickup_motion_profile";
static const char* kPlaceMotionProfileKey = "place_motion_profile";
static const char* kDriveToPlacePoseThreshold_mmKey = "place_position_threshold_mm";
static const char* kDriveToPlacePoseThreshold_radsKey = "place_position_threshold_rads";

static const float kMinRequestDelayDefault = 5.0f;
static const float kAfterPlaceBackupDistance_mmDefault = 80.0f;
static const float kAfterPlaceBackupSpeed_mmpsDefault = 80.0f;
static const float kDistToMoveTowardsFace_mm = 120.0f;
static const float kFaceVerificationTime_s = 0.75f;
static const float kDriveToPlacePoseThreshold_mmDefault = DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM;
static const float kDriveToPlacePoseThreshold_radsDefault = DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD;

void BehaviorRequestGameSimple::ConfigPerNumBlocks::LoadFromJson(const Json::Value& config)
{
  initialAnimationName = config.get(kInitialAnimationKey, "").asString();
  preDriveAnimationName = config.get(kPreDriveAnimationKey, "").asString();
  requestAnimationName = config.get(kRequestAnimNameKey, "").asString();
  denyAnimationName = config.get(kDenyAnimNameKey, "").asString();
  minRequestDelay = config.get(kMinRequestDelayKey, kMinRequestDelayDefault).asFloat();
  scoreFactor = config.get(kScoreFactorKey, 1.0f).asFloat();
}

BehaviorRequestGameSimple::BehaviorRequestGameSimple(Robot& robot, const Json::Value& config)
  : IBehaviorRequestGame(robot, config)
{
  SetDefaultName("BehaviorRequestGameSimple");

  if( config.isNull() ) {
    PRINT_NAMED_WARNING("BehaviorRequestGameSimple.Config.Error",
                        "Empty json config! This behavior will not function correctly");
  }
  else {
    _zeroBlockConfig.LoadFromJson(config[kZeroBlockGroupKey]);
    _oneBlockConfig.LoadFromJson(config[kOneBlockGroupKey]);

    LoadPathMotionProfileFromJson(_driveToPickupProfile, config[kPickupMotionProfileKey]);
    LoadPathMotionProfileFromJson(_driveToPlaceProfile, config[kPlaceMotionProfileKey]);

    _driveToPlacePoseThreshold_mm = config.get(kDriveToPlacePoseThreshold_mmKey,
                                               kDriveToPlacePoseThreshold_mmDefault).asFloat();
    _driveToPlacePoseThreshold_rads = config.get(kDriveToPlacePoseThreshold_radsKey,
                                               kDriveToPlacePoseThreshold_radsDefault).asFloat();

      
    _afterPlaceBackupDist_mm = config.get(kAfterPlaceBackupDistance_mmKey,
                                          kAfterPlaceBackupDistance_mmDefault).asFloat();
    _afterPlaceBackupSpeed_mmps = config.get(kAfterPlaceBackupSpeed_mmpsKey,
                                             kAfterPlaceBackupSpeed_mmpsDefault).asFloat();
  }
}

Result BehaviorRequestGameSimple::RequestGame_InitInternal(Robot& robot,
                                                             double currentTime_sec)
{
  _verifyStartTime_s = std::numeric_limits<float>::max();

  // disable idle animation, but save the old one on the stack
  robot.PushIdleAnimation("NONE");

  if( GetNumBlocks(robot) == 0 ) {
    _activeConfig = &_zeroBlockConfig;
    if( ! IsActing() ) {
      // skip the block stuff and go right to the face
      TransitionToLookingAtFace(robot);
    }
  }
  else {
    _activeConfig = &_oneBlockConfig;
    if( ! IsActing() ) {
      TransitionToPlayingInitialAnimation(robot);
    }
  }

  return RESULT_OK;
}

IBehavior::Status BehaviorRequestGameSimple::RequestGame_UpdateInternal(Robot& robot, double currentTime_sec)
{
  if( _state == State::SearchingForBlock ) {
    // if we are searching for a block, stop immediately when we find one
    if( GetNumBlocks(robot) > 0 ) {
      PRINT_NAMED_INFO("BehaviorRequestGameSimple.FoundBlock",
                       "found block during search");
      StopActing(false);
      TransitionToFacingBlock(robot);
    }
  }

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
    // action is can canceled automatically by IBehavior
  }

  // don't use transition to because we don't want to do anything
  _state = State::PlayingInitialAnimation;

  robot.PopIdleAnimation();
}

float BehaviorRequestGameSimple::EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const
{
  // NOTE: can't use _activeConfig because we haven't been Init'd yet  
  float score = IBehavior::EvaluateScoreInternal(robot, currentTime_sec);
  if( GetNumBlocks(robot) == 0 ) {
    score *= _zeroBlockConfig.scoreFactor;
  }
  else {
    score *= _oneBlockConfig.scoreFactor;
  }
  return score;
}

void BehaviorRequestGameSimple::TransitionToPlayingInitialAnimation(Robot& robot)
{
  IActionRunner* animationAction = new PlayAnimationAction(robot, _activeConfig->initialAnimationName);
  StartActing( animationAction, &BehaviorRequestGameSimple::TransitionToFacingBlock );
  SET_STATE(State::PlayingInitialAnimation);
}
  
void BehaviorRequestGameSimple::TransitionToFacingBlock(Robot& robot)
{
  ObjectID targetBlockID = GetRobotsBlockID(robot);
  if( targetBlockID.IsSet() ) {
    StartActing(new FaceObjectAction( robot, targetBlockID, PI_F ),
                &BehaviorRequestGameSimple::TransitionToPlayingPreDriveAnimation);
    SET_STATE(State::FacingBlock);
  }
  else {
    PRINT_NAMED_INFO("BehaviorRequestGameSimple.TransiitonToFacingBlock.NoBlock",
                     "block no longer exists (or has moved). Searching for block");
    TransitionToSearchingForBlock(robot);
  }
}

void BehaviorRequestGameSimple::TransitionToPlayingPreDriveAnimation(Robot& robot)
{
  IActionRunner* animationAction = new PlayAnimationAction(robot, _activeConfig->preDriveAnimationName);
  StartActing(animationAction, &BehaviorRequestGameSimple::TransitionToPickingUpBlock);
  SET_STATE(State::PlayingPreDriveAnimation);
}

void BehaviorRequestGameSimple::TransitionToPickingUpBlock(Robot& robot)
{
  ObjectID targetBlockID = GetRobotsBlockID(robot);
  StartActing(new DriveToPickupObjectAction(robot, targetBlockID, _driveToPickupProfile),
              [this, &robot](ActionResult result) {
                if ( result == ActionResult::SUCCESS ) {
                  ComputeFaceInteractionPose(robot);
                  TransitionToDrivingToFace(robot);
                }
                else if ( result == ActionResult::FAILURE_RETRY ) {
                  TransitionToPickingUpBlock(robot);
                }
                else {
                  // if its an abort failure, do nothing, which will cause the behavior to stop
                  PRINT_NAMED_INFO("BehaviorRequestGameSimple.PickingUpBlock.Failed",
                                   "failed to pick up block with no retry, so ending the behavior");
                }
              } );
  SET_STATE(State::PickingUpBlock);
}

void BehaviorRequestGameSimple::TransitionToSearchingForBlock(Robot& robot)
{
  // face the last known pose, then look around a bit (left and right). The Update loop will cancel this
  // action if a block is found
  Pose3d lastKnownPose;
  if( GetLastBlockPose(lastKnownPose) ) {
    SET_STATE(State::SearchingForBlock);

    CompoundActionSequential* searchAction = new CompoundActionSequential(robot);

    FacePoseAction* faceAction = new FacePoseAction(robot, lastKnownPose, PI_F);
    faceAction->SetPanTolerance(DEG_TO_RAD(5));
    searchAction->AddAction(faceAction);

    searchAction->AddAction(new SearchSideToSideAction(robot));
    
    StartActing(searchAction,
                [this, &robot](ActionResult result) {
                  if( GetNumBlocks(robot) > 0 ) {
                    // check one more time to see if we found a block
                    TransitionToFacingBlock(robot);
                  }
                  else {
                    // could fall back to the 0 block request here, but might not want to
                    // TODO:(bn) shouldn't incur repetition penalty in this case?
                    PRINT_NAMED_INFO("BehaviorRequestGameSimple.SearchForBlock.Failed",
                                     "block has disappeared! Giving up on behavior");
                  }
                });
  }
  else {
    PRINT_NAMED_ERROR("BehaviorRequestGameSimple.NoLastBlockPose",
                      "Trying to search, but never had a block pose. This is a bug");
    StopActing(false);
  }
}


void BehaviorRequestGameSimple::ComputeFaceInteractionPose(Robot& robot)
{
  _hasFaceInteractionPose = GetFaceInteractionPose(robot, _faceInteractionPose);
}

void BehaviorRequestGameSimple::TransitionToDrivingToFace(Robot& robot)
{
  if( ! _hasFaceInteractionPose ) {
    PRINT_NAMED_INFO("BehaviorRequestGameSimple.TransitionToDrivingToFace.NoPose",
                     "%s: No interaction pose set to drive to face!",
                     GetName().c_str());
    return;
  }
  else {
    StartActing(new DriveToPoseAction(robot,
                                      _faceInteractionPose,
                                      _driveToPlaceProfile,
                                      false,
                                      false,
                                      _driveToPlacePoseThreshold_mm,
                                      _driveToPlacePoseThreshold_rads),
                [this, &robot](ActionResult result) {
                  if ( result == ActionResult::SUCCESS ) {
                    // transition back here, but don't reset the face pose to drive to
                    TransitionToPlacingBlock(robot);
                  }
                  else if (result == ActionResult::FAILURE_RETRY) {
                    TransitionToDrivingToFace(robot);
                  }
                  else {
                    // if its an abort failure, do nothing, which will cause the behavior to stop
                    PRINT_NAMED_INFO("BehaviorRequestGameSimple.DriveToFace.Failed",
                                     "failed to drive to a spot to interact with the face, so ending the behavior");
                  }
                } );
    SET_STATE(State::DrivingToFace);
  }
}

void BehaviorRequestGameSimple::TransitionToPlacingBlock(Robot& robot)
{
  StartActing(new CompoundActionSequential(robot,
                {
                  new PlaceObjectOnGroundAction(robot),
                  // TODO:(bn) use same motion profile here
                  new DriveStraightAction(robot, -_afterPlaceBackupDist_mm, -_afterPlaceBackupSpeed_mmps)
                }),
              [this, &robot](ActionResult result) {
                if ( result == ActionResult::SUCCESS ) {
                  TransitionToLookingAtFace(robot);
                }
                else if (result == ActionResult::FAILURE_RETRY) {
                  TransitionToPlacingBlock(robot);
                }
                else {
                  // the place action can fail if visual verify fails (it doesn't see the cube after it
                  // drops it). For now, if it fails, just keep going anyway
        
                  // if its an abort failure, do nothing, which will cause the behavior to stop
                  PRINT_NAMED_INFO("BehaviorRequestGameSimple.PlacingBlock.Failed",
                                   "failed to place the block on the ground, but pretending it didn't");

                  TransitionToLookingAtFace(robot);
                }
              } );

  SET_STATE(State::PlacingBlock);
}

void BehaviorRequestGameSimple::TransitionToLookingAtFace(Robot& robot)
{
  Pose3d lastFacePose;
  if( GetFacePose( robot, lastFacePose ) ) {
    StartActing(new FacePoseAction(robot, lastFacePose, PI_F),
                [this, &robot](ActionResult result) {
                  if( result == ActionResult::SUCCESS ) {
                    TransitionToVerifyingFace(robot);
                  }
                  else if (result == ActionResult::FAILURE_RETRY) {
                    TransitionToLookingAtFace(robot);
                  }
                  else {
                    PRINT_NAMED_INFO("BehaviorRequestGameSimple.LookingAtFace.Failed",
                                     "failed to look at face after dropping block, so ending the behavior");
                  }
                } );
    SET_STATE(State::LookingAtFace);
  }
}

void BehaviorRequestGameSimple::TransitionToVerifyingFace(Robot& robot)
{
  _verifyStartTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  StartActing(new WaitAction( robot, kFaceVerificationTime_s ),
              [this, &robot](ActionResult result) {
                if( result == ActionResult::SUCCESS && GetLastSeenFaceTime() >= _verifyStartTime_s ) {
                  TransitionToPlayingRequstAnim(robot);
                }
                else {
                  // the face must not have been where we expected, so drop out of the behavior for now
                  // TODO:(bn) try to bring the block to a different face if we have more than one?
                  PRINT_NAMED_INFO("BehaviorRequestGameSimple.VerifyingFace.Failed",
                                   "failed to verify the face, so considering this a rejection");
                  TransitionToPlayingDenyAnim(robot);
                }
              } );
  SET_STATE(State::VerifyingFace);
}

void BehaviorRequestGameSimple::TransitionToPlayingRequstAnim(Robot& robot)
{
  StartActing(new PlayAnimationAction(robot, _activeConfig->requestAnimationName),
              &BehaviorRequestGameSimple::TransitionToTrackingFace);
  SET_STATE(State::PlayingRequstAnim);
}

void BehaviorRequestGameSimple::TransitionToTrackingFace(Robot& robot)
{
  SendRequest(robot);

  if( GetFaceID() == Face::UnknownFace ) {
    PRINT_NAMED_WARNING("BehaviorRequestGameSimple.NoValidFace",
                        "Can't do face tracking because there is no valid face!");
    // use an action that just hangs to simulate the track face logic      
    StartActing( new HangAction(robot) );
  }
  else {
    // no callback here, behavior is over once this is done
    StartActing( new TrackFaceAction(robot, GetFaceID()) );
  }

  SET_STATE(State::TrackingFace);
}

void BehaviorRequestGameSimple::TransitionToPlayingDenyAnim(Robot& robot)
{
  // no callback here, behavior is over once this is Done
  StartActing( new PlayAnimationAction( robot, _activeConfig->denyAnimationName ) );

  SET_STATE(State::PlayingDenyAnim);
}

void BehaviorRequestGameSimple::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorRequestGameSimple.TransitionTo", "%s", stateName.c_str());
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
  targetPose = targetPose.GetWithRespectToOrigin();

  return true;
}

void BehaviorRequestGameSimple::HandleGameDeniedRequest(Robot& robot)
{
  StopActing();

  TransitionToPlayingDenyAnim(robot);
}

}
}

