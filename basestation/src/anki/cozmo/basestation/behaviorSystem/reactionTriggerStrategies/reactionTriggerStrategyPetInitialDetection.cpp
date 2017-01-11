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

namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Pet detected";
  
// Console parameters
#define CONSOLE_GROUP "Behavior.ReactToPet"
  
CONSOLE_VAR(s32, kReactToPetNumTimesObserved, CONSOLE_GROUP, 3);
CONSOLE_VAR(f32, kReactToPetCooldown_s, CONSOLE_GROUP, 60.0f);
CONSOLE_VAR(bool, kReactToPetEnable, CONSOLE_GROUP, true);
  
static inline double GetCurrentTimeInSeconds() {
  return Anki::BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}
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
    PRINT_CH_INFO("ReactionTriggers",
                  "ReactStratPetInitialDetect.ShouldSwitch.RecentlyReacted",
                  "Recently reacted to a pet");
    UpdateReactedTo(robot);
    return false;
  }
  
  // Check for new pets
  const auto & petWorld = robot.GetPetWorld();
  const auto & pets = petWorld.GetAllKnownPets();
  
  for (const auto & it : pets) {
    const auto petID = it.first;
    if (_reactedTo.find(petID) != _reactedTo.end()) {
      PRINT_CH_INFO("ReactionTriggers",
                    "ReactStratPetInitialDetect.ShouldSwitch.AlreadyReacted",
                    "Already reacted to petID %d", petID);
      continue;
    }
    const auto & pet = it.second;
    const auto numTimesObserved = pet.GetNumTimesObserved();
    if (numTimesObserved < kReactToPetNumTimesObserved) {
      PRINT_CH_INFO("ReactionTriggers",
                    "ReactStratPetInitialDetect.ShouldSwitch.NumTimesObserved",
                    "PetID %d does not meet observation threshold (%d < %d)",
                  petID, numTimesObserved, kReactToPetNumTimesObserved);
      continue;
    }
    _targets.insert(petID);
  }
  
  
  if (!kReactToPetEnable) {
    PRINT_CH_INFO("ReactionTriggers",
                  "ReactStratPetInitialDetect.IsRunnable.ReactionDisabled",
                  "Reaction is disabled");
    return false;
  }
  
  if(!_targets.empty()){
    if (robot.IsOnCharger() || robot.IsOnChargerPlatform()) {
      PRINT_CH_INFO("ReactionTriggers",
                    "ReactStratPetInitialDetect.IsRunnable.RobotOnCharger",
                    "Robot is on charger");
      return false;
    }
    
    BehaviorPreReqAcknowledgePet acknowledgePetPreReqs(_targets);
    // If we found a good target, behavior should become active.
    return behavior->IsRunnable(acknowledgePetPreReqs);
  }
  
  return false;
}
  

bool ReactionTriggerStrategyPetInitialDetection::RecentlyReacted() const
{
  if (_lastReactionTime_s > NEVER) {
    if (_lastReactionTime_s + kReactToPetCooldown_s > GetCurrentTimeInSeconds()) {
      return true;
    }
  }
  return false;
}
  
  
void ReactionTriggerStrategyPetInitialDetection::BehaviorThatStartegyWillTrigger(IBehavior* behavior)
{
  behavior->AddListener(this);
}


void ReactionTriggerStrategyPetInitialDetection::RefreshReactedToIDs()
{
  InitReactedTo(_robot);
}


void ReactionTriggerStrategyPetInitialDetection::UpdateLastReactionTime()
{
  _lastReactionTime_s = GetCurrentTimeInSeconds();
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
