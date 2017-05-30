/**
 * File: DoATrickSelector
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

#ifndef __Cozmo_Basestation_BehaviorSystem_VoiceCommandUtils_DoATrickSelector_H__
#define __Cozmo_Basestation_BehaviorSystem_VoiceCommandUtils_DoATrickSelector_H__

#include "clad/types/unlockTypes.h"

namespace Anki {
namespace Cozmo {
  
// forward declarations
class IBehavior;
class Robot;
  
class DoATrickSelector{
public:
  DoATrickSelector(Robot& robot);

  void RequestATrick(Robot& robot);
private:
  std::vector<std::pair<int, UnlockId>> _weightToUnlockMap;
  // track the last trick that was performed
  UnlockId _lastTrickPerformed;
};
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_VoiceCommandUtils_DoATrickSelector_H__
