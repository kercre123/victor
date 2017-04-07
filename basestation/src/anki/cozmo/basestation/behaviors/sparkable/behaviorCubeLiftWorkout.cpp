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

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorCubeLiftWorkout.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/behaviorSystem/objectInteractionInfoCache.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/workoutComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
static const ObjectInteractionIntention kObjectIntention =
                   ObjectInteractionIntention::PickUpAnyObject;

// if we haven't seen the cube this recently, search for it
// TODO:(bn) centralize this logic somewhere, every behavior should have access to this
static const uint32_t kMinAgeToPerformSearch_ms = 300;
static const f32 kPostLiftDriveBackwardDist_mm = 20.f;
static const f32 kPostLiftDriveBackwardSpeed_mmps = 100.f;
  
static const uint32_t kMaxReAlignsPickupFail = 1;
  
constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersWorkoutArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            false},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::PyramidInitialDetection,      false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::StackOfCubesInitialDetection, true},
  {ReactionTrigger::UnexpectedMovement,           true},
  {ReactionTrigger::VC,                           true}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersWorkoutArray),
              "Reaction triggers duplicate or non-sequential");
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeLiftWorkout::BehaviorCubeLiftWorkout(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _failToPickupCount(0)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCubeLiftWorkout::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const Robot& robot = preReqData.GetRobot();
  if( robot.IsCarryingObject() ) {
    return true;
  }
  else {
    auto& objInfoCache = robot.GetAIComponent().GetObjectInteractionInfoCache();
    const bool hasCube = objInfoCache.GetBestObjectForIntention(kObjectIntention).IsSet();
    return hasCube;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorCubeLiftWorkout::InitInternal(Robot& robot)
{
  SmartDisableReactionsWithLock(GetName(), kAffectTriggersWorkoutArray);

  // disable idle
  robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::Count);

  const auto& currWorkout = robot.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();  
  _numStrongLiftsToDo = currWorkout.GetNumStrongLifts(robot);
  _numWeakLiftsToDo = currWorkout.GetNumWeakLifts(robot);
  _failToPickupCount = 0;

  if( robot.IsCarryingObject() ) {
    _shouldBeCarrying = true;
    _targetBlockID = robot.GetCarryingObject();
    TransitionToPostLiftAnim(robot);
  }
  else {
    _shouldBeCarrying = false;
    auto& objInfoCache = robot.GetAIComponent().GetObjectInteractionInfoCache();
    _targetBlockID = objInfoCache.GetBestObjectForIntention(kObjectIntention);

    TransitionToAligningToCube(robot);
  }  
  
  return RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::StopInternal(Robot& robot)
{
  // restore previous idle
  robot.GetAnimationStreamer().PopIdleAnimation();
  
  // Ensure the cube workout lights are not set
  robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::Workout, _targetBlockID);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorCubeLiftWorkout::UpdateInternal(Robot& robot)
{
  if( _shouldBeCarrying && ! robot.IsCarryingObject() ) {
    PRINT_CH_INFO("Behaviors", (GetName() + ".Update.NotCarryingWhenShould").c_str(),
                  "behavior thinks we should be carrying an object but we aren't, so it must have been detected on "
                  "the ground. Exit the behavior");
    // let the current animation finish, but then don't continue with the behavior
    StopOnNextActionComplete();
  }

  return super::UpdateInternal(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToAligningToCube(Robot& robot)
{
  DriveToObjectAction* driveToBlockAction = new DriveToObjectAction(robot,
                                                                    _targetBlockID,
                                                                    PreActionPose::ActionType::DOCKING);

  auto& objInfoCache = robot.GetAIComponent().GetObjectInteractionInfoCache();

  RetryWrapperAction::RetryCallback retryCallback =
    [this, driveToBlockAction, &objInfoCache](const ExternalInterface::RobotCompletedAction& completion,
                               const u8 retryCount,
                               AnimationTrigger& retryAnimTrigger)
    {
      const bool blockStillValid = objInfoCache.IsObjectValidForInteraction(kObjectIntention, _targetBlockID);
      if( ! blockStillValid ) {
        return false;
      }

      retryAnimTrigger = AnimationTrigger::WorkoutPickupRealign;
    
      // Use a different preAction pose if we are retrying because we weren't seeing the object
      if(completion.result == ActionResult::VISUAL_OBSERVATION_FAILED)
      {
        using namespace std::placeholders;
        auto useSecondClosest = std::bind( &IBehavior::UseSecondClosestPreActionPose, this, driveToBlockAction, _1, _2, _3);
        driveToBlockAction->SetGetPossiblePosesFunc( useSecondClosest );
        return true;
      }
      else {
        return IActionRunner::GetActionResultCategory(completion.result) == ActionResultCategory::RETRY;
      }
    };

  static const u8 kNumRetries = 2;
  RetryWrapperAction* action = new RetryWrapperAction(robot, driveToBlockAction, retryCallback, kNumRetries);

  // Set the cube lights to the workout lights
  robot.GetCubeLightComponent().PlayLightAnim(_targetBlockID, CubeAnimationTrigger::Workout);

  StartActing(action, [this,&robot](const ExternalInterface::RobotCompletedAction& completion) {
      ActionResult res = completion.result;
      
      if( res == ActionResult::SUCCESS ) {
        TransitionToPreLiftAnim(robot);
      }
      else if(IActionRunner::GetActionResultCategory(res) != ActionResultCategory::CANCELLED) {
        // only count driving failures if there are no predock poses (that way a different cube or behavior
        // will get selected)
        const bool countFailure = (res == ActionResult::NO_PREACTION_POSES);
        TransitionToFailureRecovery(robot, countFailure);
      }
    });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToPreLiftAnim(Robot& robot)
{
  const auto& currWorkout = robot.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();  

  if( currWorkout.preLiftAnim == AnimationTrigger::Count ) {
    // skip the animation
    TransitionToPickingUpCube(robot);
  }
  else {
    StartActing(new TriggerLiftSafeAnimationAction(robot, currWorkout.preLiftAnim),
                &BehaviorCubeLiftWorkout::TransitionToPickingUpCube);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToPickingUpCube(Robot& robot)
{
  PickupObjectAction* pickupAction = new PickupObjectAction(robot, _targetBlockID);
  
  // we already checked this in the drive to, don't re-do driving if the animation moves us a bit
  pickupAction->SetDoNearPredockPoseCheck(false);
  
  CompoundActionSequential* pickupAndDriveBackAction = new CompoundActionSequential(robot, {
    pickupAction,
    new DriveStraightAction(robot, -kPostLiftDriveBackwardDist_mm, kPostLiftDriveBackwardSpeed_mmps),
  });
  pickupAndDriveBackAction->SetProxyTag(pickupAction->GetTag());

  StartActing(pickupAndDriveBackAction, [this, &robot](ActionResult res) {
      if( res == ActionResult::SUCCESS ) {
        TransitionToPostLiftAnim(robot);
      }
      else {
        if(_failToPickupCount < kMaxReAlignsPickupFail){
          _failToPickupCount++;
          TransitionToAligningToCube(robot);
        }else{
          const bool countFailure = true;
          TransitionToFailureRecovery(robot, countFailure);
        }
      }
    });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToPostLiftAnim(Robot& robot)
{
  // at this point we should be carrying the object
  _shouldBeCarrying = true;

  const auto& currWorkout = robot.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();  

  if( currWorkout.postLiftAnim == AnimationTrigger::Count ) {
    TransitionToStrongLifts(robot);
  }
  else {
    StartActing(new TriggerAnimationAction(robot, currWorkout.postLiftAnim),
                &BehaviorCubeLiftWorkout::TransitionToStrongLifts);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToStrongLifts(Robot& robot)
{
  PRINT_CH_INFO("Behaviors", "BehaviorCubeLiftWorkout.TransitionToStrongLifts", "%s: %d strong lifts remain",
                GetName().c_str(),
                _numStrongLiftsToDo);
  
  if( _numStrongLiftsToDo == 0 ) {
    TransitionToWeakPose(robot);
  }
  else {
    const auto& currWorkout = robot.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();  
    StartActing(new TriggerAnimationAction(robot, currWorkout.strongLiftAnim, _numStrongLiftsToDo),
                &BehaviorCubeLiftWorkout::TransitionToWeakPose);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToWeakPose(Robot& robot)
{
  const auto& currWorkout = robot.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();  
  if( currWorkout.transitionAnim == AnimationTrigger::Count ) {
    TransitionToWeakLifts(robot);
  }
  else {
    StartActing(new TriggerAnimationAction(robot, currWorkout.transitionAnim),
                &BehaviorCubeLiftWorkout::TransitionToWeakLifts);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToWeakLifts(Robot& robot)
{
  PRINT_CH_INFO("Behaviors", "BehaviorCubeLiftWorkout.TransitionToWeakLifts", "%s: %d weak lifts remain",
                GetName().c_str(),
                _numWeakLiftsToDo);

  if( _numWeakLiftsToDo == 0 ) {
    TransitionToPuttingDown(robot);
  }
  else {
    const auto& currWorkout = robot.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();  
    StartActing(new TriggerAnimationAction(robot, currWorkout.weakLiftAnim, _numWeakLiftsToDo),
                &BehaviorCubeLiftWorkout::TransitionToPuttingDown);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToPuttingDown(Robot& robot)
{
  _shouldBeCarrying = false;
  const auto& currWorkout = robot.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout();  
  if( currWorkout.putDownAnim == AnimationTrigger::Count ) {
    TransitionToManualPutDown(robot);
  }
  else {
    StartActing(new TriggerAnimationAction(robot, currWorkout.putDownAnim),
                // double check the put down with a manual one, just in case
                &BehaviorCubeLiftWorkout::TransitionToCheckPutDown);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToCheckPutDown(Robot& robot)
{
  // Stop the workout cube lights as soon as we think we have put the cube down
  robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::Workout, _targetBlockID);

  // if we still think we are carrying the object, see if we can find it
  if( robot.IsCarryingObject() ) {

    PRINT_CH_INFO("Behaviors", (GetName() + ".StillCarryingCheck").c_str(),
                  "Robot still thinks it's carrying object, do a quick search for it");

    CompoundActionSequential* action = new CompoundActionSequential(robot);

    // lower lift so we can see (and possibly also put the cube down if it's really still in the lift)
    action->AddAction( new MoveLiftToHeightAction(robot, MoveLiftToHeightAction::Preset::LOW_DOCK) );
    action->AddAction( new SearchForNearbyObjectAction(robot, _targetBlockID) );
  
    StartActing(action, &BehaviorCubeLiftWorkout::EndIteration);
  }
  else {
    // otherwise we are done, so finish the iteration
    EndIteration(robot);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToManualPutDown(Robot& robot)
{
  // in case the put down didn't work, do it manually
  if( robot.IsCarryingObject() ) {
    PRINT_CH_INFO("Behaviors", (GetName() + ".ManualPutDown").c_str(),
                  "Manually putting down object because animation (may have) failed to do it");
    StartActing(new PlaceObjectOnGroundAction(robot), &BehaviorCubeLiftWorkout::EndIteration);
  }
  else {
    EndIteration(robot);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::EndIteration(Robot& robot)
{
  // if we are _still_ holding the cube, then clearly we are confused, so just mark the cube as no longer in
  // the lift
  if( robot.IsCarryingObject() ) {
    robot.SetCarriedObjectAsUnattached();
  }

  BehaviorObjectiveAchieved(BehaviorObjective::PerformedWorkout);
  
  BehaviorObjective additionalObjective = robot.GetAIComponent().GetWorkoutComponent().GetCurrentWorkout().GetAdditionalBehaviorObjectiveOnComplete();
  if (additionalObjective != BehaviorObjective::Count) {
    BehaviorObjectiveAchieved(additionalObjective);
  }

  robot.GetAIComponent().GetWorkoutComponent().CompleteCurrentWorkout();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeLiftWorkout::TransitionToFailureRecovery(Robot& robot, bool countFailure)
{
  const ObservableObject* obj = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlockID);
  if( nullptr == obj ) {
    // bail out of the behavior
    return;
  }
  
  // if we haven't seen it recently, trigger a search
  const TimeStamp_t lastObservedTime = obj->GetLastObservedTime();
  const TimeStamp_t lastObservedDelta_ms = robot.GetLastImageTimeStamp() - lastObservedTime;
  const bool doSearchAction = lastObservedDelta_ms > kMinAgeToPerformSearch_ms;
  
  if( doSearchAction ) {
    SearchForNearbyObjectAction* searchAction = nullptr;
    
    // Workaround for COZMO-6657, if the pose is still Known, the search action will immediately succeed, so
    // do a search without a target blcok
    if( obj->IsPoseStateKnown() ){
      searchAction = new SearchForNearbyObjectAction(robot);
    }
    else {
      searchAction = new SearchForNearbyObjectAction(robot, _targetBlockID);
    }

    StartActing(searchAction,
                [this, &robot, lastObservedTime, countFailure](ActionResult res) {
                  if( res == ActionResult::SUCCESS ) {
                    // check if it was actually observed again (another workaround for COZMO-6657)
                    const ObservableObject* obj = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlockID);
                    if( nullptr != obj ) {
                      const TimeStamp_t mostRecentObservation = obj->GetLastObservedTime();
                      const bool wasObserved = mostRecentObservation > lastObservedTime;

                      if( wasObserved ) {                
                        // found the block, restart the behavior!
                        InitInternal(robot);
                        return;
                      }
                    }
                  }

                  if( countFailure ) {
                    // if we get here, we didn't find the block (or had another failure), set failure and bail out
                    const ObservableObject* failedObject = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlockID);
                    if(failedObject){
                      const auto objectAction = AIWhiteboard::ObjectActionFailure::PickUpObject;
                      robot.GetAIComponent().GetWhiteboard().SetFailedToUse(*failedObject, objectAction);
                    }
                  }
                });
  }
  else {
    if( countFailure ) {
      // we don't want to search, so just set a failure and bail from the behavior
      const ObservableObject* failedObject = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlockID);
      if(failedObject){
        const auto objectAction = AIWhiteboard::ObjectActionFailure::PickUpObject;
        robot.GetAIComponent().GetWhiteboard().SetFailedToUse(*failedObject, objectAction);
      }
    }
  }
}

}
}

