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


#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/trackFaceAction.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/gameRequest/behaviorRequestGameSimple.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/events/animationTriggerHelpers.h"
#include "engine/faceWorld.h"
#include "engine/pathMotionProfileHelpers.h"

namespace Anki {
namespace Cozmo {

namespace{
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
static const char* kDisableReactionsEarlyKey = "disable_reactions_early";

static const float kMinRequestDelayDefault = 5.0f;
static const float kAfterPlaceBackupDistance_mmDefault = 80.0f;
static const float kAfterPlaceBackupSpeed_mmpsDefault = 80.0f;
static const float kDriveToPlacePoseThreshold_mmDefault = DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM;
static const float kDriveToPlacePoseThreshold_radsDefault = DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD;
static const bool  kShouldUseBlocksDefault = true;

static const int   kFaceVerificationNumImages = 4;
static const int   kMaxNumberOfRetries = 2;


// #define kDistToMoveTowardsFace_mm {120.0f, 100.0f, 140.0f, 50.0f, 180.0f, 20.0f}
// to avoid cliff issues, we're only going to move slightly forwards
#define kDistToMoveTowardsFace_mm {20.0f}

// need to be as far away as the size of the robot + 2 blocks + padding
static const float kSafeDistSqFromObstacle_mm = SQUARE(100);

} // end namespace


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRequestGameSimple::BehaviorRequestGameSimple(const Json::Value& config)
: ICozmoBehaviorRequestGame(config)
, _numRetriesDrivingToFace(0)
, _numRetriesPlacingBlock(0)
, _wasTriggeredAsInterrupt(false)
{
  if( config.isNull() ) {
    PRINT_NAMED_WARNING("BehaviorRequestGameSimple.Config.Error",
                        "Empty json config! This behavior will not function correctly");
  }
  else {

    _shouldUseBlocks = config.get(kShouldUseBlocksKey, kShouldUseBlocksDefault).asBool();
  
    _zeroBlockConfig.LoadFromJson(config[kZeroBlockGroupKey]);

    _disableReactionsEarly = config.get(kDisableReactionsEarlyKey, false).asBool();

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
                              GetIDStr().c_str(),
                              blockKey);
        }
      }

      if( ! FLT_NEAR( _zeroBlockConfig.scoreFactor, 1.0f ) ) {
        PRINT_NAMED_WARNING("BehaviorRequestGameSimple.PossibleScoreError",
                            "Behavior '%s' is not using blocks, but the zero block config discounts score by %f",
                            GetIDStr().c_str(),
                            _zeroBlockConfig.scoreFactor);
      }
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::RequestGame_OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _verifyStartTime_s = std::numeric_limits<float>::max();

  // use the driving motion profile by default
  SmartSetMotionProfile(_driveToPlaceProfile);
  
