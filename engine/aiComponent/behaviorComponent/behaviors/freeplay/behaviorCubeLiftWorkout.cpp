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
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/events/animationTriggerHelpers.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
static const ObjectInteractionIntention kObjectIntention =
                   ObjectInteractionIntention::PickUpObjectNoAxisCheck;

static const f32 kPostLiftDriveBackwardDist_mm = 20.f;
static const f32 kPostLiftDriveBackwardSpeed_mmps = 100.f;  
  
constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersWorkoutArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           true},
  {ReactionTrigger::VC,                           true}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersWorkoutArray),
              "Reaction triggers duplicate or non-sequential");
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeLiftWorkout::BehaviorCubeLiftWorkout(const Json::Value& config)
: IBehavior(config)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCubeLiftWorkout::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  if( robot.GetCarryingComponent().IsCarryingObject() ) {
    return true;
  }
  else {
    auto& objInfoCache = behaviorExternalInterface.GetAIComponent().GetObjectInteractionInfoCache();
    const bool hasCube = objInfoCache.GetBestObjectForIntention(kObjectIntention).IsSet();
    return hasCube;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorCubeLiftWorkout::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  SmartDisableReactionsWithLock(GetIDStr(), kAffectTriggersWorkoutArray);

  // disable idle
  SmartPushIdleAnimation(behaviorExternalInterface, AnimationTrigger::Count);

  const auto& currWorkout = behaviorExternalInterface.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  
  _numStrongLiftsToDo = currWorkout.GetNumStrongLifts(robot);
  _numWeakLiftsToDo = currWorkout.GetNumWeakLifts(robot);

  if( robot.GetCarryingComponent().IsCarryingObject() ) {
    _shouldBeCarrying = true;
    _targetBlockID = robot.GetCarryingComponent().GetCarryingObject();
    TransitionToPostLiftAnim(behaviorExternalInterface);
  }
  else {
    _shouldBeCarrying = false;
    auto& objInfoCache = robot.GetAIComponent().GetObjectInteractionInfoCache();
    _targetBlockID = objInfoCache.GetBestObjectForIntention(kObjectIntention);

    TransitionToPickingUpCube(behaviorExternalInterface);
  }  
  
  return RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  auto publicStateBroadcaster = behaviorExternalInterface.GetRobotPublicStateBroadcaster().lock();
  if(publicStateBroadcaster != nullptr){
    publicStateBroadcaster->UpdateBroadcastBehaviorStage(BehaviorStageTag::Count, 0);
  }

  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    // Ensure the cube workout lights are not set
    robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::Workout, _targetBlockID);
  }

  
  // Need to re-compute whether or not eighties music should be played for the next workout
  behaviorExternalInterface.GetAIComponent().GetWorkoutComponent().ShouldRecomputeEightiesMusic();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorCubeLiftWorkout::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  if( _shouldBeCarrying && ! robot.GetCarryingComponent().IsCarryingObject() ) {
    PRINT_CH_INFO("Behaviors", (GetIDStr() + ".Update.NotCarryingWhenShould").c_str(),
                  "behavior thinks we should be carrying an object but we aren't, so it must have been detected on "
                  "the ground. Exit the behavior");
    // let the current animation finish, but then don't continue with the behavior
    StopOnNextActionComplete();
  }

  return super::UpdateInternal_WhileRunning(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToPickingUpCube(BehaviorExternalInterface& behaviorExternalInterface)
{
  
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    // Set the cube lights to the workout lights
    robot.GetCubeLightComponent().PlayLightAnim(_targetBlockID, CubeAnimationTrigger::Workout);
  }

  
  const auto& currWorkout = behaviorExternalInterface.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();

  
  PickupBlockParamaters params;
  params.animBeforeDock = currWorkout.preLiftAnim;
  params.allowedToRetryFromDifferentPose = true;
  
  auto& factory = behaviorExternalInterface.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();

  HelperHandle pickupHelper = factory.CreatePickupBlockHelper(behaviorExternalInterface, *this, _targetBlockID, params);
  SmartDelegateToHelper(behaviorExternalInterface, pickupHelper,
                        &BehaviorCubeLiftWorkout::TransitionToPostLiftAnim);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToPostLiftAnim(BehaviorExternalInterface& behaviorExternalInterface)
{
  // at this point we should be carrying the object
  _shouldBeCarrying = true;

  const auto& currWorkout = behaviorExternalInterface.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  CompoundActionSequential* driveBackAndAnimateAction = new CompoundActionSequential(robot);
  
  static const bool shouldIgnoreFailure = true;
  driveBackAndAnimateAction->AddAction(new DriveStraightAction(
                                             robot,
                                             -kPostLiftDriveBackwardDist_mm,
                                             kPostLiftDriveBackwardSpeed_mmps),
                                       shouldIgnoreFailure);

  
  if( currWorkout.postLiftAnim != AnimationTrigger::Count ) {
    driveBackAndAnimateAction->AddAction(new TriggerAnimationAction(robot, currWorkout.postLiftAnim));
  }
  
  DelegateIfInControl(driveBackAndAnimateAction,
              &BehaviorCubeLiftWorkout::TransitionToStrongLifts);
  
  // Update the music round if we're playing 80's music for this workout
  auto publicStateBroadcaster = behaviorExternalInterface.GetRobotPublicStateBroadcaster().lock();
  if (behaviorExternalInterface.GetAIComponent().GetWorkoutComponent().ShouldPlayEightiesMusic() &&
      publicStateBroadcaster != nullptr) {
    publicStateBroadcaster->UpdateBroadcastBehaviorStage(BehaviorStageTag::Workout,
                                                                   static_cast<uint8_t>(WorkoutStage::EightiesWorkoutLift));
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToStrongLifts(BehaviorExternalInterface& behaviorExternalInterface)
{
  PRINT_CH_INFO("Behaviors", "BehaviorCubeLiftWorkout.TransitionToStrongLifts", "%s: %d strong lifts remain",
                GetIDStr().c_str(),
                _numStrongLiftsToDo);
  
  if( _numStrongLiftsToDo == 0 ) {
    TransitionToWeakPose(behaviorExternalInterface);
  }
  else {
    const auto& currWorkout = behaviorExternalInterface.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    DelegateIfInControl(new TriggerAnimationAction(robot, currWorkout.strongLiftAnim, _numStrongLiftsToDo),
                &BehaviorCubeLiftWorkout::TransitionToWeakPose);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToWeakPose(BehaviorExternalInterface& behaviorExternalInterface)
{
  const auto& currWorkout = behaviorExternalInterface.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();
  if( currWorkout.transitionAnim == AnimationTrigger::Count ) {
    TransitionToWeakLifts(behaviorExternalInterface);
  }
  else {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    DelegateIfInControl(new TriggerAnimationAction(robot, currWorkout.transitionAnim),
                &BehaviorCubeLiftWorkout::TransitionToWeakLifts);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToWeakLifts(BehaviorExternalInterface& behaviorExternalInterface)
{
  PRINT_CH_INFO("Behaviors", "BehaviorCubeLiftWorkout.TransitionToWeakLifts", "%s: %d weak lifts remain",
                GetIDStr().c_str(),
                _numWeakLiftsToDo);

  if( _numWeakLiftsToDo == 0 ) {
    TransitionToPuttingDown(behaviorExternalInterface);
  }
  else {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    const auto& currWorkout = behaviorExternalInterface.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();
    DelegateIfInControl(new TriggerAnimationAction(robot, currWorkout.weakLiftAnim, _numWeakLiftsToDo),
                &BehaviorCubeLiftWorkout::TransitionToPuttingDown);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToPuttingDown(BehaviorExternalInterface& behaviorExternalInterface)
{
  _shouldBeCarrying = false;
  const auto& currWorkout = behaviorExternalInterface.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();
  if( currWorkout.putDownAnim == AnimationTrigger::Count ) {
    TransitionToManualPutDown(behaviorExternalInterface);
  }
  else {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    DelegateIfInControl(new TriggerAnimationAction(robot, currWorkout.putDownAnim),
                // double check the put down with a manual one, just in case
                &BehaviorCubeLiftWorkout::TransitionToCheckPutDown);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToCheckPutDown(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  // Stop the workout cube lights as soon as we think we have put the cube down
  robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::Workout, _targetBlockID);

  // if we still think we are carrying the object, see if we can find it
  if( robot.GetCarryingComponent().IsCarryingObject() ) {

    PRINT_CH_INFO("Behaviors", (GetIDStr() + ".StillCarryingCheck").c_str(),
                  "Robot still thinks it's carrying object, do a quick search for it");

    CompoundActionSequential* action = new CompoundActionSequential(robot);

    // lower lift so we can see (and possibly also put the cube down if it's really still in the lift)
    action->AddAction( new MoveLiftToHeightAction(robot, MoveLiftToHeightAction::Preset::LOW_DOCK) );
    action->AddAction( new SearchForNearbyObjectAction(robot, _targetBlockID) );
  
    DelegateIfInControl(action, &BehaviorCubeLiftWorkout::EndIteration);
  }
  else {
    // otherwise we are done, so finish the iteration
    EndIteration(behaviorExternalInterface);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToManualPutDown(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  // in case the put down didn't work, do it manually
  if( robot.GetCarryingComponent().IsCarryingObject() ) {
    PRINT_CH_INFO("Behaviors", (GetIDStr() + ".ManualPutDown").c_str(),
                  "Manually putting down object because animation (may have) failed to do it");
    DelegateIfInControl(new PlaceObjectOnGroundAction(robot), &BehaviorCubeLiftWorkout::EndIteration);
  }
  else {
    EndIteration(behaviorExternalInterface);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::EndIteration(BehaviorExternalInterface& behaviorExternalInterface)
{
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    // if we are _still_ holding the cube, then clearly we are confused, so just mark the cube as no longer in
    // the lift
    if( robot.GetCarryingComponent().IsCarryingObject() ) {
      robot.GetCarryingComponent().SetCarriedObjectAsUnattached();
    }
  }
  BehaviorObjectiveAchieved(BehaviorObjective::PerformedWorkout);
  NeedActionCompleted();
  
  BehaviorObjective additionalObjective = behaviorExternalInterface.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout().GetAdditionalBehaviorObjectiveOnComplete();
  if (additionalObjective != BehaviorObjective::Count) {
    BehaviorObjectiveAchieved(additionalObjective);
  }

  behaviorExternalInterface.GetAIComponent().GetWorkoutComponent().CompleteCurrentWorkout();
}

  
}
}

