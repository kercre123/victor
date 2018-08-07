/**
 * File: BehaviorAudioComponent.cpp
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



#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorAudioComponent.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

#include "clad/audio/audioEventTypes.h"
#include "clad/externalInterface/messageGameToEngine.h"

#include "util/helpers/fullEnumToValueArrayChecker.h"

namespace Anki {
namespace Vector {
namespace Audio {

using namespace AudioMetaData::SwitchState;
  
using StageTagMultiMap = std::multimap<BehaviorStageTag, BehaviorID>;
using Util::FullEnumToValueArrayChecker::IsSequentialArray; // import IsSequentialArray to this namespace

namespace {
  
//const AudioMetaData::GameObjectType kMusicGameObject = AudioMetaData::GameObjectType::Default;

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
/**{const std::unordered_map<BehaviorID, Freeplay_Mood> freeplayStateMap
{
   BehaviorID::Activity_BuildPyramid,            AudioMetaData::SwitchState::Freeplay_Mood::Invalid },
  { BehaviorID::Activity_Feeding,                 AudioMetaData::SwitchState::Freeplay_Mood::Nurture_Feeding },
  { BehaviorID::Activity_Hiking,                  AudioMetaData::SwitchState::Freeplay_Mood::Hiking },
  { BehaviorID::Activity_NeedsSevereLowEnergy,    AudioMetaData::SwitchState::Freeplay_Mood::Invalid },
  { BehaviorID::Activity_NeedsSevereLowRepair,    AudioMetaData::SwitchState::Freeplay_Mood::Invalid },
  { BehaviorID::Activity_NeedsSevereLowPlayGetIn, AudioMetaData::SwitchState::Freeplay_Mood::Invalid },
  { BehaviorID::Activity_PlayWithHumans,          AudioMetaData::SwitchState::Freeplay_Mood::Neutral },
  { BehaviorID::Activity_PlayAlone,               AudioMetaData::SwitchState::Freeplay_Mood::Neutral },
  { BehaviorID::Activity_PutDownDispatch,         AudioMetaData::SwitchState::Freeplay_Mood::Neutral },
  { BehaviorID::Activity_Singing,                 AudioMetaData::SwitchState::Freeplay_Mood::Invalid },
  { BehaviorID::Activity_Socialize,               AudioMetaData::SwitchState::Freeplay_Mood::Neutral },
  { BehaviorID::Activity_NothingToDo,             AudioMetaData::SwitchState::Freeplay_Mood::Bored }
};**/
  
