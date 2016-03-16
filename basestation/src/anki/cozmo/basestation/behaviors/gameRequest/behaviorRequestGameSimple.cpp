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

#define DO_FACE_VERIFICATION_STEP 0

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

static const char* kCliffReactAnimName = "anim_VS_loco_cliffReact";

static const float kMinRequestDelayDefault = 5.0f;
static const float kAfterPlaceBackupDistance_mmDefault = 80.0f;
static const float kAfterPlaceBackupSpeed_mmpsDefault = 80.0f;
static const float kFaceVerificationTime_s = 0.75f;
static const float kDriveToPlacePoseThreshold_mmDefault = DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM;
static const float kDriveToPlacePoseThreshold_radsDefault = DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD;

// #define kDistToMoveTowardsFace_mm {120.0f, 100.0f, 140.0f, 50.0f, 180.0f, 20.0f}
// to avoid cliff issues, we're only going to move slightly forwards
#define kDistToMoveTowardsFace_mm {20.0f}

// need to be as far away as the size of the robot + 2 blocks + padding
static const float kSafeDistSqFromObstacle_mm = SQUARE(100);


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

  SubscribeToTags({EngineToGameTag::CliffEvent});
}

Result BehaviorRequestGameSimple::RequestGame_InitInternal(Robot& robot,
                                                             double currentTime_sec)
{
  _verifyStartTime_s = std::numeric_limits<float>::max();

  // disable idle animation, but save the old one on the stack
  if( ! _shouldPopIdle ) {
    _shouldPopIdle = true;
    robot.PushIdleAnimation("NONE");
  }

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
  
  if( _state == State::TrackingFace &&
      GetRequestMinDelayComplete_s() >= 0.0f &&
      currentTime_sec >= GetRequestMinDelayComplete_s() ) {
    // timeout acts as a deny
    StopActing();
    SendDeny(robot);
    TransitionToPlayingDenyAnim(robot);
  }

  if( IsActing() ) {
    return Status::Running;
  }
  
  PRINT_NAMED_DEBUG("BehaviorRequestGameSimple.Complete", "no current actions, so finishing");

  return Status::Complete;
}

Result BehaviorRequestGameSimple::InterruptInternal(Robot& robot, double currentTime_sec)
{
  if( _state == State::HandlingCliff ) {
    return Result::RESULT_FAIL;
  }
  
  StopInternal(robot, currentTime_sec);
  StopActing(false);
  
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

  // don't use transition to because we don't want to do anything.
  _state = State::PlayingInitialAnimation;

  if( _shouldPopIdle ) {
    _shouldPopIdle = false;
    robot.PopIdleAnimation();
  }
}

