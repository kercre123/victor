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
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviors/gameRequest/behaviorRequestGameSimple.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/pathMotionProfileHelpers.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
#define SET_STATE(s) SetState_internal(State::s, #s)

#define DO_FACE_VERIFICATION_STEP 0

static const char* kInitialAnimationKey = "initial_animName";
static const char* kPreDriveAnimationKey = "preDrive_animName";
static const char* kRequestAnimNameKey = "request_animName";
static const char* kDenyAnimNameKey = "deny_animName";
static const char* kIdleAnimNameKey = "idle_animName";
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
static const char* kShouldUseBlocksKey = "use_block";
static const char* kDoSecondRequestKey = "do_second_request";

static const float kMinRequestDelayDefault = 5.0f;
static const float kAfterPlaceBackupDistance_mmDefault = 80.0f;
static const float kAfterPlaceBackupSpeed_mmpsDefault = 80.0f;
static const float kDriveToPlacePoseThreshold_mmDefault = DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM;
static const float kDriveToPlacePoseThreshold_radsDefault = DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD;
static const bool  kShouldUseBlocksDefault = true;
static const bool  kDoSecondRequestDefault = false;

static const int   kFaceVerificationNumImages = 4;
static const int   kMaxNumberOfRetries = 2;


// #define kDistToMoveTowardsFace_mm {120.0f, 100.0f, 140.0f, 50.0f, 180.0f, 20.0f}
// to avoid cliff issues, we're only going to move slightly forwards
#define kDistToMoveTowardsFace_mm {20.0f}

// need to be as far away as the size of the robot + 2 blocks + padding
static const float kSafeDistSqFromObstacle_mm = SQUARE(100);


void BehaviorRequestGameSimple::ConfigPerNumBlocks::LoadFromJson(const Json::Value& config)
{
  // Valid for some of these to be "Count"
  JsonTools::GetValueOptional(config,kInitialAnimationKey,initialAnimTrigger);
  JsonTools::GetValueOptional(config,kPreDriveAnimationKey,preDriveAnimTrigger);
  JsonTools::GetValueOptional(config,kRequestAnimNameKey,requestAnimTrigger);
  JsonTools::GetValueOptional(config,kDenyAnimNameKey,denyAnimTrigger);
  JsonTools::GetValueOptional(config,kIdleAnimNameKey,idleAnimTrigger);
  minRequestDelay = config.get(kMinRequestDelayKey, kMinRequestDelayDefault).asFloat();
  scoreFactor = config.get(kScoreFactorKey, 1.0f).asFloat();
}

BehaviorRequestGameSimple::BehaviorRequestGameSimple(Robot& robot, const Json::Value& config)
  : IBehaviorRequestGame(robot, config)
  , _numRetriesPickingUpBlock(0)
  , _numRetriesDrivingToFace(0)
  , _numRetriesPlacingBlock(0)
{
  SetDefaultName("BehaviorRequestGameSimple");

  if( config.isNull() ) {
    PRINT_NAMED_WARNING("BehaviorRequestGameSimple.Config.Error",
                        "Empty json config! This behavior will not function correctly");
  }
  else {

    _shouldUseBlocks = config.get(kShouldUseBlocksKey, kShouldUseBlocksDefault).asBool();
    _doSecondRequest = config.get(kDoSecondRequestKey, kDoSecondRequestDefault).asBool();
  
    _zeroBlockConfig.LoadFromJson(config[kZeroBlockGroupKey]);

    if( _shouldUseBlocks ) {
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
    else {
      for( const char* blockKey : { kOneBlockGroupKey,
                                    kPickupMotionProfileKey,
                                    kPlaceMotionProfileKey,
                                    kDriveToPlacePoseThreshold_mmKey,
                                    kDriveToPlacePoseThreshold_radsKey,
                                    kAfterPlaceBackupDistance_mmKey,
                                    kAfterPlaceBackupSpeed_mmpsKey
                                  } ) {
        if( config.isMember(blockKey) ) {
          PRINT_NAMED_WARNING("BehaviorRequestGameSimple.InvalidKey",
                              "Behavior '%s' specifies that it should not use block, but specifies key '%s' "
                              "which will be ignored",
                              GetName().c_str(),
                              blockKey);
        }
      }

      if( ! FLT_NEAR( _zeroBlockConfig.scoreFactor, 1.0f ) ) {
        PRINT_NAMED_WARNING("BehaviorRequestGameSimple.PossibleScoreError",
                            "Behavior '%s' is not using blocks, but the zero block config discounts score by %f",
                            GetName().c_str(),
                            _zeroBlockConfig.scoreFactor);
      }
    }
  }
}

Result BehaviorRequestGameSimple::RequestGame_InitInternal(Robot& robot)
{
  _verifyStartTime_s = std::numeric_limits<float>::max();

  // disable idle animation, but save the old one on the stack
  if( ! _shouldPopIdle ) {
    _shouldPopIdle = true;
    robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::Count);
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
    if(robot.IsCarryingObject()){
      TransitionToDrivingToFace(robot);
    }else if( ! IsActing() ) {
      TransitionToPlayingInitialAnimation(robot);
    }
  }

  _initialRequest = true;
  _numRetriesPickingUpBlock = 0;
  _numRetriesDrivingToFace = 0;
  _numRetriesPlacingBlock = 0;
  
  
  return RESULT_OK;
}

