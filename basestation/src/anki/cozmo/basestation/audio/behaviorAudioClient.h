/**
 * File: behaviorAudioClient.h
 *
 * Author: Jordan Rivas
 * Created: 11/02/2016
 *
 * Description: This Client provides behavior audio needs for updating Sparked Behavior music state and round. When
 *              using first call ActivateSparkedMusic() to activate audio client and when behavior is completed call
 *              DeactivateSparkedMusic().
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Basestation_Audio_BehaviorAudioClient_H__
#define __Basestation_Audio_BehaviorAudioClient_H__

#include "clad/audio/audioStateTypes.h"
#include "clad/audio/audioSwitchTypes.h"
#include "clad/types/unlockTypes.h"
#include <vector>


#define kBehaviorRound  0

namespace Anki {
namespace Cozmo {
class BehaviorManager;
class Robot;
namespace Audio {


class BehaviorAudioClient {
  
public:
  
  // Update the behavior's current round
  // Return false behavior UnlockId does not match the UnlockId that was used to activate the sparked music
  bool UpdateBehaviorRound(const UnlockId behaviorUnlockId, const int round);
  
  // True if Client has been Activated
  bool IsActive() const { return _isActive; }
  
  // Get Current round
  int GetRound() const { return _round; }
  
  
protected:
  
  friend class Anki::Cozmo::BehaviorManager;
  
  // Default Constructor
  BehaviorAudioClient(Robot& robot);
  
  // Activate to allow behavior to update audio engine
  // If GameState::Music::Invalid is passed in audio engine music state will not be updated
  // If SwitchState::Sparked::Invalid is passed in audio engine switch state will not be updated
  // Return false if behavior UnlockId is Invalid
  bool ActivateSparkedMusic(const UnlockId behaviorUnlockId,
                            const GameState::Music musicState,
                            const SwitchState::Sparked sparkedState,
                            const int round = kBehaviorRound);
  
  // This deactivates the BehaviorAudioClient and set new music state
  // If GameState::Music::Invalid is passed in audio engine music state will not be updated
  // Return false behavior UnlockId does not match the UnlockId that was used to activate the sparked music or Invalid
  bool DeactivateSparkedMusic(const UnlockId behaviorUnlockId, const GameState::Music musicState);
  
  
private:
  
  Robot&    _robot;
  UnlockId  _unlockId = UnlockId::Invalid;
  bool      _isActive = false;
  int       _round    = kBehaviorRound;
  
  std::vector<SwitchState::Gameplay_Round> _sparkedEnums;
  
  void SetDefaultBehaviorRound() { _round = kBehaviorRound; }
  
};

} // Audio
} // Cozmo
} // Anki



#endif /* __Basestation_Audio_BehaviorAudioClient_H__ */
