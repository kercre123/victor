/**
 * File: behaviorAudioClient.cpp
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


#include "anki/cozmo/basestation/audio/behaviorAudioClient.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
namespace Audio {

using namespace AudioMetaData::SwitchState;
  
static const AudioMetaData::GameObjectType kMusicGameObject = AudioMetaData::GameObjectType::Default;
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAudioClient::BehaviorAudioClient(Robot& robot)
: _robot(robot)
{
  _sparkedEnums = {
    Gameplay_Round::Round_00,
    Gameplay_Round::Round_01,
    Gameplay_Round::Round_02,
    Gameplay_Round::Round_03,
    Gameplay_Round::Round_04,
    Gameplay_Round::Round_05,
    Gameplay_Round::Round_06,
    Gameplay_Round::Round_07,
    Gameplay_Round::Round_08,
    Gameplay_Round::Round_09,
    Gameplay_Round::Round_10
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAudioClient::UpdateBehaviorRound(const UnlockId behaviorUnlockId, const int round)
{
  if (_unlockId != behaviorUnlockId) {
    return false;
  }
  
  _round = round;
  
  // Determine Round State audio enum
  Gameplay_Round roundState = Gameplay_Round::Invalid;
  if (_round < _sparkedEnums.size()) {
    roundState = _sparkedEnums[_round];
  }
  else {
    DEV_ASSERT_MSG(_round < _sparkedEnums.size(),
                   "BehaviorAudioClient.SetBehaviorStateLevel.InvalidRound",
                   "round: %d", round);
  }
  
  // Update audio engine
  if (_isActive && roundState != Gameplay_Round::Invalid) {
    _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Gameplay_Round,
                                                  static_cast<GenericSwitch>(roundState),
                                                  kMusicGameObject);
  }
  return true;
}

// Protected Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAudioClient::ActivateSparkedMusic(const UnlockId behaviorUnlockId,
                                               const AudioMetaData::GameState::Music musicState,
                                               const AudioMetaData::SwitchState::Sparked sparkedState,
                                               const int round)
{
  _unlockId = behaviorUnlockId;

  if (_unlockId == UnlockId::Invalid) {
    _isActive = false;
    return false;
  }

  _isActive = true;
  UpdateBehaviorRound(_unlockId, round);

  // Post Switch state for Sparked Behavior
  if (sparkedState != Sparked::Invalid) {
    _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Sparked,
                                                  static_cast<GenericSwitch>(sparkedState),
                                                  kMusicGameObject);
  }
  
  // Post Music state for Sparked Behavior
  if (musicState != AudioMetaData::GameState::Music::Invalid) {
    _robot.GetRobotAudioClient()->PostMusicState(static_cast<AudioMetaData::GameState::GenericState>(musicState));
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAudioClient::DeactivateSparkedMusic(const UnlockId behaviorUnlockId, const AudioMetaData::GameState::Music musicState)
{
  if (_unlockId != behaviorUnlockId && _unlockId != UnlockId::Invalid) {
    return false;
  }
  
  if (musicState != AudioMetaData::GameState::Music::Invalid) {
    _robot.GetRobotAudioClient()->PostMusicState(static_cast<AudioMetaData::GameState::GenericState>(musicState));
  }
  
  _isActive = false;
  _unlockId = UnlockId::Invalid;
  SetDefaultBehaviorRound();
  return true;
}


} // Audio
} // Cozmo
} // Anki