IBehavior::Status BehaviorRequestGameSimple::RequestGame_UpdateInternal(Robot& robot)
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
  
  if(CheckRequestTimeout()) {
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
  
bool BehaviorRequestGameSimple::CheckRequestTimeout()
{
  const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const float minDelayComplete_sec = GetRequestMinDelayComplete_s();

  return (_state == State::Idle && minDelayComplete_sec >= 0.0f && currentTime_sec >= minDelayComplete_sec);
}

void BehaviorRequestGameSimple::StopInternal(Robot& robot)
{
  PRINT_NAMED_INFO("BehaviorRequestGameSimple.StopInternal", "");

  if( _state == State::Idle ) {
    // this means we have send up the game request, so now we should send a Deny to cancel it
    PRINT_NAMED_INFO("BehaviorRequestGameSimple.DenyRequest",
                     "behavior is denying it's own request");

    SendDeny(robot);
    // action is can canceled automatically by IBehavior
  }

  // don't use transition to because we don't want to do anything.
  _state = State::PlayingInitialAnimation;
  StopActing(false);

  if( _shouldPopIdle ) {
    _shouldPopIdle = false;
    robot.GetAnimationStreamer().PopIdleAnimation();
  }
}

float BehaviorRequestGameSimple::EvaluateScoreInternal(const Robot& robot) const
{
  // NOTE: can't use _activeConfig because we haven't been Init'd yet  
  float score = IBehavior::EvaluateScoreInternal(robot);
  if( GetNumBlocks(robot) == 0 ) {
    score *= _zeroBlockConfig.scoreFactor;
  }
  else {
    score *= _oneBlockConfig.scoreFactor;
  }
  return score;
}

float BehaviorRequestGameSimple::EvaluateRunningScoreInternal(const Robot& robot) const
{
  // if we have requested, and are past the timeout, then we don't want to keep running
  if( IsActing() ) {
    // while we are doing things, we really don't want to be interrupted
    return 1.5f;
  }

  // otherwise, fall back to running score
  return EvaluateScoreInternal(robot);
}

void BehaviorRequestGameSimple::TransitionToPlayingInitialAnimation(Robot& robot)
{
  IActionRunner* animationAction = new TurnTowardsFaceWrapperAction(
    robot,
    new TriggerAnimationAction(robot, _activeConfig->initialAnimTrigger) );
  StartActing( animationAction, &BehaviorRequestGameSimple::TransitionToFacingBlock );
  SET_STATE(PlayingInitialAnimation);
}
  
void BehaviorRequestGameSimple::TransitionToFacingBlock(Robot& robot)
{
  ObjectID targetBlockID = GetRobotsBlockID(robot);
  if( targetBlockID.IsSet() ) {
    StartActing(new TurnTowardsObjectAction( robot, targetBlockID, M_PI_F ),
                &BehaviorRequestGameSimple::TransitionToPlayingPreDriveAnimation);
    SET_STATE(FacingBlock);
  }
  else {
    PRINT_NAMED_INFO("BehaviorRequestGameSimple.TransiitonToFacingBlock.NoBlock",
                     "block no longer exists (or has moved). Searching for block");
    TransitionToSearchingForBlock(robot);
  }
}

void BehaviorRequestGameSimple::TransitionToPlayingPreDriveAnimation(Robot& robot)
{
  IActionRunner* animationAction = new TriggerAnimationAction(robot, _activeConfig->preDriveAnimTrigger);
  StartActing(animationAction, &BehaviorRequestGameSimple::TransitionToPickingUpBlock);
  SET_STATE(PlayingPreDriveAnimation);
}

void BehaviorRequestGameSimple::TransitionToPickingUpBlock(Robot& robot)
{
  // Ensure we don't loop forever
  if(_numRetriesPickingUpBlock > kMaxNumberOfRetries){
    ComputeFaceInteractionPose(robot);
    TransitionToDrivingToFace(robot);
    return;
  }
  
  ObjectID targetBlockID = GetRobotsBlockID(robot);
  DriveToPickupObjectAction* action = new DriveToPickupObjectAction(robot, targetBlockID);
  action->SetMotionProfile(_driveToPickupProfile);
  
  StartActing(action,
              [this, targetBlockID, &robot](const ExternalInterface::RobotCompletedAction& resultMsg) {
                ActionResultCategory resCat = IActionRunner::GetActionResultCategory(resultMsg.result);
                if ( resCat == ActionResultCategory::SUCCESS ) {
                  ComputeFaceInteractionPose(robot);
                  TransitionToDrivingToFace(robot);
                }
                else if ( resCat == ActionResultCategory::RETRY ) {
                  _numRetriesPickingUpBlock++;
                  // TODO:(bn) animation groups here?
                  IActionRunner* animAction = nullptr;
                  switch(resultMsg.result)
                  {
                    case ActionResult::NOT_CARRYING_OBJECT_RETRY:
                    case ActionResult::LAST_PICK_AND_PLACE_FAILED:
                    {
                      animAction = new TriggerAnimationAction(robot, AnimationTrigger::RequestGamePickupFail);
                      break;
                    }
            
                    default:
                    {
                      animAction = new TriggerAnimationAction(robot, AnimationTrigger::RequestGameDrivingFail);
                      break;
                    }
                  }
                  assert(nullptr != animAction);

                  StartActing(animAction, &BehaviorRequestGameSimple::TransitionToPickingUpBlock);
                }
                else {
                  // mark the block as unable to pickup
                  const ObservableObject* failedObject = robot.GetBlockWorld().GetObjectByID(targetBlockID);
                  if(failedObject){
                    robot.GetAIComponent().GetWhiteboard().SetFailedToUse(*failedObject, AIWhiteboard::ObjectUseAction::PickUpObject);
                  }
                  
                  // couldn't pick up this block. If we have another, try that. Otherwise, fail
                  if( SwitchRobotsBlock(robot) ) {
                    
                    // Play failure animation
                    if (resultMsg.result == ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_MOVING || resultMsg.result == ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_NOT_MOVING) {
                      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::RequestGamePickupFail), &BehaviorRequestGameSimple::TransitionToPickingUpBlock);
                    } else {
                      TransitionToPickingUpBlock(robot);
                    }
                  }
                  else {
                    // if its an abort failure, do nothing, which will cause the behavior to stop
                    PRINT_NAMED_INFO("BehaviorRequestGameSimple.PickingUpBlock.Failed",
                                     "failed to pick up block with no retry, so ending the behavior");
                    
                    // Play failure animation
                    if (resultMsg.result == ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_MOVING || resultMsg.result == ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_NOT_MOVING) {
                      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::RequestGamePickupFail));
                    }

                  }
                }
              } );
  
  SET_STATE(PickingUpBlock);
}

