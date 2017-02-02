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

static const GameObjectType kMusicGameObject = GameObjectType::Default;
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAudioClient::BehaviorAudioClient(Robot& robot)
: _robot(robot)
{
  using namespace SwitchState;
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
  using namespace SwitchState;
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
                                                  static_cast<SwitchState::GenericSwitch>(roundState),
                                                  kMusicGameObject);
  }
  return true;
}

// Protected Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAudioClient::ActivateSparkedMusic(const UnlockId behaviorUnlockId,
                                               const GameState::Music musicState,
                                               const SwitchState::Sparked sparkedState,
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
  if (sparkedState != SwitchState::Sparked::Invalid) {
    _robot.GetRobotAudioClient()->PostSwitchState(SwitchState::SwitchGroupType::Sparked,
                                                  static_cast<SwitchState::GenericSwitch>(sparkedState),
                                                  kMusicGameObject);
  }
  
  // Post Music state for Sparked Behavior
  if (musicState != GameState::Music::Invalid) {
    _robot.GetRobotAudioClient()->PostMusicState(static_cast<GameState::GenericState>(musicState));
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAudioClient::DeactivateSparkedMusic(const UnlockId behaviorUnlockId, const GameState::Music musicState)
{
  if (_unlockId != behaviorUnlockId && _unlockId != UnlockId::Invalid) {
    return false;
  }
  
  if (musicState != GameState::Music::Invalid) {
    _robot.GetRobotAudioClient()->PostMusicState(static_cast<GameState::GenericState>(musicState));
  }
  
  _isActive = false;
  _unlockId = UnlockId::Invalid;
  SetDefaultBehaviorRound();
  return true;
}


} // Audio
} // Cozmo
} // Anki
