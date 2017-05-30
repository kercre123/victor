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

#include "anki/cozmo/basestation/behaviorSystem/voiceCommandUtils/doATrickSelector.h"

#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robot.h"
#include <set>
namespace Anki{
namespace Cozmo{
  
namespace{
const char * kUnlockIDConfigKey   = "unlockID";
const char * kWeightConfigKey     = "weight";
const int kMaxRetrys = 1000;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DoATrickSelector::DoATrickSelector(Robot& robot)
: _lastTrickPerformed(UnlockId::Invalid)
{
  const Json::Value& doATrickConfig = robot.GetContext()->GetDataLoader()->GetDoATrickWeightsConfig();
  for(const auto& entry: doATrickConfig){
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
void DoATrickSelector::RequestATrick(Robot& robot)
{
  int cumulativeWeight = 0;
  std::vector<std::pair<int, UnlockId>> unlockedTricks;
  for(const auto& entry: _weightToUnlockMap){
    if(robot.GetProgressionUnlockComponent().IsUnlocked(entry.second)){
      cumulativeWeight += entry.first;
      unlockedTricks.emplace_back(std::make_pair(cumulativeWeight, entry.second));
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
      randomIndicator = robot.GetRNG().RandInt(cumulativeWeight);
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
    const bool isSoftSpark = false;
    robot.GetBehaviorManager().SetRequestedSpark(_lastTrickPerformed, isSoftSpark);
  }
}

} // namespace Cozmo
} // namespace Anki
