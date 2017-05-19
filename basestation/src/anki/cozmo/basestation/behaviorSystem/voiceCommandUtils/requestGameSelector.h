/**
 * File: RequestGameSelector
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

#ifndef __Cozmo_Basestation_BehaviorSystem_VoiceCommandUtils_RequestGameSelector_H__
#define __Cozmo_Basestation_BehaviorSystem_VoiceCommandUtils_RequestGameSelector_H__

#include "clad/types/unlockTypes.h"

#include <map>

namespace Anki {
namespace Cozmo {
  
// forward declarations
class IBehavior;
class Robot;
  
class RequestGameSelector{
public:
  RequestGameSelector(std::map<UnlockId, IBehavior*>&& gameRequests)
  : _gameRequests(std::move(gameRequests))
  , _lastGameRequested(nullptr)
  {};
  ~RequestGameSelector() {};
  IBehavior* GetNextRequestGameBehavior(Robot& robot, const IBehavior* currentRunningBehavior);
private:
  // The raw pointers to available request game behaviors
  std::map<UnlockId, IBehavior*> _gameRequests;
  // track the last game that was requested so that we don't request the same
  // game over and over again
  IBehavior* _lastGameRequested;
};
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_VoiceCommandUtils_RequestGameSelector_H__