  if(_wasTriggeredAsInterrupt){
    _activeConfig = &_zeroBlockConfig;
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::RequestGameInterrupt),
                &BehaviorRequestGameSimple::TransitionToLookingAtFace);
  }else if( GetNumBlocks(behaviorExternalInterface) == 0 ) {
    _activeConfig = &_zeroBlockConfig;
    if( ! IsControlDelegated() ) {
      // skip the block stuff and go right to the face
      TransitionToLookingAtFace(behaviorExternalInterface);
    }
  }
  else {
    _activeConfig = &_oneBlockConfig;
    if(behaviorExternalInterface.GetRobotInfo().GetCarryingComponent().IsCarryingObject()){
      TransitionToDrivingToFace(behaviorExternalInterface);
    }else if( ! IsControlDelegated() ) {
      TransitionToPlayingInitialAnimation(behaviorExternalInterface);
    }
  }

  _numRetriesDrivingToFace = 0;
  _numRetriesPlacingBlock = 0;  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::RequestGame_UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( _state == State::SearchingForBlock ) {
    // if we are searching for a block, stop immediately when we find one
    if( GetNumBlocks(behaviorExternalInterface) > 0 ) {
      PRINT_NAMED_INFO("BehaviorRequestGameSimple.FoundBlock",
                       "found block during search");
      CancelDelegates(false);
      TransitionToFacingBlock(behaviorExternalInterface);
    }
  }
  
  if(CheckRequestTimeout()) {
    // timeout acts as a deny
    CancelDelegates(false);
    SendDeny(behaviorExternalInterface);
    TransitionToPlayingDenyAnim(behaviorExternalInterface);
  }

  if( IsControlDelegated() ) {
    return;
  }
  
  PRINT_NAMED_DEBUG("BehaviorRequestGameSimple.Complete", "no current actions, so finishing");

  CancelSelf();
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRequestGameSimple::CheckRequestTimeout()
{
  const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const float minDelayComplete_sec = GetRequestMinDelayComplete_s();

  return (_state == State::Idle && minDelayComplete_sec >= 0.0f && currentTime_sec >= minDelayComplete_sec);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::RequestGame_OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  PRINT_NAMED_INFO("BehaviorRequestGameSimple.RequestGame_StopInternal", "");

  if( _state == State::Idle ) {
    // this means we have send up the game request, so now we should send a Deny to cancel it
    PRINT_NAMED_INFO("BehaviorRequestGameSimple.DenyRequest",
                     "behavior is denying it's own request");

    SendDeny(behaviorExternalInterface);
    // action is can canceled automatically by ICozmoBehavior
    
  }

  // don't use transition to because we don't want to do anything.
  _state = State::PlayingInitialAnimation;
  _wasTriggeredAsInterrupt = false;
  CancelDelegates(false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::TransitionToPlayingInitialAnimation(BehaviorExternalInterface& behaviorExternalInterface)
{
  
  IActionRunner* animationAction = new TurnTowardsFaceWrapperAction(
    new TriggerAnimationAction(_activeConfig->initialAnimTrigger) );
  DelegateIfInControl( animationAction, &BehaviorRequestGameSimple::TransitionToFacingBlock );
  SET_STATE(PlayingInitialAnimation);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::TransitionToFacingBlock(BehaviorExternalInterface& behaviorExternalInterface)
{

  ObjectID targetBlockID = GetRobotsBlockID(behaviorExternalInterface);
  if( targetBlockID.IsSet() ) {
    DelegateIfInControl(new TurnTowardsObjectAction(targetBlockID),
                &BehaviorRequestGameSimple::TransitionToPlayingPreDriveAnimation);
    SET_STATE(FacingBlock);
  }
  else {
    PRINT_NAMED_INFO("BehaviorRequestGameSimple.TransiitonToFacingBlock.NoBlock",
                     "block no longer exists (or has moved). Searching for block");
    
    SET_STATE(SearchingForBlock);

    auto& factory = behaviorExternalInterface.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
    SearchParameters params;
    params.numberOfBlocksToLocate = 1;
    HelperHandle searchHelper = factory.CreateSearchForBlockHelper(behaviorExternalInterface, *this, params);
    SmartDelegateToHelper(behaviorExternalInterface, searchHelper, &BehaviorRequestGameSimple::TransitionToFacingBlock);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::TransitionToPlayingPreDriveAnimation(BehaviorExternalInterface& behaviorExternalInterface)
{
  IActionRunner* animationAction = new TriggerAnimationAction(_activeConfig->preDriveAnimTrigger);
  DelegateIfInControl(animationAction, &BehaviorRequestGameSimple::TransitionToPickingUpBlock);
  SET_STATE(PlayingPreDriveAnimation);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::TransitionToPickingUpBlock(BehaviorExternalInterface& behaviorExternalInterface)
{
  
  ObjectID targetBlockID = GetRobotsBlockID(behaviorExternalInterface);

  // use the pickup motion profile for pickup, clearing the driving one first
  SmartClearMotionProfile();
  SmartSetMotionProfile(_driveToPickupProfile);

  auto onPickupSuccess = [this](BehaviorExternalInterface& behaviorExternalInterface){
    // restore the driving motion profile
    SmartClearMotionProfile();
    SmartSetMotionProfile(_driveToPlaceProfile);
    
    TransitionToDrivingToFace(behaviorExternalInterface);
  };
  
  auto onPickupFailure = [this](BehaviorExternalInterface& behaviorExternalInterface){
    // restore the driving motion profile
    SmartClearMotionProfile();
    SmartSetMotionProfile(_driveToPlaceProfile);

    // couldn't pick up this block. If we have another, try that. Otherwise, fail
    if( SwitchRobotsBlock(behaviorExternalInterface) ) {
      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::RequestGamePickupFail),
                              &BehaviorRequestGameSimple::TransitionToPickingUpBlock);
    }else {
      // if its an abort failure, do nothing, which will cause the behavior to stop
      PRINT_NAMED_INFO("BehaviorRequestGameSimple.PickingUpBlock.Failed",
                       "Helper failed to pick up block, so ending the behavior");
      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::RequestGamePickupFail));
    }
  };
  
  auto& helperComp = behaviorExternalInterface.GetAIComponent().GetBehaviorHelperComponent();
  
  auto& factory = helperComp.GetBehaviorHelperFactory();
  PickupBlockParamaters params;
  params.allowedToRetryFromDifferentPose = true;
  HelperHandle pickupHelper = factory.CreatePickupBlockHelper(behaviorExternalInterface, *this, targetBlockID, params);
  SmartDelegateToHelper(behaviorExternalInterface, pickupHelper,
                        onPickupSuccess,
                        onPickupFailure);

  SET_STATE(PickingUpBlock);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::ComputeFaceInteractionPose(BehaviorExternalInterface& behaviorExternalInterface)
{
  _hasFaceInteractionPose = GetFaceInteractionPose(behaviorExternalInterface, _faceInteractionPose);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::TransitionToDrivingToFace(BehaviorExternalInterface& behaviorExternalInterface)
{
  // Ensure we don't loop forever
  if(_numRetriesDrivingToFace > kMaxNumberOfRetries){
    TransitionToPlacingBlock(behaviorExternalInterface);
    return;
  }
  
  ComputeFaceInteractionPose(behaviorExternalInterface);
  if( ! _hasFaceInteractionPose ) {
    PRINT_NAMED_INFO("BehaviorRequestGameSimple.TransitionToDrivingToFace.NoPose",
                     "%s: No interaction pose set to drive to face!",
                     GetIDStr().c_str());
    return;
  }
  else {
    DriveToPoseAction* action = new DriveToPoseAction(_faceInteractionPose,
                                                      false,
                                                      false,
                                                      _driveToPlacePoseThreshold_mm,
                                                      _driveToPlacePoseThreshold_rads);
    DelegateIfInControl(action,
                [this, &behaviorExternalInterface](ActionResult result) {
                  if ( result == ActionResult::SUCCESS ) {
                    // transition back here, but don't reset the face pose to drive to
                    TransitionToPlacingBlock(behaviorExternalInterface);
                  }
                  else if (IActionRunner::GetActionResultCategory(result) == ActionResultCategory::RETRY) {
                    _numRetriesDrivingToFace++;
                    TransitionToDrivingToFace(behaviorExternalInterface);
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

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::TransitionToPlacingBlock(BehaviorExternalInterface& behaviorExternalInterface)
{
  // Ensure we don't loop forever
  if(_numRetriesPlacingBlock > kMaxNumberOfRetries){
    TransitionToLookingAtFace(behaviorExternalInterface);
    return;
  }

  CompoundActionSequential* action = new CompoundActionSequential();

  {
    const bool shouldEmitCompletion = true;
    action->AddAction( new PlaceObjectOnGroundAction(), false, shouldEmitCompletion );
  }

  // TODO:(bn) use same motion profile here
  action->AddAction(new DriveStraightAction(-_afterPlaceBackupDist_mm, _afterPlaceBackupSpeed_mmps));

  DelegateIfInControl(action,
              [this, &behaviorExternalInterface](ActionResult result) {
                ActionResultCategory resCat = IActionRunner::GetActionResultCategory(result);
                if ( resCat == ActionResultCategory::SUCCESS ) {
                  _numRetriesPlacingBlock++;
                  TransitionToLookingAtFace(behaviorExternalInterface);
                }
                else if (resCat == ActionResultCategory::RETRY) {
                  TransitionToPlacingBlock(behaviorExternalInterface);
                }
                else {
                  // the place action can fail if visual verify fails (it doesn't see the cube after it
                  // drops it). For now, if it fails, just keep going anyway
        
                  // if its an abort failure, do nothing, which will cause the behavior to stop
                  PRINT_NAMED_INFO("BehaviorRequestGameSimple.PlacingBlock.Failed",
                                   "failed to place the block on the ground, but pretending it didn't");

                  TransitionToLookingAtFace(behaviorExternalInterface);
                }
              } );
  
  SET_STATE(PlacingBlock);

}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::TransitionToLookingAtFace(BehaviorExternalInterface& behaviorExternalInterface)
{
  const bool sayName = true;
  DelegateIfInControl(new TurnTowardsLastFacePoseAction(M_PI_F, sayName),
              &BehaviorRequestGameSimple::TransitionToVerifyingFace);
  SET_STATE(LookingAtFace);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::TransitionToVerifyingFace(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( DO_FACE_VERIFICATION_STEP ) {
    _verifyStartTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    DelegateIfInControl(new WaitForImagesAction(kFaceVerificationNumImages, VisionMode::DetectingFaces),
                [this, &behaviorExternalInterface](ActionResult result) {
                  if( result == ActionResult::SUCCESS && GetLastSeenFaceTime() >= _verifyStartTime_s ) {
                    TransitionToPlayingRequstAnim(behaviorExternalInterface);
                  }
                  else {
                    // the face must not have been where we expected, so drop out of the behavior for now
                    // TODO:(bn) try to bring the block to a different face if we have more than one?
                    PRINT_NAMED_INFO("BehaviorRequestGameSimple.VerifyingFace.Failed",
                                     "failed to verify the face, so considering this a rejection");
                    TransitionToPlayingDenyAnim(behaviorExternalInterface);
                  }
                } );
    SET_STATE(VerifyingFace);
  }
  else {
    // just skip verification and go straight to playing the request
    TransitionToPlayingRequstAnim(behaviorExternalInterface);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::TransitionToPlayingRequstAnim(BehaviorExternalInterface& behaviorExternalInterface) {

  DelegateIfInControl(new TriggerAnimationAction(_activeConfig->requestAnimTrigger),
              &BehaviorRequestGameSimple::TransitionToIdle);
  SET_STATE(PlayingRequestAnim);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::TransitionToIdle(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(Idle);
  SendRequest(behaviorExternalInterface);
  IdleLoop(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::IdleLoop(BehaviorExternalInterface& behaviorExternalInterface){
  if(_activeConfig->idleAnimTrigger != AnimationTrigger::Count){
    SmartPushIdleAnimation(behaviorExternalInterface, _activeConfig->idleAnimTrigger);
  }else{
    SmartPushIdleAnimation(behaviorExternalInterface, AnimationTrigger::Count);
  }

  if(GetFaceID() != Vision::UnknownFaceID){
    DelegateIfInControl(new TrackFaceAction(GetFaceID()));
  }else{
    DelegateIfInControl( new HangAction() );
  }
  
  BehaviorObjectiveAchieved(BehaviorObjective::RequestedGame);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::TransitionToPlayingDenyAnim(BehaviorExternalInterface& behaviorExternalInterface)
{
  IActionRunner* denyAnimAction = new TriggerAnimationAction(_activeConfig->denyAnimTrigger );
  DelegateIfInControl(denyAnimAction);
  SET_STATE(PlayingDenyAnim);
} 


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  SetDebugStateName(stateName);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 BehaviorRequestGameSimple::GetNumBlocks(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if( _shouldUseBlocks ) {
    return ICozmoBehaviorRequestGame::GetNumBlocks(behaviorExternalInterface);
  }
  else {
    return 0;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRequestGameSimple::GetFaceInteractionPose(BehaviorExternalInterface& behaviorExternalInterface, Pose3d& targetPoseRet)
{
  Pose3d facePose;
  
  const bool hasFace = GetFacePose(behaviorExternalInterface, facePose);
  if(!hasFace) {
    PRINT_NAMED_WARNING("BehaviorRequestGameSimple.NoFace",
                        "Face pose is invalid!");
    return false;
  }
  
  auto& robotPose = behaviorExternalInterface.GetRobotInfo().GetPose();
  if( ! facePose.GetWithRespectTo(robotPose, facePose) ) {
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

    targetPose = Pose3d{ targetAngle, Z_AXIS_3D(), {relX, relY, 0.0f}, robotPose };
    targetPose = targetPose.GetWithRespectToRoot();

    BlockWorldFilter filter;
    filter.OnlyConsiderLatestUpdate(false);
    filter.SetFilterFcn( [&](const ObservableObject* obj) {
        if( obj->GetPoseState() != PoseState::Known ) { // TODO Brad review this. This is ignoring !Known which is not =Unknown
          // ignore unknown obstacles
          return false;
        }
        auto& carryingComp = behaviorExternalInterface.GetRobotInfo().GetCarryingComponent();
        if( carryingComp.IsCarryingObject() &&
            carryingComp.GetCarryingObject() == obj->GetID() ) {
          // ignore the block we are carrying
          return false;
        }

        float distSq = (obj->GetPose().GetWithRespectToRoot().GetTranslation() -
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
    behaviorExternalInterface.GetBlockWorld().FindLocatedMatchingObjects(filter, blocks);

    if(blocks.empty()) {
      targetPoseRet = targetPose;
      return true;
    }
  }

  PRINT_NAMED_INFO("BehaviorRequestGameSimple.NoSafeBlockPose",
                   "Could not find a safe place to put down the cube, using current position");  
  targetPoseRet = Pose3d{ targetAngle, Z_AXIS_3D(), {0.0f, 0.0f, 0.0f}, robotPose };
  targetPoseRet = targetPoseRet.GetWithRespectToRoot();

  return true;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRequestGameSimple::HandleGameDeniedRequest(BehaviorExternalInterface& behaviorExternalInterface)
{
  CancelDelegates(false);

  TransitionToPlayingDenyAnim(behaviorExternalInterface);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

} // namespace Cozmo
} // namespace Anki

