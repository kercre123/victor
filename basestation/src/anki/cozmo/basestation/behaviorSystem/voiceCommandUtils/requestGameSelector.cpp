/**
 * File: requestGameSelector.cpp
 *
 * Author: Kevin M. Karol
 * Created: 05/18/17
 *
 * Description: Selector used by the voice command activity to choose the best
 * game to request upon hearing the player ask to play a game
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/voiceCommandUtils/requestGameSelector.h"

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki{
namespace Cozmo{

namespace{
const char * kUnlockIDConfigKey   = "unlockID";
const char * kBehaviorIDConfigKey = "behaviorID";
const char * kWeightConfigKey     = "weight";
const int kMaxRetrys = 1000;
}
  


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RequestGameSelector::RequestGameSelector(Robot& robot)
: _lastGameRequested(nullptr)
{
  // load the lets play mapping in from JSON
  const Json::Value& letsPlayConfig = robot.GetContext()->GetDataLoader()->GetLetsPlayWeightsConfig();
  const auto& BF = robot.GetBehaviorFactory();
  for(const auto& entry: letsPlayConfig){
    UnlockId unlockID = UnlockIdFromString(
              JsonTools::ParseString(entry,
                                     kUnlockIDConfigKey,
                                     "RequestGameSelector.UnlockID"));
    IBehavior* behaviorPtr = BF.FindBehaviorByID(
                   BehaviorIDFromString(
                     JsonTools::ParseString(entry,
                                            kBehaviorIDConfigKey,
                                            "RequestGameSelector.BehaviorID")));
    int weight = JsonTools::ParseUint8(entry,
                                       kWeightConfigKey,
                                       "RequestGameSelector.Weight");
    DEV_ASSERT_MSG(behaviorPtr != nullptr &&
                   behaviorPtr->GetClass() == BehaviorClass::RequestGameSimple,
                   "ActivityVoiceCommand.InvalidGameClass.ImproperClassRetrievedForName",
                   "Behavior with ID %s is of improper class",
                   behaviorPtr != nullptr ? behaviorPtr->GetIDStr().c_str() : "nullPtr returned");
    
    _gameRequests.emplace_back(GameRequestData(unlockID, behaviorPtr, weight));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* RequestGameSelector::GetNextRequestGameBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  // Map weight to behavior for unlocked
  std::vector<std::pair<int, IBehavior*>> unlockedBehaviors;
  int totalWeight = 0;
  for(const auto& entry: _gameRequests){
    if(robot.GetProgressionUnlockComponent().IsUnlocked(entry._unlockID)){
      totalWeight += entry._weight;
      unlockedBehaviors.emplace_back(std::make_pair(totalWeight, entry._behavior));
    }
  }
  
  // Select the best game request - if there are more than one valid games to
  // request, choose one that is not the last game requested
  if(unlockedBehaviors.size() == 0){
    _lastGameRequested = nullptr;
  }else if(unlockedBehaviors.size() == 1){
    _lastGameRequested = (*unlockedBehaviors.begin()).second;
  }else{
    IBehavior* nextRequest = nullptr;
    int randomIndicator = 0;
    BOUNDED_WHILE(kMaxRetrys, ((nextRequest == nullptr) ||
                               (nextRequest == _lastGameRequested)))
    {
      // to achieve weighted randomness the random indicator maps to the first entry
      // in the vector that it's value is less than
      randomIndicator = robot.GetRNG().RandInt(totalWeight);
      for(const auto& entry: unlockedBehaviors){
        if(randomIndicator <  entry.first){
          nextRequest = entry.second;
          break;
        }
      }
    }
    
    _lastGameRequested = nextRequest;
  }
  return _lastGameRequested;
}

} // namespace Cozmo
} // namespace Anki
