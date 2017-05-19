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

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki{
namespace Cozmo{
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* RequestGameSelector::GetNextRequestGameBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  // Identify which behavior requests are currently unlocked
  std::vector<IBehavior*> unlockedBehaviors;
  for(const auto& entry: _gameRequests){
    if(robot.GetProgressionUnlockComponent().IsUnlocked(entry.first)){
      unlockedBehaviors.push_back(entry.second);
    }
  }
  
  // Select the best game request - if there are more than one valid games to
  // request, choose one that is not the last game requested
  if(unlockedBehaviors.size() == 0){
    _lastGameRequested = nullptr;
  }else if(unlockedBehaviors.size() == 1){
    _lastGameRequested = *unlockedBehaviors.begin();
  }else{
    IBehavior* nextRequest = nullptr;
    while((nextRequest == nullptr) ||
          (nextRequest == _lastGameRequested))
    {
      nextRequest = unlockedBehaviors[robot.GetRNG().RandInt(
                           static_cast<int>(unlockedBehaviors.size()))];
    }
    
    _lastGameRequested = nextRequest;
  }
  return _lastGameRequested;
}

} // namespace Cozmo
} // namespace Anki
