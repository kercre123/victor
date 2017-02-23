/**
 * File: reactionTriggerStrategyPet.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/


#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPetInitialDetection.h"

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgePet.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/petWorld.h"
#include "anki/common/basestation/utils/timer.h"
#include "util/console/consoleInterface.h"

#define LOG_DEBUG(...) PRINT_CH_DEBUG("ReactionTriggers", ##__VA_ARGS__)
#define LOG_INFO(...) PRINT_CH_INFO("ReactionTriggers", ##__VA_ARGS__)

namespace {
static const char* kTriggerStrategyName = "Trigger Strategy Pet detected";
  
// Console parameters
#define CONSOLE_GROUP "Behavior.ReactToPet"
  
CONSOLE_VAR(s32, kReactToPetNumTimesObserved, CONSOLE_GROUP, 3);
CONSOLE_VAR(f32, kReactToPetCooldown_s, CONSOLE_GROUP, 60.0f);
CONSOLE_VAR(bool, kReactToPetEnable, CONSOLE_GROUP, true);
}


namespace Anki {
namespace Cozmo {

ReactionTriggerStrategyPetInitialDetection::ReactionTriggerStrategyPetInitialDetection(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
, _robot(robot)
{
  
}

//
// Called at the start of each tick when behavior is runnable but not active.
// Return true if behavior should become active.
//
bool ReactionTriggerStrategyPetInitialDetection::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  // Keep track of petIDs observed during cooldown.  This prevents Cozmo from
  // suddenly reacting to a "known" pet when cooldown expires.
  if (RecentlyReacted()) {
    LOG_INFO("ReactStratPetInitialDetect.ShouldSwitch.RecentlyReacted", "Recently reacted to a pet");
    UpdateReactedTo(robot);
    return false;
  }
  
  // Check for new pets
  const auto & petWorld = robot.GetPetWorld();
  const auto & pets = petWorld.GetAllKnownPets();
  
  for (const auto & it : pets) {
    const auto petID = it.first;
    if (_reactedTo.find(petID) != _reactedTo.end()) {
      LOG_DEBUG("ReactStratPetInitialDetect.ShouldSwitch.AlreadyReacted", "Already reacted to petID %d", petID);
      continue;
    }
    const auto & pet = it.second;
    const auto numTimesObserved = pet.GetNumTimesObserved();
    if (numTimesObserved < kReactToPetNumTimesObserved) {
      LOG_DEBUG("ReactStratPetInitialDetect.ShouldSwitch.NumTimesObserved",
                "PetID %d does not meet observation threshold (%d < %d)",
                petID, numTimesObserved, kReactToPetNumTimesObserved);
      continue;
    }
    _targets.insert(petID);
  }
  
  if (_targets.empty()) {
    return false;
  }

  if (!kReactToPetEnable) {
    LOG_DEBUG("ReactStratPetInitialDetect.IsRunnable.ReactionDisabled", "Reaction is disabled");
    return false;
  }
  
  if (robot.IsOnChargerPlatform()) {
    LOG_DEBUG("ReactStratPetInitialDetect.IsRunnable.RobotOnCharger", "Robot is on charger");
    return false;
  }
  
  // If we found a good target, behavior should become active.
  BehaviorPreReqAcknowledgePet acknowledgePetPreReqs(_targets);
  return behavior->IsRunnable(acknowledgePetPreReqs);
}
  

bool ReactionTriggerStrategyPetInitialDetection::RecentlyReacted() const
{
  if (_lastReactionTime_s > NEVER) {
    if (_lastReactionTime_s + kReactToPetCooldown_s > BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
      return true;
    }
  }
  return false;
}
  
  
void ReactionTriggerStrategyPetInitialDetection::BehaviorThatStrategyWillTrigger(IBehavior* behavior)
{
  behavior->AddListener(this);
}

void ReactionTriggerStrategyPetInitialDetection::BehaviorDidReact(const std::set<Vision::FaceID_t> & targets)
{
  //
  // Remember all petIDs at end of iteration to prevent triggering twice for the same petID.
  // Note that if pet moves out of frame, then back into frame, it may be assigned a new petID.
  // This means the behavior may trigger again for the same pet, but only if we lose track of the pet.
  //
  LOG_DEBUG("ReactStratPetInitialDetect.BehaviorDidReact", "Reacted to %zu targets", targets.size());

  InitReactedTo(_robot);
  _reactedTo.insert(targets.begin(), targets.end());
  _lastReactionTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _targets.clear();

}

//
// Called to update list of petIDs that we should not trigger reaction.
// Any petIDs observed during cooldown will not trigger reaction.
//
void ReactionTriggerStrategyPetInitialDetection::UpdateReactedTo(const Robot& robot)
{
  const auto & petWorld = robot.GetPetWorld();
  for (const auto & it : petWorld.GetAllKnownPets()) {
    const auto petID = it.first;
    _reactedTo.insert(petID);
  }
}

void ReactionTriggerStrategyPetInitialDetection::InitReactedTo(const Robot& robot)
{
  _reactedTo.clear();
  UpdateReactedTo(robot);
}
  
  
  
} // namespace Cozmo
} // namespace Anki