/**const StageTagMultiMap activityAllowedStagesMap
{
  { BehaviorStageTag::Feeding, BehaviorID::Activity_Feeding},
  { BehaviorStageTag::GuardDog, BehaviorID::Activity_PlayAlone},
  { BehaviorStageTag::PyramidConstruction, BehaviorID::Activity_BuildPyramid},
  { BehaviorStageTag::PyramidConstruction, BehaviorID::Activity_SparksBuildPyramid}
};**/
  
} // end anonymous namespace
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAudioComponent::BehaviorAudioComponent(Audio::EngineRobotAudioClient* robotAudioClient)
//: _robotAudioClient(robotAudioClient)
: IDependencyManagedComponent(this, BCComponentID::BehaviorAudioComponent)
, _activeBehaviorStage(BehaviorStageTag::Count)
//, _prevGuardDogStage(GuardDogStage::Count)
//, _prevFeedingStage(FeedingStage::MildEnergy)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioComponent::InitDependent(Vector::Robot* robot, const BCCompMap& dependentComps)
{
  auto& bei = dependentComps.GetComponent<BehaviorExternalInterface>();
  Init(bei);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioComponent::Init(BehaviorExternalInterface& behaviorExternalInterface)
{
  // Get the appropriate spark music state from unity
  /**auto externalInterface = behaviorExternalInterface.GetRobotExternalInterface().lock();
  if(externalInterface != nullptr){
    auto handleSetSparkMusicState = [this, &behaviorExternalInterface](const AnkiEvent<ExternalInterface::MessageGameToEngine>& musicState){
      _sparkedMusicState = musicState.GetData().Get_SetSparkedMusicState().sparkedMusicState;
      HandleSparkUpdates(behaviorExternalInterface, _lastUnlockIDReceived, _round);
    };
    
    _eventHandles.push_back(externalInterface->Subscribe(
                                                         ExternalInterface::MessageGameToEngineTag::SetSparkedMusicState,
                                                         handleSetSparkMusicState));
  }**/
  
  // Get state change info from the robot
  auto handleStateChangeWrapper = [this, &behaviorExternalInterface](const AnkiEvent<RobotPublicState>& stateEvent){
    HandleRobotPublicStateChange(behaviorExternalInterface,stateEvent.GetData());
  };
  
  if(behaviorExternalInterface.HasPublicStateBroadcaster()){
    auto& publicStateBroadcaster = behaviorExternalInterface.GetRobotPublicStateBroadcaster();
    _eventHandles.push_back(publicStateBroadcaster.Subscribe(handleStateChangeWrapper));
  }
  
  if(ANKI_DEV_CHEATS)
  {
    /** 
    const auto nonSparkFreeplayActivities = robot.GetBehaviorManager().GetNonSparkFreeplayActivities();
    for(const auto& activity : nonSparkFreeplayActivities)
    {
      const BehaviorID activityID = activity->GetID();

      DEV_ASSERT_MSG(freeplayStateMap.find(activityID) != freeplayStateMap.end(),
                     "BehaviorAudioComponent.Init.MissingFreeplayActivity",
                     "Freeplay activity %s does not exist in freeplayStateMap",
                     EnumToString(activityID));
    }**/
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioComponent::UpdateActivityMusicState(BehaviorID activityID)
{
//  PRINT_CH_INFO(RobotAudioClient::kRobotAudioLogChannelName,
//                "RobotAudioClient.SetFreeplayMusic",
//                "ActivityName '%s'", ActivityIDToString(activityID));
//  
//  // Search for freeplay goal state
//  const auto it = freeplayStateMap.find(activityID);
//  if ( it != freeplayStateMap.end() ) {
//    // Found State, update audio (and reset round)
//    _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Gameplay_Round,
//                                                  static_cast<const GenericSwitch>(Gameplay_Round::Round_00),
//                                                  AudioMetaData::GameObjectType::Default);
//    _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Freeplay_Mood,
//                                                  static_cast<const GenericSwitch>(it->second),
//                                                  AudioMetaData::GameObjectType::Default);
//  }
//  else
//  {
//    PRINT_CH_INFO(RobotAudioClient::kRobotAudioLogChannelName,
//                  "RobotAudioClient.UpdateActivityMusicState.NoFreeplayMusic",
//                  "No freeplay music for ActivityName '%s'", ActivityIDToString(activityID));
//  }
}
  
  
// Protected Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioComponent::HandleRobotPublicStateChange(BehaviorExternalInterface& behaviorExternalInterface,
                                                       const RobotPublicState& stateEvent)
{
  // Check for changes in "world events" that audio is interested in
  HandleWorldEventUpdates(stateEvent);
  
  // Inform the audio engine of changes in needs levels
  HandleNeedsUpdates(stateEvent.needsLevels);
  
  // Check if we need to dim the music for the current activity
  HandleDimMusicForActivity(stateEvent);
  
  // Handle AI Activity transitions and Guard Dog behavior transitions
  /**const auto& currActivity = stateEvent.currentActivity;
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
                     "BehaviorAudioComponent.HandleRobotPublicStateChange.IncorrectBehaviorStage",
                     "Behavior stage is %s but activity is %s",
                     BehaviorStageTagToString(GetActiveBehaviorStage()),
                     ActivityIDToString(currActivity));
    }
  }**/
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioComponent::HandleWorldEventUpdates(const RobotPublicState& stateEvent)
{
// // COZMO-14148 - Firing world events during feeding causes improper
// // audio state transitions, so short circuit this function here
// // to avoid posting events
// if(stateEvent.currentActivity == ActivityID::Feeding){
//   return;
// }
//
//  using AE = AudioMetaData::GameEvent::GenericEvent;
//  
//  if(_isCubeInLift != stateEvent.isCubeInLift){
//    _isCubeInLift = stateEvent.isCubeInLift;
//    
//    if(_isCubeInLift){
//      _robot.GetRobotAudioClient()->PostEvent(AE::Play__Cue_World_Event__Lift_Cube, kMusicGameObject);
//    }else{
//      _robot.GetRobotAudioClient()->PostEvent(AE::Stop__Cue_World_Event__Lift_Cube, kMusicGameObject);
//    }
//  }
//  
//  if(_isRequestingGame != stateEvent.isRequestingGame){
//    _isRequestingGame = stateEvent.isRequestingGame;
//
//    if(_isRequestingGame){
//      _robot.GetRobotAudioClient()->PostEvent(AE::Play__Cue_World_Event__Request_Game, kMusicGameObject);
//    }else{
//      _robot.GetRobotAudioClient()->PostEvent(AE::Stop__Cue_World_Event__Request_Game, kMusicGameObject);
//    }
//  }
//  
//  
//  if(_stackExists != (stateEvent.tallestStackHeight > 1)){
//    _stackExists = (stateEvent.tallestStackHeight > 1);
//    
//    if(_stackExists){
//      _robot.GetRobotAudioClient()->PostEvent(AE::Play__Cue_World_Event__Cubes_Stacked, kMusicGameObject);
//    }else{
//      _robot.GetRobotAudioClient()->PostEvent(AE::Stop__Cue_World_Event__Cubes_Stacked, kMusicGameObject);
//    }
//  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioComponent::HandleGuardDogUpdates(const BehaviorStageStruct& currPublicStateStruct)
{
//  // Update the Guard Dog music mood/round if appropriate
//  if (currPublicStateStruct.behaviorStageTag == BehaviorStageTag::GuardDog) {
//    // Change the freeplay mood to GuardDog if not already active
//    if (GetActiveBehaviorStage() != BehaviorStageTag::GuardDog) {
//      _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Freeplay_Mood,
//                                                    static_cast<const GenericSwitch>(Freeplay_Mood::Guard_Dog),
//                                                    AudioMetaData::GameObjectType::Default);
//    }
//    
//    // Update the GuardDog round if needed
//    if(currPublicStateStruct.currentGuardDogStage != _prevGuardDogStage) {
//      const auto round = Util::EnumToUnderlying(currPublicStateStruct.currentGuardDogStage);
//      DEV_ASSERT_MSG(round < kGameplayRoundMap.size(),
//                     "RobotAudioClient.HandleRobotPublicState.InvalidRound",
//                     "round: %d", round);
//      
//      _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Gameplay_Round,
//                                                    static_cast<const GenericSwitch>(kGameplayRoundMap[round]),
//                                                    AudioMetaData::GameObjectType::Default);
//      
//      _prevGuardDogStage = currPublicStateStruct.currentGuardDogStage;
//    }
//  } else if (GetActiveBehaviorStage() != BehaviorStageTag::GuardDog) {
//    // reset GuardDog stuff
//    _prevGuardDogStage = GuardDogStage::Count;
//  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioComponent::HandleDancingUpdates(const BehaviorStageStruct& currPublicStateStruct)
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
void BehaviorAudioComponent::HandleFeedingUpdates(const BehaviorStageStruct& currPublicStateStruct)
{
//  const bool stageOrRoundChanged = (GetActiveBehaviorStage() != BehaviorStageTag::Feeding) ||
//                                     (_prevFeedingStage != currPublicStateStruct.currentFeedingStage);
//  
//  if((currPublicStateStruct.behaviorStageTag == BehaviorStageTag::Feeding) &&
//     stageOrRoundChanged){
//    _prevFeedingStage =  currPublicStateStruct.currentFeedingStage;
//    _robot.GetRobotAudioClient()->PostSwitchState(SwitchGroupType::Gameplay_Round,
//                                                  static_cast<const GenericSwitch>(kGameplayRoundMap[_round]),
//                                                  AudioMetaData::GameObjectType::Default);
//  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioComponent::HandleNeedsUpdates(const NeedsLevels& needsLevel)
{
//  using AGO = AudioMetaData::GameObjectType;
//  if(!FLT_NEAR(needsLevel.energy, _needsLevel.energy)){
//    _needsLevel.energy = needsLevel.energy;
//    _robot.GetRobotAudioClient()->PostParameter(AudioMetaData::GameParameter::ParameterType::Nurture_Energy,
//                                                _needsLevel.energy,
//                                                AGO::Invalid);
//  }
//  
//  if(!FLT_NEAR(needsLevel.repair, _needsLevel.repair)){
//    _needsLevel.repair = needsLevel.repair;
//    _robot.GetRobotAudioClient()->PostParameter(AudioMetaData::GameParameter::ParameterType::Nurture_Repair,
//                                                _needsLevel.repair,
//                                                AGO::Invalid);
//  }
//  
//  if(!FLT_NEAR(needsLevel.play, _needsLevel.play)){
//    _needsLevel.play = needsLevel.play;
//    _robot.GetRobotAudioClient()->PostParameter(AudioMetaData::GameParameter::ParameterType::Nurture_Play,
//                                                _needsLevel.play,
//                                                AGO::Invalid);
//  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStageTag BehaviorAudioComponent::GetActiveBehaviorStage()
{
  return _activeBehaviorStage;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioComponent::SetActiveBehaviorStage(BehaviorStageTag stageTag)
{
  DEV_ASSERT_MSG((stageTag == BehaviorStageTag::Count) ||
                 (_activeBehaviorStage == BehaviorStageTag::Count),
                 "BehaviorAudioComponent.SetActiveBehaviorStage.StageAlreadySet",
                 "Trying to set new stage tag %s, but stage tag is already set as %s",
                 BehaviorStageTagToString(stageTag),
                 BehaviorStageTagToString(_activeBehaviorStage));
  _activeBehaviorStage = stageTag;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAudioComponent::HandleDimMusicForActivity(const RobotPublicState& stateEvent)
{
//  using AE = AudioMetaData::GameEvent::GenericEvent;
//  if(_prevActivity != stateEvent.currentActivity)
//  {
//    if(stateEvent.currentActivity == ActivityID::Singing)
//    {
//      _robot.GetRobotAudioClient()->PostEvent(static_cast<AE>(AudioMetaData::GameEvent::App::Music_Dim_On),
//                                              kMusicGameObject);
//    }
//    else if(_prevActivity == ActivityID::Singing)
//    {
//      _robot.GetRobotAudioClient()->PostEvent(static_cast<AE>(AudioMetaData::GameEvent::App::Music_Dim_Off),
//                                              kMusicGameObject);
//    }
//  }
}

  
  
} // Audio
} // Cozmo
} // Anki
