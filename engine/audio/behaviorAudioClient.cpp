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


#include "engine/audio/behaviorAudioClient.h"
#include "engine/audio/cozmoAudioController.h"
#include "engine/audio/robotAudioClient.h"
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/workoutComponent.h"
#include "engine/behaviorSystem/activities/activities/activityFreeplay.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

#include "clad/audio/audioEventTypes.h"
#include "clad/externalInterface/messageGameToEngine.h"

#include "util/helpers/fullEnumToValueArrayChecker.h"

namespace Anki {
namespace Cozmo {
namespace Audio {

using namespace AudioMetaData::SwitchState;
  
using StageTagMultiMap = std::multimap<BehaviorStageTag, ActivityID>;
using Util::FullEnumToValueArrayChecker::IsSequentialArray; // import IsSequentialArray to this namespace

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

// Mapping from ActivityId to FreeplayMood for music states
// 'buildPyramid'    - when Cozmo is building a pyramid (manages its own music states)
// 'feeding'         - when Cozmo is being fed cubes by the user to refil his energy need
// 'hiking'          - when Cozmo is exploring its surroundings on his own
// 'needsSevereLowEnergy' - when Cozmo's needstate is severely low energy
// 'needsSevereLowRepair' - when Cozmo's needstate is severely low repair
// 'needsSevereLowPlayGetIn' - when Cozmo is transitioning into severely low play state
// 'playWithHumans'  - when Cozmo requests games to play with the player
// 'playAlone'       - when Cozmo does stuff in place on his own, showing off, playing with cubes, ..
// 'putDownDispatch' - when Cozmo is looking for stuff after returning to on treads
// 'singing'         - when Cozmo is singing (manages its own music states)
// 'socialize'       - when Cozmo wants to interact with faces and players, but without playing games
// 'nothingToDo'     - fallback when Cozmo can't do anything else
const std::unordered_map<ActivityID, Freeplay_Mood> freeplayStateMap
{
  { ActivityID::BuildPyramid,            AudioMetaData::SwitchState::Freeplay_Mood::Invalid },
  { ActivityID::Feeding,                 AudioMetaData::SwitchState::Freeplay_Mood::Nurture_Feeding },
  { ActivityID::Hiking,                  AudioMetaData::SwitchState::Freeplay_Mood::Hiking },
  { ActivityID::NeedsSevereLowEnergy,    AudioMetaData::SwitchState::Freeplay_Mood::Invalid },
  { ActivityID::NeedsSevereLowRepair,    AudioMetaData::SwitchState::Freeplay_Mood::Invalid },
  { ActivityID::NeedsSevereLowPlayGetIn, AudioMetaData::SwitchState::Freeplay_Mood::Invalid },
  { ActivityID::PlayWithHumans,          AudioMetaData::SwitchState::Freeplay_Mood::Neutral },
  { ActivityID::PlayAlone,               AudioMetaData::SwitchState::Freeplay_Mood::Neutral },
  { ActivityID::PutDownDispatch,         AudioMetaData::SwitchState::Freeplay_Mood::Neutral },
  { ActivityID::Singing,                 AudioMetaData::SwitchState::Freeplay_Mood::Invalid },
  { ActivityID::Socialize,               AudioMetaData::SwitchState::Freeplay_Mood::Neutral },
  { ActivityID::NothingToDo,             AudioMetaData::SwitchState::Freeplay_Mood::Bored }
};
  
const StageTagMultiMap activityAllowedStagesMap
{
  { BehaviorStageTag::Feeding, ActivityID::Feeding},
  { BehaviorStageTag::GuardDog, ActivityID::PlayAlone},
  { BehaviorStageTag::PyramidConstruction, ActivityID::BuildPyramid},
  { BehaviorStageTag::PyramidConstruction, ActivityID::SparksBuildPyramid},
  { BehaviorStageTag::Workout, ActivityID::PlayAlone},
  { BehaviorStageTag::Workout, ActivityID::SparksWorkout},
};
  
} // end anonymous namespace
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAudioClient::BehaviorAudioClient(Robot& robot)
: _robot(robot)
, _prevActivity(ActivityID::Invalid)
, _activeBehaviorStage(BehaviorStageTag::Count)
, _prevGuardDogStage(GuardDogStage::Count)
, _prevFeedingStage(FeedingStage::MildEnergy)
{
  // Get the appropriate spark music state from unity
  if(robot.HasExternalInterface()){
    auto handleSetSparkMusicState = [this](const AnkiEvent<ExternalInterface::MessageGameToEngine>& musicState){
      _sparkedMusicState = musicState.GetData().Get_SetSparkedMusicState().sparkedMusicState;
      HandleSparkUpdates(_lastUnlockIDReceived, _round);
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
void BehaviorAudioClient::Init()
{
  if(ANKI_DEV_CHEATS)
  {
    const auto nonSparkFreeplayActivities = _robot.GetBehaviorManager().GetNonSparkFreeplayActivities();
    for(const auto& activity : nonSparkFreeplayActivities)
    {
      const ActivityID activityID = activity->GetID();

      DEV_ASSERT_MSG(freeplayStateMap.find(activityID) != freeplayStateMap.end(),
                     "BehaviorAudioClient.Init.MissingFreeplayActivity",
                     "Freeplay activity %s does not exist in freeplayStateMap",
                     EnumToString(activityID));
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAudioClient::UpdateBehaviorRound(const UnlockId behaviorUnlockId, const int round)
{
  if (_activeSparkMusicID != behaviorUnlockId) {
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
void BehaviorAudioClient::UpdateActivityMusicState(ActivityID activityID)
{
  PRINT_CH_INFO(RobotAudioClient::kRobotAudioLogChannelName,
                "RobotAudioClient.SetFreeplayMusic",
                "ActivityName '%s'", ActivityIDToString(activityID));
  
  // Search for freeplay goal state
  const auto it = freeplayStateMap.find(activityID);
  if ( it != freeplayStateMap.end() ) {
    // Found State, update audio (and reset round)
    _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Gameplay_Round,
                                                  static_cast<const GenericSwitch>(Gameplay_Round::Round_00),
                                                  AudioMetaData::GameObjectType::Default);
    _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Freeplay_Mood,
                                                  static_cast<const GenericSwitch>(it->second),
                                                  AudioMetaData::GameObjectType::Default);
  }
  else
  {
    PRINT_CH_INFO(RobotAudioClient::kRobotAudioLogChannelName,
                  "RobotAudioClient.UpdateActivityMusicState.NoFreeplayMusic",
                  "No freeplay music for ActivityName '%s'", ActivityIDToString(activityID));
  }
}
  
  
// Protected Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAudioClient::ActivateSparkedMusic(const UnlockId behaviorUnlockId,
                                               const AudioMetaData::GameState::Music musicState,
                                               const AudioMetaData::SwitchState::Sparked sparkedState,
                                               const int round)
{
  _activeSparkMusicID = behaviorUnlockId;

  if (_activeSparkMusicID == UnlockId::Invalid) {
    _isActive = false;
    return false;
  }

  _isActive = true;
  UpdateBehaviorRound(_activeSparkMusicID, round);

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
  _activeSparkMusicID = UnlockId::Count;
  SetDefaultBehaviorRound();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioClient::HandleRobotPublicStateChange(const RobotPublicState& stateEvent)
{
  // Check for changes in "world events" that audio is interested in
  HandleWorldEventUpdates(stateEvent);
  
  // Update sparked music if spark ID changed
  HandleSparkUpdates(stateEvent.currentSpark,
                     PublicStateBroadcaster::GetBehaviorRoundFromMessage(stateEvent));
  
  // Inform the audio engine of changes in needs levels
  HandleNeedsUpdates(stateEvent.needsLevels);
  
  // Check if we need to dim the music for the current activity
  HandleDimMusicForActivity(stateEvent);
  
  // Handle AI Activity transitions and Guard Dog behavior transitions
  const auto& currActivity = stateEvent.currentActivity;
  if (currActivity != _prevActivity) {
    UpdateActivityMusicState(currActivity);
    _prevActivity = currActivity;
  } else {
    
    const BehaviorStageStruct& currPublicStateStruct = stateEvent.userFacingBehaviorStageStruct;
    HandleGuardDogUpdates(currPublicStateStruct);
    //HandleDancingUpdates(currPublicStateStruct);
    HandleFeedingUpdates(currPublicStateStruct);
  }
  
  // Ensure that if a stage has been set we're tracking it properly
  BehaviorStageTag currentStageTag = stateEvent.userFacingBehaviorStageStruct.behaviorStageTag;
  if(currentStageTag != GetActiveBehaviorStage()){
    SetActiveBehaviorStage(currentStageTag);
  }
  
  _lastUnlockIDReceived = stateEvent.currentSpark;
  // Ensure behavior stages have been cleared/re-set appropriately if the activity
  // has changed
  if(ANKI_DEV_CHEATS){
    if(GetActiveBehaviorStage() != BehaviorStageTag::Count){
      bool activityIsValid = false;
      for(const auto& entry: activityAllowedStagesMap){
        if(entry.first == GetActiveBehaviorStage()){
          if(entry.second == currActivity){
            activityIsValid = true;
            break;
          }
        }
      }
      DEV_ASSERT_MSG(activityIsValid,
                     "BehaviorAudioClient.HandleRobotPublicStateChange.IncorrectBehaviorStage",
                     "Behavior stage is %s but activity is %s",
                     BehaviorStageTagToString(GetActiveBehaviorStage()),
                     ActivityIDToString(currActivity));
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioClient::HandleWorldEventUpdates(const RobotPublicState& stateEvent)
{
  // COZMO-14148 - Firing world events during feeding causes improper
  // audio state transitions, so short circuit this function here
  // to avoid posting events
  if(stateEvent.currentActivity == ActivityID::Feeding){
    return;
  }
  
  using AE = AudioMetaData::GameEvent::GenericEvent;
  
  if(_isCubeInLift != stateEvent.isCubeInLift){
    _isCubeInLift = stateEvent.isCubeInLift;
    
    if(_isCubeInLift){
      _robot.GetRobotAudioClient()->PostEvent(AE::Play__Cue_World_Event__Lift_Cube, kMusicGameObject);
    }else{
      _robot.GetRobotAudioClient()->PostEvent(AE::Stop__Cue_World_Event__Lift_Cube, kMusicGameObject);
    }
  }
  
  if(_isRequestingGame != stateEvent.isRequestingGame){
    _isRequestingGame = stateEvent.isRequestingGame;

    if(_isRequestingGame){
      _robot.GetRobotAudioClient()->PostEvent(AE::Play__Cue_World_Event__Request_Game, kMusicGameObject);
    }else{
      _robot.GetRobotAudioClient()->PostEvent(AE::Stop__Cue_World_Event__Request_Game, kMusicGameObject);
    }
  }
  
  
  if(_stackExists != (stateEvent.tallestStackHeight > 1)){
    _stackExists = (stateEvent.tallestStackHeight > 1);
    
    if(_stackExists){
      _robot.GetRobotAudioClient()->PostEvent(AE::Play__Cue_World_Event__Cubes_Stacked, kMusicGameObject);
    }else{
      _robot.GetRobotAudioClient()->PostEvent(AE::Stop__Cue_World_Event__Cubes_Stacked, kMusicGameObject);
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioClient::HandleSparkUpdates(const UnlockId& sparkID, const int behaviorRound)
{
  if(_activeSparkMusicID != sparkID){
    if(sparkID == UnlockId::Count){
      DeactivateSparkedMusic();
    }else if(_sparkedMusicState == AudioMetaData::SwitchState::Sparked::Invalid){
      PRINT_NAMED_INFO("BehaviorAudioClient.HandleRobotPublicStateChange.InvalidMusicState",
                       "Attempted to activate sparked music state with invalid music state");
    }else{
      // Special handling for Workout (blergghhhhbleghgh... ahemm, excuse me)
      int startingRound = 0;
      if (sparkID == UnlockId::Workout) {
        // If 80's music is to be played, we need to start at music stage 'EightiesWorkoutPrep' (round 1).
        //   If not, start at stage 'NormalWorkout' (round 0)
        auto& workoutComponent = _robot.GetAIComponent().GetWorkoutComponent();
        const bool playEightiesMusic = workoutComponent.ShouldPlayEightiesMusic();
        startingRound = Util::EnumToUnderlying(playEightiesMusic ?
                                               WorkoutStage::EightiesWorkoutPrep :
                                               WorkoutStage::NormalWorkout);
        // Need to tell the PublicStateBroadcaster so that it stays in sync with the proper music round
        _robot.GetPublicStateBroadcaster().UpdateBroadcastBehaviorStage(BehaviorStageTag::Workout, startingRound);
      }
      
      ActivateSparkedMusic(sparkID,
                           AudioMetaData::GameState::Music::Spark,
                           _sparkedMusicState,
                           startingRound);
    }
  } else {
    // Same spark - just update the behavior round
    if(_round != behaviorRound){
      UpdateBehaviorRound(sparkID, behaviorRound);
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioClient::HandleGuardDogUpdates(const BehaviorStageStruct& currPublicStateStruct)
{
  // Update the Guard Dog music mood/round if appropriate
  if (currPublicStateStruct.behaviorStageTag == BehaviorStageTag::GuardDog) {
    // Change the freeplay mood to GuardDog if not already active
    if (GetActiveBehaviorStage() != BehaviorStageTag::GuardDog) {
      _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Freeplay_Mood,
                                                    static_cast<const GenericSwitch>(Freeplay_Mood::Guard_Dog),
                                                    AudioMetaData::GameObjectType::Default);
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
  } else if (GetActiveBehaviorStage() != BehaviorStageTag::GuardDog) {
    // reset GuardDog stuff
    _prevGuardDogStage = GuardDogStage::Count;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioClient::HandleDancingUpdates(const BehaviorStageStruct& currPublicStateStruct)
{
  // If this is dancing then post dancing switch
  /**if((currPublicStateStruct.behaviorStageTag == BehaviorStageTag::Dance) &&
     (GetActiveBehaviorStage() != BehaviorStageTag::Dance))
  {
    _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Freeplay_Mood,
                                                  static_cast<GenericSwitch>(Freeplay_Mood::Dancing),
                                                  AudioMetaData::GameObjectType::Default);
  }**/
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioClient::HandleFeedingUpdates(const BehaviorStageStruct& currPublicStateStruct)
{
  const bool stageOrRoundChanged = (GetActiveBehaviorStage() != BehaviorStageTag::Feeding) ||
                                     (_prevFeedingStage != currPublicStateStruct.currentFeedingStage);
  
  if((currPublicStateStruct.behaviorStageTag == BehaviorStageTag::Feeding) &&
     stageOrRoundChanged){
    _prevFeedingStage =  currPublicStateStruct.currentFeedingStage;
    _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Gameplay_Round,
                                                  static_cast<const GenericSwitch>(kGameplayRoundMap[_round]),
                                                  AudioMetaData::GameObjectType::Default);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioClient::HandleNeedsUpdates(const NeedsLevels& needsLevel)
{
  using AGO = AudioMetaData::GameObjectType;
  if(!FLT_NEAR(needsLevel.energy, _needsLevel.energy)){
    _needsLevel.energy = needsLevel.energy;
    _robot.GetRobotAudioClient()->PostParameter(AudioMetaData::GameParameter::ParameterType::Nurture_Energy,
                                                _needsLevel.energy,
                                                AGO::Invalid);
  }
  
  if(!FLT_NEAR(needsLevel.repair, _needsLevel.repair)){
    _needsLevel.repair = needsLevel.repair;
    _robot.GetRobotAudioClient()->PostParameter(AudioMetaData::GameParameter::ParameterType::Nurture_Repair,
                                                _needsLevel.repair,
                                                AGO::Invalid);
  }
  
  if(!FLT_NEAR(needsLevel.play, _needsLevel.play)){
    _needsLevel.play = needsLevel.play;
    _robot.GetRobotAudioClient()->PostParameter(AudioMetaData::GameParameter::ParameterType::Nurture_Play,
                                                _needsLevel.play,
                                                AGO::Invalid);
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStageTag BehaviorAudioClient::GetActiveBehaviorStage()
{
  return _activeBehaviorStage;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioClient::SetActiveBehaviorStage(BehaviorStageTag stageTag)
{
  DEV_ASSERT_MSG((stageTag == BehaviorStageTag::Count) ||
                 (_activeBehaviorStage == BehaviorStageTag::Count),
                 "BehaviorAudioClient.SetActiveBehaviorStage.StageAlreadySet",
                 "Trying to set new stage tag %s, but stage tag is already set as %s",
                 BehaviorStageTagToString(stageTag),
                 BehaviorStageTagToString(_activeBehaviorStage));
  _activeBehaviorStage = stageTag;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioClient::HandleDimMusicForActivity(const RobotPublicState& stateEvent)
{
  using AE = AudioMetaData::GameEvent::GenericEvent;
  if(_prevActivity != stateEvent.currentActivity)
  {
    if(stateEvent.currentActivity == ActivityID::Singing)
    {
      _robot.GetRobotAudioClient()->PostEvent(static_cast<AE>(AudioMetaData::GameEvent::App::Music_Dim_On),
                                              kMusicGameObject);
    }
    else if(_prevActivity == ActivityID::Singing)
    {
      _robot.GetRobotAudioClient()->PostEvent(static_cast<AE>(AudioMetaData::GameEvent::App::Music_Dim_Off),
                                              kMusicGameObject);
    }
  }
}

  
  
} // Audio
} // Cozmo
} // Anki
