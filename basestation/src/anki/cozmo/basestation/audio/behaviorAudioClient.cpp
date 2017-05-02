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
#include "anki/cozmo/basestation/audio/cozmoAudioController.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/components/publicStateBroadcaster.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"

#include "clad/externalInterface/messageGameToEngine.h"

namespace Anki {
namespace Cozmo {
namespace Audio {

using namespace AudioMetaData::SwitchState;
  
namespace {
  
const AudioMetaData::GameObjectType kMusicGameObject = AudioMetaData::GameObjectType::Default;
 
// Mapping from integral index to 'Gameplay_Round'
const std::array<AudioMetaData::SwitchState::Gameplay_Round, 11> kGameplayRoundMap = {{
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
}};
  
} // end anonymous namespace
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAudioClient::BehaviorAudioClient(Robot& robot)
: _robot(robot)
{
  // Get the appropriate spark music state from unity
  if(robot.HasExternalInterface()){
    auto handleSetSparkMusicState = [this](const AnkiEvent<ExternalInterface::MessageGameToEngine>& musicState){
      _sparkedMusicState = musicState.GetData().Get_SetSparkedMusicState().sparkedMusicState;
    };
    
    _eventHandles.push_back(robot.GetExternalInterface()->Subscribe(
                              ExternalInterface::MessageGameToEngineTag::SetSparkedMusicState,
                              handleSetSparkMusicState));
  }
  
  // Get state change info from the robot
  auto handleStateChangeWrapper = [this](const AnkiEvent<RobotPublicState>& stateEvent){
    HandleRobotPublicStateChange(stateEvent.GetData());
  };
  
  _eventHandles.push_back(robot.GetPublicStateBroadcaster().Subscribe(handleStateChangeWrapper));

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
  if (_round < kGameplayRoundMap.size()) {
    roundState = kGameplayRoundMap[_round];
  }
  else {
    DEV_ASSERT_MSG(false,
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

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioClient::UpdateAiGoalMusicState(const std::string& aiGoalName)
{
  // Ai Goals from behavior_config.json
  // 'hiking'          - when Cozmo is exploring its surroundings on his own
  // 'playWithHumans'  - when Cozmo requests games to play with the player
  // 'playAlone'       - when Cozmo does stuff in place on his own, showing off, playing with cubes, ..
  // 'socialize'       - when Cozmo wants to interact with faces and players, but without playing games
  // 'nothingToDo'     - fallback when Cozmo can't do anything else
  
  PRINT_CH_INFO(RobotAudioClient::kRobotAudioLogChannelName,
                "RobotAudioClient.SetFreeplayMusic",
                "AiGoalName '%s'", aiGoalName.c_str());
  static const std::unordered_map<std::string, Freeplay_Mood> freeplayStateMap
  {
    { "hiking",          AudioMetaData::SwitchState::Freeplay_Mood::Hiking },
    { "playWithHumans",  AudioMetaData::SwitchState::Freeplay_Mood::Neutral },
    { "playAlone",       AudioMetaData::SwitchState::Freeplay_Mood::Neutral },
    { "socialize",       AudioMetaData::SwitchState::Freeplay_Mood::Neutral },
    { "nothingToDo",     AudioMetaData::SwitchState::Freeplay_Mood::Bored }
  };
  
  // Search for freeplay goal state
  const auto it = freeplayStateMap.find(aiGoalName);
  if ( it != freeplayStateMap.end() ) {
    // Found State, update audio (and reset round)
    _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Gameplay_Round,
                                                  static_cast<const GenericSwitch>(Gameplay_Round::Round_00),
                                                  AudioMetaData::GameObjectType::Default);
    _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Freeplay_Mood,
                                                  static_cast<const GenericSwitch>(it->second),
                                                  AudioMetaData::GameObjectType::Default);
  }
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
void BehaviorAudioClient::DeactivateSparkedMusic()
{
  _sparkedMusicState = AudioMetaData::SwitchState::Sparked::Invalid;
  _isActive = false;
  _unlockId = UnlockId::Count;
  SetDefaultBehaviorRound();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioClient::HandleRobotPublicStateChange(const RobotPublicState& stateEvent)
{
  // Update sparked music if spark ID changed
  auto sparkID = stateEvent.currentSpark;
  if(_unlockId != sparkID){
    if(sparkID == UnlockId::Count){
      DeactivateSparkedMusic();
    }else if(_sparkedMusicState == AudioMetaData::SwitchState::Sparked::Invalid){
      PRINT_NAMED_ERROR("BehaviorAudioClient.HandleRobotPublicStateChange.InvalidMusicState",
                        "Attempted to activate sparked music state with invalid music state");
    }else{
      ActivateSparkedMusic(stateEvent.currentSpark,
                           AudioMetaData::GameState::Music::Spark,
                           _sparkedMusicState);
    }
  }
  
  const int behaviorRound = PublicStateBroadcaster::GetBehaviorRoundFromMessage(stateEvent);
  if(_round != behaviorRound){
    UpdateBehaviorRound(sparkID, behaviorRound);
  }
  
  // Handle AI goal transitions and Guard Dog behavior transitions
  const auto& currAiGoal = stateEvent.currentAiGoal;
  if (currAiGoal != _prevAiGoal) {
    UpdateAiGoalMusicState(currAiGoal);
    _prevAiGoal = currAiGoal;
  } else {
    // Update the Guard Dog music mood/round if appropriate
    const auto& currPublicStateStruct = stateEvent.userFacingBehaviorStageStruct;
    if (currAiGoal == "playAlone" &&
        currPublicStateStruct.behaviorStageTag == BehaviorStageTag::GuardDog)
    {
      // Change the freeplay mood to GuardDog if not already active
      if (!_guardDogActive) {
        _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Freeplay_Mood,
                                                      static_cast<const GenericSwitch>(Freeplay_Mood::Guard_Dog),
                                                      AudioMetaData::GameObjectType::Default);
        _guardDogActive = true;
      }
      
      // Update the GuardDog round if needed
      if(currPublicStateStruct.currentGuardDogStage != _prevGuardDogStage) {
        const auto round = Util::EnumToUnderlying(currPublicStateStruct.currentGuardDogStage);
        DEV_ASSERT_MSG(round < kGameplayRoundMap.size(),
                       "RobotAudioClient.HandleRobotPublicState.InvalidRound",
                       "round: %d", round);
        
        _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Gameplay_Round,
                                                      static_cast<const GenericSwitch>(kGameplayRoundMap[round]),
                                                      AudioMetaData::GameObjectType::Default);
        
        _prevGuardDogStage = currPublicStateStruct.currentGuardDogStage;
      }
    } else if (_guardDogActive) {
      // reset GuardDog stuff
      _guardDogActive = false;
      _prevGuardDogStage = GuardDogStage::Count;
      // Update the AI goal state to current AI goal
      //  (this will stop the GuardDog-specific music)
      UpdateAiGoalMusicState(currAiGoal);
    }
  }
}


} // Audio
} // Cozmo
} // Anki
