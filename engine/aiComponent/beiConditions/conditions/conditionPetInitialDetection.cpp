/**
* File: ConditionPetInitialDetection.h
*
* Author: Kevin M. Karol
* Created: 11/1/17
*
* Description: Strategy for responding to a pet being detected
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionPetInitialDetection.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/petWorld.h"

#define LOG_CHANNEL "BehaviorSystem"

namespace Anki {
namespace Cozmo {

namespace {
static const int kReactToPetNumTimesObserved = 3;
}    
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionPetInitialDetection::ConditionPetInitialDetection(const Json::Value& config, BEIConditionFactory& factory)
  : IBEICondition(config, factory)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionPetInitialDetection::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{ 
  // Check for new pets
  const auto & petWorld = behaviorExternalInterface.GetPetWorld();
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
    _reactedTo.insert(petID);
    return true;
  }
  
  return false;
}


} // namespace Cozmo
} // namespace Anki
