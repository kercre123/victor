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

#include "anki/cozmo/basestation/events/ankiEventMgr.h"

#include "clad/audio/audioStateTypes.h"
#include "clad/audio/audioSwitchTypes.h"
#include "clad/types/activityTypes.h"
#include "clad/types/behaviorTypes.h"
#include "clad/types/unlockTypes.h"


#define kBehaviorRound  0

namespace Anki {
namespace Cozmo {
class BehaviorManager;
class Robot;
struct RobotPublicState;

namespace Audio {


class BehaviorAudioClient {
  
public:
  BehaviorAudioClient(Robot& robot);

  // True if Client has been Activated
  bool IsActive() const { return _isActive; }
  
  // Get Current round
  int GetRound() const { return _round; }
  
  // Change music switch state for Ai Goals in freeplay
  void UpdateActivityMusicState(ActivityID activityID);
  
protected:
  // Activate to allow behavior to update audio engine
  // If GameState::Music::Invalid is passed in audio engine music state will not be updated
  // If SwitchState::Sparked::Invalid is passed in audio engine switch state will not be updated
  // Return false if behavior UnlockId is Invalid
  bool ActivateSparkedMusic(const UnlockId behaviorUnlockId,
                            const AudioMetaData::GameState::Music musicState,
                            const AudioMetaData::SwitchState::Sparked sparkedState,
                            const int round = kBehaviorRound);
  
  // This deactivates the BehaviorAudioClient and set new music state to freeplay
  void DeactivateSparkedMusic();
  
  // Update the behavior's current round
  // Return false behavior UnlockId does not match the UnlockId that was used to activate the sparked music
  bool UpdateBehaviorRound(const UnlockId behaviorUnlockId, const int round);
  
  void HandleRobotPublicStateChange(const RobotPublicState& stateEvent);
  
private:  
  Robot&    _robot;
  UnlockId  _unlockId = UnlockId::Count;
  AudioMetaData::SwitchState::Sparked _sparkedMusicState = AudioMetaData::SwitchState::Sparked::Invalid;
  bool      _isActive = false;
  int       _round    = kBehaviorRound;
  std::vector<::Signal::SmartHandle> _eventHandles;
  
  void SetDefaultBehaviorRound() { _round = kBehaviorRound; }
  
  // Keep track of the last AI goal name sent from the PublicStateBroadcaster so we
  //  can properly handle AI goal transitions.
  ActivityID _prevActivity;
  
  // True if the GuardDog behavior is active (uses its own music state)
  bool _guardDogActive = false;
  
  // Keep track of the last GuardDog behavior stage so we can set the
  //  proper audio round when transitioning between stages.
  GuardDogStage _prevGuardDogStage = GuardDogStage::Count;
  
};

} // Audio
} // Cozmo
} // Anki



#endif /* __Basestation_Audio_BehaviorAudioClient_H__ */
