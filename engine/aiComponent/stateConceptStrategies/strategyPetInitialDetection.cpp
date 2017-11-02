/**
* File: StrategyPetInitialDetection.h
*
* Author: Kevin M. Karol
* Created: 11/1/17
*
* Description: Strategy for responding to a pet being detected
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/stateConceptStrategies/strategyPetInitialDetection.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/petWorld.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
#define LOG_DEBUG(...) PRINT_CH_DEBUG("BehaviorSystem", ##__VA_ARGS__)
#define LOG_INFO(...) PRINT_CH_INFO("BehaviorSystem", ##__VA_ARGS__)

namespace {
static const int kReactToPetNumTimesObserved = 3;
}    
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyPetInitialDetection::StrategyPetInitialDetection(BehaviorExternalInterface& behaviorExternalInterface,
                                                         IExternalInterface* robotExternalInterface,
                                                         const Json::Value& config)
: IStateConceptStrategy(behaviorExternalInterface, robotExternalInterface, config)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyPetInitialDetection::AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{ 
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
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
    _reactedTo.insert(petID);
    return true;
  }
  
  return false;
}


} // namespace Cozmo
} // namespace Anki