float BehaviorRequestGameSimple::EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const
{
  if( _state == State::HandlingCliff ) {
    return 1.0f;
  }
  
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

float BehaviorRequestGameSimple::EvaluateRunningScoreInternal(const Robot& robot, double currentTime_sec) const
{
  // if we have requested, and are past the timeout, then we don't want to keep running
  if( IsActing() ) {
    // while we are doing things, we really don't want to be interrupted
    return 1.0f;
  }

  // otherwise, fall back to running score
  return EvaluateScoreInternal(robot, currentTime_sec);
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
    StartActing(new TurnTowardsObjectAction( robot, targetBlockID, PI_F ),
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
                  // couldn't pick up this block. If we have another, try that. Otherwise, fail
                  if( SwitchRobotsBlock(robot) ) {
                    TransitionToPickingUpBlock(robot);
                  }
                  else {
                    // if its an abort failure, do nothing, which will cause the behavior to stop
                    PRINT_NAMED_INFO("BehaviorRequestGameSimple.PickingUpBlock.Failed",
                                     "failed to pick up block with no retry, so ending the behavior");
                  }
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

    TurnTowardsPoseAction* turnTowardsPoseAction = new TurnTowardsPoseAction(robot, lastKnownPose, PI_F);
    turnTowardsPoseAction->SetPanTolerance(DEG_TO_RAD(5));
    searchAction->AddAction(turnTowardsPoseAction);

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
    StartActing(new TurnTowardsPoseAction(robot, lastFacePose, PI_F),
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
  if( DO_FACE_VERIFICATION_STEP ) {
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
  else {
    // just skip verification and go straight to playing the request
    TransitionToPlayingRequstAnim(robot);
  }
}

void BehaviorRequestGameSimple::TransitionToPlayingRequstAnim(Robot& robot)
{
  // always turn back to the face after the animation in case the animation moves the head
  StartActing(new CompoundActionSequential(robot, {
        new PlayAnimationAction(robot, _activeConfig->requestAnimationName),
        new TurnTowardsLastFacePoseAction(robot, PI_F)}),
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

void BehaviorRequestGameSimple::TransitionToHandlingCliff(Robot& robot)
{
  // cancel any other action when we enter this state
  StopActing(false);
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  action->AddAction(new PlayAnimationAction(robot, kCliffReactAnimName));
  if( robot.IsCarryingObject() ) {
    action->AddAction(new PlaceObjectOnGroundAction(robot));
  }

  // after action is complete, let the behavior end, resetting the state
  StartActing( action , [this]() { SET_STATE(State::PlayingInitialAnimation); });

  SET_STATE(State::HandlingCliff);
}

void BehaviorRequestGameSimple::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorRequestGameSimple.TransitionTo", "%s", stateName.c_str());
  SetStateName(stateName);
}

bool BehaviorRequestGameSimple::GetFaceInteractionPose(Robot& robot, Pose3d& targetPoseRet)
{
  Pose3d facePose;
  if ( ! GetFacePose(robot, facePose ) ) {
    PRINT_NAMED_WARNING("BehaviorRequestGameSimple.NoFace",
                        "Face pose is invalid!");
    return false;
  }
  
  if( ! facePose.GetWithRespectTo(robot.GetPose(), facePose) ) {
    PRINT_NAMED_ERROR("BehaviorRequestGameSimple.NoFacePose",
                      "could not get face pose with respect to robot. This should never happen");
    return false;
  }

  float xyDistToFace = std::sqrt( std::pow( facePose.GetTranslation().x(), 2) +
                                  std::pow( facePose.GetTranslation().y(), 2) );

  // try a few different positions until we get one that isn't near another cube
  Radians targetAngle = std::atan2(facePose.GetTranslation().y(), facePose.GetTranslation().x());
  float oneOverDist = 1.0 / xyDistToFace;

  Pose3d targetPose;
  
  for(float newDist : kDistToMoveTowardsFace_mm) {
    float distanceRatio =  newDist * oneOverDist;
        
    float relX = distanceRatio * facePose.GetTranslation().x();
    float relY = distanceRatio * facePose.GetTranslation().y();

    targetPose = Pose3d{ targetAngle, Z_AXIS_3D(), {relX, relY, 0.0f}, &robot.GetPose() };
    targetPose = targetPose.GetWithRespectToOrigin();

    BlockWorldFilter filter;
    filter.OnlyConsiderLatestUpdate(false);
    filter.SetFilterFcn( [&](ObservableObject* obj) {
        if( obj->GetPoseState() != ObservableObject::PoseState::Known ) {
          // ignore unknown obstacles
          return false;
        }

        if( robot.IsCarryingObject() && robot.GetCarryingObject() == obj->GetID() ) {
          // ignore the block we are carrying
          return false;
        }

        float distSq = (obj->GetPose().GetWithRespectToOrigin().GetTranslation() -
                        targetPose.GetTranslation()).LengthSq();
        if( distSq < kSafeDistSqFromObstacle_mm ) {
          PRINT_NAMED_DEBUG("BehaviorRequestGameSimple.GetFaceInteractionPose.Obstacle",
                            "Obstacle %d within sqrt(%f) of point at d=%f",
                            obj->GetID().GetValue(),
                            distSq,
                            newDist);
          return true;
        }
        return false;
      });

    std::vector<ObservableObject*> blocks;
    robot.GetBlockWorld().FindMatchingObjects(filter, blocks);

    if(blocks.empty()) {
      targetPoseRet = targetPose;
      return true;
    }
  }

  PRINT_NAMED_INFO("BehaviorRequestGameSimple.NoSafeBlockPose",
                   "Could not find a safe place to put down the cube, using current position");  
  targetPoseRet = Pose3d{ targetAngle, Z_AXIS_3D(), {0.0f, 0.0f, 0.0f}, &robot.GetPose() };
  targetPoseRet = targetPoseRet.GetWithRespectToOrigin();

  return true;
}

void BehaviorRequestGameSimple::HandleGameDeniedRequest(Robot& robot)
{
  StopActing();

  TransitionToPlayingDenyAnim(robot);
}
  
f32 BehaviorRequestGameSimple::GetRequestMinDelayComplete_s() const
{
  if( _requestTime_s < 0.0f ) {
    return -1.0f;
  }
  
  float minRequestDelay = kMinRequestDelayDefault;
  if (nullptr != _activeConfig)
  {
    minRequestDelay = _activeConfig->minRequestDelay;
  }
  
  return _requestTime_s + minRequestDelay;
}

void BehaviorRequestGameSimple::HandleCliffEvent(Robot& robot, const EngineToGameEvent& event)
{
  if( event.GetData().Get_CliffEvent().detected ) {
    PRINT_NAMED_INFO("BehaviorRequestGameSimple.Cliff",
                     "handling cliff event");
    TransitionToHandlingCliff(robot);
  }
}

}
}