void BehaviorRequestGameSimple::TransitionToSearchingForBlock(Robot& robot)
{
  // face the last known pose, then look around a bit (left and right). The Update loop will cancel this
  // action if a block is found
  Pose3d lastKnownPose;
  if( GetLastBlockPose(lastKnownPose) ) {
    SET_STATE(SearchingForBlock);

    CompoundActionSequential* searchAction = new CompoundActionSequential(robot);

    TurnTowardsPoseAction* turnTowardsPoseAction = new TurnTowardsPoseAction(robot, lastKnownPose, M_PI_F);
    turnTowardsPoseAction->SetPanTolerance(DEG_TO_RAD(5));
    searchAction->AddAction(turnTowardsPoseAction);

    // passes either the vaild id which will return as soon as its seen, or an invalid ID so
    // the search will complete fully
    const ObjectID blockID = GetRobotsBlockID(robot);
    searchAction->AddAction(new SearchForNearbyObjectAction(robot, blockID));
    
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
  // Ensure we don't loop forever
  if(_numRetriesDrivingToFace > kMaxNumberOfRetries){
    TransitionToPlacingBlock(robot);
    return;
  }
  
  if( ! _hasFaceInteractionPose ) {
    PRINT_NAMED_INFO("BehaviorRequestGameSimple.TransitionToDrivingToFace.NoPose",
                     "%s: No interaction pose set to drive to face!",
                     GetName().c_str());
    return;
  }
  else {
    DriveToPoseAction* action = new DriveToPoseAction(robot,
                                                      _faceInteractionPose,
                                                      false,
                                                      false,
                                                      _driveToPlacePoseThreshold_mm,
                                                      _driveToPlacePoseThreshold_rads);
    action->SetMotionProfile(_driveToPlaceProfile);
    StartActing(action,
                [this, &robot](ActionResult result) {
                  if ( result == ActionResult::SUCCESS ) {
                    // transition back here, but don't reset the face pose to drive to
                    TransitionToPlacingBlock(robot);
                  }
                  else if (IActionRunner::GetActionResultCategory(result) == ActionResultCategory::RETRY) {
                    _numRetriesDrivingToFace++;
                    TransitionToDrivingToFace(robot);
                  }
                  else {
                    // if its an abort failure, do nothing, which will cause the behavior to stop
                    PRINT_NAMED_INFO("BehaviorRequestGameSimple.DriveToFace.Failed",
                                     "failed to drive to a spot to interact with the face, so ending the behavior");
                  }
                } );
    SET_STATE(DrivingToFace);
  }
}

void BehaviorRequestGameSimple::TransitionToPlacingBlock(Robot& robot)
{
  // Ensure we don't loop forever
  if(_numRetriesPlacingBlock > kMaxNumberOfRetries){
    TransitionToLookingAtFace(robot);
    return;
  }

  CompoundActionSequential* action = new CompoundActionSequential(robot);

  {
    const bool shouldEmitCompletion = true;
    action->AddAction( new PlaceObjectOnGroundAction(robot), false, shouldEmitCompletion );
  }

  // TODO:(bn) use same motion profile here
  action->AddAction(new DriveStraightAction(robot, -_afterPlaceBackupDist_mm, _afterPlaceBackupSpeed_mmps));

  StartActing(action,
              [this, &robot](ActionResult result) {
                ActionResultCategory resCat = IActionRunner::GetActionResultCategory(result);
                if ( resCat == ActionResultCategory::SUCCESS ) {
                  _numRetriesPlacingBlock++;
                  TransitionToLookingAtFace(robot);
                }
                else if (resCat == ActionResultCategory::RETRY) {
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
  
  SET_STATE(PlacingBlock);

}

void BehaviorRequestGameSimple::TransitionToLookingAtFace(Robot& robot)
{
  const bool sayName = true;
  StartActing(new TurnTowardsLastFacePoseAction(robot, M_PI_F, sayName),
              &BehaviorRequestGameSimple::TransitionToVerifyingFace);
  SET_STATE(LookingAtFace);
}

void BehaviorRequestGameSimple::TransitionToVerifyingFace(Robot& robot)
{
  if( DO_FACE_VERIFICATION_STEP ) {
    _verifyStartTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    StartActing(new WaitForImagesAction(robot, kFaceVerificationNumImages, VisionMode::DetectingFaces),
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
    SET_STATE(VerifyingFace);
  }
  else {
    // just skip verification and go straight to playing the request
    TransitionToPlayingRequstAnim(robot);
  }
}

void BehaviorRequestGameSimple::TransitionToPlayingRequstAnim(Robot& robot) {
  // Don't interrupt the request process for a cube move
  SmartDisableReactionTrigger(ReactionTrigger::CubeMoved);

  // always turn back to the face after the animation in case the animation moves the head
  StartActing(new CompoundActionSequential(robot, {
        new TriggerAnimationAction(robot, _activeConfig->requestAnimTrigger),
        new TurnTowardsLastFacePoseAction(robot, M_PI_F)}),
    &BehaviorRequestGameSimple::TransitionToIdle);
  SET_STATE(PlayingRequestAnim);
}
 
void BehaviorRequestGameSimple::TransitionToIdle(Robot& robot)
{
  SendRequest(robot, _initialRequest);
  
  if(_activeConfig->idleAnimTrigger != AnimationTrigger::Count
     && GetFaceID() != Vision::UnknownFaceID){
    // no callback here, behavior is over once this is done
    StartActing(new CompoundActionParallel(robot, {
      new TrackFaceAction(robot, GetFaceID()),
      new TriggerAnimationAction(robot, _activeConfig->idleAnimTrigger, 0),
    }));
  }else if(GetFaceID() != Vision::UnknownFaceID){
    StartActing(new TrackFaceAction(robot, GetFaceID()));
  }else if(_activeConfig->idleAnimTrigger != AnimationTrigger::Count){
    StartActing(new TriggerAnimationAction(robot, _activeConfig->idleAnimTrigger, 0));
  }else{
    StartActing( new HangAction(robot) );
  }
  
  SET_STATE(Idle);
  BehaviorObjectiveAchieved(BehaviorObjective::RequestedGame);
}

void BehaviorRequestGameSimple::TransitionToPlayingDenyAnim(Robot& robot)
{
  IActionRunner* denyAnimAction = new TriggerAnimationAction( robot, _activeConfig->denyAnimTrigger );

  if( _initialRequest && _doSecondRequest ) {
    // try again
    _initialRequest = false;
    StartActing(denyAnimAction, &BehaviorRequestGameSimple::TransitionToLookingAtFace);
  }
  else {
    // TODO:(bn) different animation the second time?
    StartActing(denyAnimAction);
  }
  
  SET_STATE(PlayingDenyAnim);
}

void BehaviorRequestGameSimple::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorRequestGameSimple.TransitionTo", "%s", stateName.c_str());
  DEBUG_SET_STATE(s);
}
  
  
u32 BehaviorRequestGameSimple::GetNumBlocks(const Robot& robot) const
{
  if( _shouldUseBlocks ) {
    return IBehaviorRequestGame::GetNumBlocks(robot);
  }
  else {
    return 0;
  }
}

bool BehaviorRequestGameSimple::GetFaceInteractionPose(Robot& robot, Pose3d& targetPoseRet)
{
  Pose3d facePose;
  
  if (HasFace(robot)) {
    TimeStamp_t lastObservedFaceTime = robot.GetFaceWorld().GetLastObservedFaceWithRespectToRobot(facePose);
    DEV_ASSERT(lastObservedFaceTime > 0, "BehaviorRequestGameSimple.HasFaceWithoutPose");
  }
  else {
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
  float oneOverDist = 1.0f / xyDistToFace;

  Pose3d targetPose;
  
  for(float newDist : kDistToMoveTowardsFace_mm) {
    float distanceRatio =  newDist * oneOverDist;
        
    float relX = distanceRatio * facePose.GetTranslation().x();
    float relY = distanceRatio * facePose.GetTranslation().y();

    targetPose = Pose3d{ targetAngle, Z_AXIS_3D(), {relX, relY, 0.0f}, &robot.GetPose() };
    targetPose = targetPose.GetWithRespectToOrigin();

    BlockWorldFilter filter;
    filter.OnlyConsiderLatestUpdate(false);
    filter.SetFilterFcn( [&](const ObservableObject* obj) {
        if( obj->GetPoseState() != PoseState::Known ) {
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
  if (_requestTime_s < 0.0f) {
    return -1.0f;
  }
  
  float minRequestDelay = kMinRequestDelayDefault;
  if (nullptr != _activeConfig)
  {
    minRequestDelay = _activeConfig->minRequestDelay;
  }
  
  return _requestTime_s + minRequestDelay;
}

}
}

