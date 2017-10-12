/**
 * File: DoATrickSelector.cpp
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

#include "engine/aiComponent/doATrickSelector.h"

#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/robot.h"

#include "util/helpers/boundedWhile.h"

#include <set>

namespace Anki{
namespace Cozmo{
  
namespace{
const char * kUnlockIDConfigKey   = "unlockID";
const char * kWeightConfigKey     = "weight";
const int kMaxRetrys = 1000;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DoATrickSelector::DoATrickSelector(const Json::Value& trickWeightsConfig)
: _lastTrickPerformed(UnlockId::Invalid)
{
  for(const auto& entry: trickWeightsConfig){
    UnlockId unlockID = UnlockIdFromString(
                           JsonTools::ParseString(entry,
                                                  kUnlockIDConfigKey,
                                                  "RequestGameSelector.UnlockID"));
    int weight = JsonTools::ParseUint8(entry,
                                       kWeightConfigKey,
                                      "RequestGameSelector.Weight");
    _weightToUnlockMap.emplace_back(std::make_pair(weight, unlockID));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DoATrickSelector::RequestATrick(BehaviorExternalInterface& behaviorExternalInterface)
{
  int cumulativeWeight = 0;
  std::vector<std::pair<int, UnlockId>> unlockedTricks;

  if(behaviorExternalInterface.HasProgressionUnlockComponent()){
    auto& progressionUnlockComp = behaviorExternalInterface.GetProgressionUnlockComponent();
    for(const auto& entry: _weightToUnlockMap){
      if(progressionUnlockComp.IsUnlocked(entry.second)){
        cumulativeWeight += entry.first;
        unlockedTricks.emplace_back(std::make_pair(cumulativeWeight, entry.second));
      }
    }
  }
  
  if(unlockedTricks.size() == 0){
    _lastTrickPerformed = UnlockId::Invalid;
  }else if(unlockedTricks.size() == 1){
    _lastTrickPerformed = (*unlockedTricks.begin()).second;
  }else{
    UnlockId nextTrick = UnlockId::Invalid;
    int randomIndicator = 0;
    BOUNDED_WHILE(kMaxRetrys, ((nextTrick == UnlockId::Invalid) ||
                               (nextTrick == _lastTrickPerformed)))
    {
      // to achieve weighted randomness the random indicator maps to the first entry
      // in the vector that it's value is less than
      randomIndicator = behaviorExternalInterface.GetRNG().RandInt(cumulativeWeight);
      for(const auto& entry: unlockedTricks){
        if(randomIndicator <  entry.first){
          nextTrick = entry.second;
          break;
        }
      }
    }
    
    _lastTrickPerformed = nextTrick;
  }
  
  
  if(_lastTrickPerformed != UnlockId::Invalid){
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    const bool isSoftSpark = false;
    robot.GetBehaviorManager().SetRequestedSpark(_lastTrickPerformed, isSoftSpark);
  }
}

} // namespace Cozmo
} // namespace Anki
