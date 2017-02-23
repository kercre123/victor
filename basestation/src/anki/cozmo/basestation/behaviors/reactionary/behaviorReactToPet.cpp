/**
 * File: behaviorReactToPet.cpp
 *
 * Description:  Simple reaction to a pet. Cozmo plays a reaction animation, then tracks the pet for a random interval.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToPet.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorListenerInterfaces/iReactToPetListener.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgePet.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/petWorld.h"
#include "anki/common/basestation/utils/timer.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
namespace {

// Log channel
static const char* kLogChannel = "Behaviors";

// Console parameters
#define CONSOLE_GROUP "Behavior.ReactToPet"

CONSOLE_VAR(f32, kReactToPetMinTime_s, CONSOLE_GROUP, 10.0f);
CONSOLE_VAR(f32, kReactToPetMaxTime_s, CONSOLE_GROUP, 15.0f);
CONSOLE_VAR(s32, kReactToPetSneezePercent, CONSOLE_GROUP, 20);
CONSOLE_VAR(f32, kReactToPetTrackUpdateTimeout, CONSOLE_GROUP, 3.0f);
  
} // end namespace
  
using namespace Anki::Vision;
  
// Enable low-level diagnostics?
#define DEBUG_PET_REACTION 0

#define PRINT_DEBUG(name, ...) PRINT_CH_DEBUG(kLogChannel, name, ##__VA_ARGS__)
#define PRINT_INFO(name, ...)  PRINT_CH_INFO(kLogChannel, name, ##__VA_ARGS__)
    
#if DEBUG_PET_REACTION
#define PRINT_TRACE(name, ...) PRINT_CH_DEBUG(kLogChannel, name, ##__VA_ARGS__)
#else
#define PRINT_TRACE(name...) {}
#endif

BehaviorReactToPet::BehaviorReactToPet(Robot& robot, const Json::Value& config)
  : super(robot, config)
{
  PRINT_TRACE("ReactToPet.Constructor", "Constructor");
  SetDefaultName("ReactToPet");
}


//
// Called at the start of each tick. Return true if behavior can run.
//
bool BehaviorReactToPet::IsRunnableInternal(const BehaviorPreReqAcknowledgePet& preReqData) const
{
  if (AlreadyReacting()) {
    PRINT_TRACE("ReactToPet.IsRunnable.AlreadyReacting", "Already reacting to a pet");
    return true;
  }
  
  // Retain list of targets that triggered behavior
  _targets = preReqData.GetTargets();

  // Trigger should always provide targets
  DEV_ASSERT(_targets.size() > 0, "BehaviorReactToPet.IsRunnable.NoTargets");
  
  PRINT_TRACE("ReactToPet.IsRunnable.Runnable", "Behavior is runnable with %z targets", _targets.size());
  
  return true;
}
  

//
// Called each time behavior becomes active.
//
Result BehaviorReactToPet::InitInternal(Robot& robot)
{
  PRINT_INFO("ReactToPet.Init.BeginIteration", "Begin iteration");
  BeginIteration(robot);
  return RESULT_OK;
}
  
//
// Called each tick while behavior is active.
//
IBehavior::Status BehaviorReactToPet::UpdateInternal(Robot& robot)
{
  //
  // If target disappears during iteration, end iteration immediately.
  // This can happen if (e.g.) pet disappears during transition from previous
  // behavior.
  //
  if (_target == UnknownFaceID) {
    PRINT_TRACE("ReactToPet.Update.MissingTarget", "Missing target during update");
    EndIteration(robot);
    return Status::Complete;
  }
  
  //
  // If we have reached time limit, end the iteration.  This is the normal
  // end condition for each iteration.
  //
  if (_endReactionTime_s > NEVER) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if (_endReactionTime_s < currTime_s) {
      PRINT_TRACE("ReactToPet.Update.ReactionTimeExpired", "Reaction time has expired");
      EndIteration(robot);
      return Status::Complete;
    }
  }
  PRINT_TRACE("ReactToPet.Update.Running", "Behavior is running");
  return Status::Running;
}

//
// Called when behavior becomes inactive.
// This can happen if behavior is preempted during iteration.
//
void BehaviorReactToPet::StopInternal(Robot& robot)
{
  PRINT_TRACE("ReactToPet.Stop", "Stop behavior");
  
  // Notify listeners
  for (auto * listener: _petListeners) {
    listener->BehaviorDidReact(_targets);
  }
  
  // Update target state
  _endReactionTime_s = NEVER;
  _target = UnknownFaceID;
  _targets.clear();
  
}
  
//
// Which animation trigger do we play for this pet?
//
AnimationTrigger BehaviorReactToPet::GetAnimationTrigger(Vision::PetType petType)
{
  // Random chance that Cozmo will respond to cat or dog with sneeze
  if (petType == PetType::Cat || petType == PetType::Dog) {
    Util::RandomGenerator& rng = GetRNG();
    if (rng.RandInt(100) < kReactToPetSneezePercent) {
      petType = PetType::Unknown;
    }
  }
  switch (petType) {
    case PetType::Cat:
      return AnimationTrigger::PetDetectionCat;
    case PetType::Dog:
      return AnimationTrigger::PetDetectionDog;
    case PetType::Unknown:
      return AnimationTrigger::PetDetectionSneeze;
  }
}

//
// Called by ReactToPet.Init to start reaction cycle
//
void BehaviorReactToPet::BeginIteration(Robot& robot)
{
  // Trigger should always provide targets
  DEV_ASSERT(!_targets.empty(), "BehaviorReactToPet.BeginIteration.NoTargets");
  
  _target = Vision::UnknownFaceID;
  
  // React to the first valid target.  Don't worry about choosing "best".
  const auto & petWorld = robot.GetPetWorld();
  const Vision::TrackedPet * pet = nullptr;
  
  for (auto petID : _targets) {
    pet = petWorld.GetPetByID(petID);
    if (nullptr != pet) {
      _target = petID;
      break;
    }
  }
  
  if (_target == Vision::UnknownFaceID) {
    PRINT_NAMED_ERROR("ReactToPet.BeginIteration.NoValidTarget", "No valid target");
    return;
  }
  
  // Get info on this pet
  const auto petType = pet->GetType();
  const auto & petRect = pet->GetRect();
  const Point2f petXY(petRect.GetXmid(), petRect.GetYmid());
  
  // Which animation do we play?
  AnimationTrigger trigger = GetAnimationTrigger(petType);
  
  // Tracking animations do not end by themselves.
  // Choose a random duration and rely on UpdateInternal() to end the action.
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const float randTime_s = Util::numeric_cast<float>(robot.GetRNG().RandDblInRange(kReactToPetMinTime_s, kReactToPetMaxTime_s));
  const float endTime_s = currTime_s + randTime_s;

  PRINT_INFO("ReactToPet.BeginIteration", "Reacting to petID %d type %d from t=%f to t=%f",
             _target, (int)petType, currTime_s, endTime_s);
  
  // Compose reaction sequence:
  //
  // 1) Turn toward pet
  // 2) Play reaction animation
  // 3) Track pet for remaining time
  //
  // Note that pet may move out of frame during animation, so Cozmo will track *any* petID
  // that is visible after reaction.
  //
  // If playing animation takes too long, the original petXY may be pushed out of pose history.
  // A better version would rescan pet world after animation has completed.
  //
  auto turnAction = new TurnTowardsImagePointAction(robot, petXY, pet->GetTimeStamp());
  auto animAction = new TriggerAnimationAction(robot, trigger);
  auto trackAction = new TrackPetFaceAction(robot, Vision::PetType::Unknown);
  
  // Limit the amount of time that tracker will run without finding a target.
  // If pets are visible after animation, behavior will run until ended by update method.
  // If no pets are visible, behavior will end after tracking timeout.
  trackAction->SetUpdateTimeout(kReactToPetTrackUpdateTimeout);
  
  auto compoundAction = new CompoundActionSequential(robot);
  compoundAction->AddAction(turnAction);
  compoundAction->AddAction(animAction);
  compoundAction->AddAction(trackAction);
  
  // Set time limit on reaction sequence
  _endReactionTime_s = endTime_s;
  
  // Begin reaction sequence
  StartActing(compoundAction, &BehaviorReactToPet::EndIteration);
}

//
// Called by ReactToPet.Update() to end reaction cycle.
// Could also be called by tracking action if action reaches end.
//
void BehaviorReactToPet::EndIteration(Robot& robot)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  PRINT_INFO("ReactToPet.EndIteration", "End iteration for petID %d at t=%f", _target, currTime_s);
  
  // Notify listeners
  for (auto * listener: _petListeners) {
    listener->BehaviorDidReact(_targets);
  }
  
  // Update target state
  _targets.clear();
  _target = UnknownFaceID;
  _endReactionTime_s = NEVER;

  // Do we need a behavior objective?
  // BehaviorObjectiveAchieved(BehaviorObjective::ReactedToPet);
}

bool BehaviorReactToPet::AlreadyReacting() const
{
  return (_target != Vision::UnknownFaceID);
}
  
void BehaviorReactToPet::AddListener(IReactToPetListener* listener)
{
  _petListeners.insert(listener);
}

  
} // namespace Cozmo
} // namespace Anki

