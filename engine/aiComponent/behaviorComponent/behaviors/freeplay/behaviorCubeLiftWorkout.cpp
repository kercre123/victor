/**
 * File: behaviorCubeLiftWorkout.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-10-24
 *
 * Description: Behavior to lift a cube and do a "workout" routine with it, with animations and parameters
 *              defined in json
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorCubeLiftWorkout.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/retryWrapperAction.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/aiComponent/workoutComponent.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/events/animationTriggerHelpers.h"

namespace Anki {
namespace Cozmo {

namespace {
static const ObjectInteractionIntention kObjectIntention =
                   ObjectInteractionIntention::PickUpObjectNoAxisCheck;

static const f32 kPostLiftDriveBackwardDist_mm = 20.f;
static const f32 kPostLiftDriveBackwardSpeed_mmps = 100.f;  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeLiftWorkout::BehaviorCubeLiftWorkout(const Json::Value& config)
: ICozmoBehavior(config)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCubeLiftWorkout::WantsToBeActivatedBehavior() const
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  if( robotInfo.GetCarryingComponent().IsCarryingObject() ) {
    return true;
  }
  else {
    auto& objInfoCache = GetBEI().GetAIComponent().GetObjectInteractionInfoCache();
    const bool hasCube = objInfoCache.GetBestObjectForIntention(kObjectIntention).IsSet();
    return hasCube;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::OnBehaviorActivated()
{
  // disable idle
  SmartPushIdleAnimation(AnimationTrigger::Count);

  const auto& currWorkout = GetBEI().GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();

  if(GetBEI().HasMoodManager()){
    _numStrongLiftsToDo = currWorkout.GetNumStrongLifts(GetBEI().GetMoodManager());
    _numWeakLiftsToDo = currWorkout.GetNumWeakLifts(GetBEI().GetMoodManager());
  }else{
    PRINT_NAMED_WARNING("BehaviorCubeLiftWorkout.OnBehaviorActivated.NoMoodManager","");
  }


  const auto& robotInfo = GetBEI().GetRobotInfo();
  if( robotInfo.GetCarryingComponent().IsCarryingObject() ) {
    _shouldBeCarrying = true;
    _targetBlockID = robotInfo.GetCarryingComponent().GetCarryingObject();
    TransitionToPostLiftAnim();
  }
  else {
    _shouldBeCarrying = false;
    auto& objInfoCache = GetBEI().GetAIComponent().GetObjectInteractionInfoCache();
    _targetBlockID = objInfoCache.GetBestObjectForIntention(kObjectIntention);

    TransitionToPickingUpCube();
  }    
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::OnBehaviorDeactivated()
{
  if(GetBEI().HasPublicStateBroadcaster()){
    auto& publicStateBroadcaster = GetBEI().GetRobotPublicStateBroadcaster();
    publicStateBroadcaster.UpdateBroadcastBehaviorStage(BehaviorStageTag::Count, 0);
  }

  {
    // Ensure the cube workout lights are not set
    GetBEI().GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::Workout, _targetBlockID);
  }

  
  // Need to re-compute whether or not eighties music should be played for the next workout
  GetBEI().GetAIComponent().GetWorkoutComponent().ShouldRecomputeEightiesMusic();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  const auto& robotInfo = GetBEI().GetRobotInfo();
  if( _shouldBeCarrying && ! robotInfo.GetCarryingComponent().IsCarryingObject() ) {
    PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".Update.NotCarryingWhenShould").c_str(),
                  "behavior thinks we should be carrying an object but we aren't, so it must have been detected on "
                  "the ground. Exit the behavior");
    // let the current animation finish, but then don't continue with the behavior
    StopOnNextActionComplete();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToPickingUpCube()
{
  // Set the cube lights to the workout lights
  GetBEI().GetCubeLightComponent().PlayLightAnim(_targetBlockID, CubeAnimationTrigger::Workout);
  
  const auto& currWorkout = GetBEI().GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();

  
  PickupBlockParamaters params;
  params.animBeforeDock = currWorkout.preLiftAnim;
  params.allowedToRetryFromDifferentPose = true;
  
  auto& factory = GetBEI().GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();

  HelperHandle pickupHelper = factory.CreatePickupBlockHelper(*this, _targetBlockID, params);
  SmartDelegateToHelper(pickupHelper, &BehaviorCubeLiftWorkout::TransitionToPostLiftAnim);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToPostLiftAnim()
{
  // at this point we should be carrying the object
  _shouldBeCarrying = true;

  const auto& currWorkout = GetBEI().GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();

  CompoundActionSequential* driveBackAndAnimateAction = new CompoundActionSequential();
  
  static const bool shouldIgnoreFailure = true;
  driveBackAndAnimateAction->AddAction(new DriveStraightAction(-kPostLiftDriveBackwardDist_mm,
                                                               kPostLiftDriveBackwardSpeed_mmps),
                                                               shouldIgnoreFailure);

  
  if( currWorkout.postLiftAnim != AnimationTrigger::Count ) {
    driveBackAndAnimateAction->AddAction(new TriggerAnimationAction(currWorkout.postLiftAnim));
  }
  
  DelegateIfInControl(driveBackAndAnimateAction,
              &BehaviorCubeLiftWorkout::TransitionToStrongLifts);
  
  // Update the music round if we're playing 80's music for this workout
  if (GetBEI().GetAIComponent().GetWorkoutComponent().ShouldPlayEightiesMusic() &&
      GetBEI().HasPublicStateBroadcaster()) {
    auto& publicStateBroadcaster = GetBEI().GetRobotPublicStateBroadcaster();
    publicStateBroadcaster.UpdateBroadcastBehaviorStage(BehaviorStageTag::Workout,
                                                        static_cast<uint8_t>(WorkoutStage::EightiesWorkoutLift));
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToStrongLifts()
{
  PRINT_CH_INFO("Behaviors", "BehaviorCubeLiftWorkout.TransitionToStrongLifts", "%s: %d strong lifts remain",
                GetDebugLabel().c_str(),
                _numStrongLiftsToDo);
  
  if( _numStrongLiftsToDo == 0 ) {
    TransitionToWeakPose();
  }
  else {
    const auto& currWorkout = GetBEI().GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();
    DelegateIfInControl(new TriggerAnimationAction(currWorkout.strongLiftAnim, _numStrongLiftsToDo),
                &BehaviorCubeLiftWorkout::TransitionToWeakPose);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToWeakPose()
{
  const auto& currWorkout = GetBEI().GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();
  if( currWorkout.transitionAnim == AnimationTrigger::Count ) {
    TransitionToWeakLifts();
  }
  else {
    DelegateIfInControl(new TriggerAnimationAction(currWorkout.transitionAnim),
                &BehaviorCubeLiftWorkout::TransitionToWeakLifts);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToWeakLifts()
{
  PRINT_CH_INFO("Behaviors", "BehaviorCubeLiftWorkout.TransitionToWeakLifts", "%s: %d weak lifts remain",
                GetDebugLabel().c_str(),
                _numWeakLiftsToDo);

  if( _numWeakLiftsToDo == 0 ) {
    TransitionToPuttingDown();
  }
  else {
    const auto& currWorkout = GetBEI().GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();
    DelegateIfInControl(new TriggerAnimationAction(currWorkout.weakLiftAnim, _numWeakLiftsToDo),
                &BehaviorCubeLiftWorkout::TransitionToPuttingDown);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToPuttingDown()
{
  _shouldBeCarrying = false;
  const auto& currWorkout = GetBEI().GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();
  if( currWorkout.putDownAnim == AnimationTrigger::Count ) {
    TransitionToManualPutDown();
  }
  else {
    DelegateIfInControl(new TriggerAnimationAction(currWorkout.putDownAnim),
                // double check the put down with a manual one, just in case
                &BehaviorCubeLiftWorkout::TransitionToCheckPutDown);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToCheckPutDown()
{
  // Stop the workout cube lights as soon as we think we have put the cube down
  GetBEI().GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::Workout, _targetBlockID);

  // if we still think we are carrying the object, see if we can find it
  if( GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject() ) {

    PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".StillCarryingCheck").c_str(),
                  "Robot still thinks it's carrying object, do a quick search for it");

    CompoundActionSequential* action = new CompoundActionSequential();

    // lower lift so we can see (and possibly also put the cube down if it's really still in the lift)
    action->AddAction( new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK) );
    action->AddAction( new SearchForNearbyObjectAction(_targetBlockID) );
  
    DelegateIfInControl(action, &BehaviorCubeLiftWorkout::EndIteration);
  }
  else {
    // otherwise we are done, so finish the iteration
    EndIteration();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToManualPutDown()
{
  auto& robotInfo = GetBEI().GetRobotInfo();
  // in case the put down didn't work, do it manually
  if( robotInfo.GetCarryingComponent().IsCarryingObject() ) {
    PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".ManualPutDown").c_str(),
                  "Manually putting down object because animation (may have) failed to do it");
    DelegateIfInControl(new PlaceObjectOnGroundAction(), &BehaviorCubeLiftWorkout::EndIteration);
  }
  else {
    EndIteration();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::EndIteration()
{
  {
    auto& robotInfo = GetBEI().GetRobotInfo();
    // if we are _still_ holding the cube, then clearly we are confused, so just mark the cube as no longer in
    // the lift
    if( robotInfo.GetCarryingComponent().IsCarryingObject() ) {
      robotInfo.GetCarryingComponent().SetCarriedObjectAsUnattached();
    }
  }
  BehaviorObjectiveAchieved(BehaviorObjective::PerformedWorkout);
  NeedActionCompleted();
  
  BehaviorObjective additionalObjective = GetBEI().GetAIComponent().GetWorkoutComponent().GetCurrentWorkout().GetAdditionalBehaviorObjectiveOnComplete();
  if (additionalObjective != BehaviorObjective::Count) {
    BehaviorObjectiveAchieved(additionalObjective);
  }

  GetBEI().GetAIComponent().GetWorkoutComponent().CompleteCurrentWorkout();
}

  
}
}

