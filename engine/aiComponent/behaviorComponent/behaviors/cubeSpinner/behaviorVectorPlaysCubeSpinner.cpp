/**
 * File: BehaviorVectorPlaysCubeSpinner.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-26
 *
 * Description: A Behavior where Vector plays the cube spinner game
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/cubeSpinner/behaviorVectorPlaysCubeSpinner.h"

#include "clad/types/animationTrigger.h"
#include "coretech/common/engine/jsonTools.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/cubeSpinner/cubeSpinnerGame.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/dataAccessorComponent.h"

#include "util/console/consoleInterface.h"

#include <algorithm>

namespace Anki {
namespace Cozmo {
  
namespace {
const char* kGameConfigKey = "gameConfig";
const char* kLockAnimationName = "anim_spinner_tap_01";
const float kDistanceFromMarker_mm =  25.0f;
const float kPreActionAngleTol_deg = 15.0f;
const int kMaxDistanceFromCube_mm = kDistanceFromMarker_mm + 20;

// game config
const char* kMinRoundsToPlayKey = "minRoundsToPlay";
const char* kMaxRoundsToPlayKey = "maxRoundsToPlay";

// vector player config
const char* kVectorPlayerConfigKey                = "vectorPlayerConfig";
const char* kOddsOfMissingPatternKey              = "oddsOfMissingPattern";
const char* kOddsOfTappingAfterCorrectPatternKey  = "oddsOfTappingAfterCorrectPattern";
const char* kOddsOfTappingCorrectPatternOnLockKey = "oddsOfTappingCorrectPatternOnLock";

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVectorPlaysCubeSpinner::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVectorPlaysCubeSpinner::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVectorPlaysCubeSpinner::VectorPlayerConfig::VectorPlayerConfig(const Json::Value& config)
{
  // oddsOfMissingPattern
  if(config[kOddsOfMissingPatternKey].isArray()){
    int i = 0;
    for(const auto& entry: config[kOddsOfMissingPatternKey]){
      if(i >= CubeLightAnimation::kNumCubeLEDs){
        PRINT_NAMED_ERROR("BehaviorVectorPlaysCubeSpinner.VectorPlayerConfig.TooManyMissingPatterns", "");
        break;
      }
      oddsOfMissingPattern[i] = entry.asFloat();
      i++;
    }
  }

  // oddsOfTappingAfterCorrectPattern
  if(config[kOddsOfTappingAfterCorrectPatternKey].isArray()){
    int i = 0;
    for(const auto& entry: config[kOddsOfTappingAfterCorrectPatternKey]){
      if(i >= CubeLightAnimation::kNumCubeLEDs){
        PRINT_NAMED_ERROR("BehaviorVectorPlaysCubeSpinner.VectorPlayerConfig.TooManyCorrectPatterns", "");
        break;
      }
      oddsOfTappingAfterCorrectPattern[i] = entry.asInt();
      i++;
    }
  }

  // oddsOfTappingCorrectPatternOnLock
  if(config[kOddsOfTappingCorrectPatternOnLockKey].isArray()){
    int i = 0;
    for(const auto& entry: config[kOddsOfTappingCorrectPatternOnLockKey]){
      if(i >= CubeLightAnimation::kNumCubeLEDs){
        PRINT_NAMED_ERROR("BehaviorVectorPlaysCubeSpinner.VectorPlayerConfig.TooManyPatternOnLock", "");
        break;
      }
      oddsOfTappingCorrectPatternOnLock[i] = entry.asInt();
      i++;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVectorPlaysCubeSpinner::BehaviorVectorPlaysCubeSpinner(const Json::Value& config)
 : ICozmoBehavior(config)
{
  _iConfig.gameConfig = config[kGameConfigKey];
  
  SubscribeToTags({
    EngineToGameTag::ObjectTapped
  });

  const std::string debugName = "BehaviorVectorPlaysCubeSpinner.Config.KeyIssue.";

  _iConfig.minRoundsToPlay = JsonTools::ParseInt32(config, kMinRoundsToPlayKey, (debugName + kMinRoundsToPlayKey).c_str());
  _iConfig.maxRoundsToPlay = JsonTools::ParseInt32(config, kMaxRoundsToPlayKey, (debugName + kMaxRoundsToPlayKey).c_str());

  _iConfig.playerConfig = VectorPlayerConfig(config[kVectorPlayerConfigKey]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVectorPlaysCubeSpinner::~BehaviorVectorPlaysCubeSpinner()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorVectorPlaysCubeSpinner::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kGameConfigKey,
    kMinRoundsToPlayKey,
    kMaxRoundsToPlayKey,
    kVectorPlayerConfigKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::InitBehavior()
{
  const auto& lightConfig = GetBEI().GetDataAccessorComponent().GetCubeSpinnerConfig();

  _iConfig.cubeSpinnerGame = std::make_unique<CubeSpinnerGame>(_iConfig.gameConfig, lightConfig,
                                                               GetBEI().GetCubeCommsComponent(),
                                                               GetBEI().GetCubeLightComponent(), 
                                                               GetBEI().GetBackpackLightComponent(),
                                                               GetBEI().GetBlockWorld(),
                                                               GetBEI().GetRobotInfo().GetRNG());
  
  auto lockedCallback = [this](CubeSpinnerGame::LockResult result){
    _dVars.lastLockResult = result;
  };

  _iConfig.cubeSpinnerGame->RegisterLightLockedCallback(lockedCallback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  _dVars.roundsLeftToPlay = GetBEI().GetRobotInfo().GetRNG().RandIntInRange(_iConfig.minRoundsToPlay, 
                                                                            _iConfig.maxRoundsToPlay);
  ResetGame();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::BehaviorUpdate() 
{
  if( !IsActivated()) {
    return;
  }

  if(!IsControlDelegated() && !IsInPositionToTap()){
    TransitionToDriveToCube();
  }

  if(!_dVars.hasGameStarted){
    return;
  }

  if(!IsControlDelegated()){
    MakeTapDecision();
  }
  
  _iConfig.cubeSpinnerGame->Update();

  if(_dVars.lastLockResult != CubeSpinnerGame::LockResult::Count){
    switch(_dVars.lastLockResult){
      case CubeSpinnerGame::LockResult::Locked:{
        break;
      }
      case CubeSpinnerGame::LockResult::Error:{
        DelegateNow(new TriggerAnimationAction(AnimationTrigger::DEPRECATED_OnSpeedtapRoundPlayerWinLowIntensity), [this](){
          TransitionToNextGame();
        });
        break;
      }
      case CubeSpinnerGame::LockResult::Complete:{
        DelegateNow(new TriggerAnimationAction(AnimationTrigger::FistBumpSuccess), [this](){
          TransitionToNextGame();
        });
        break;
      }
      default:{
        PRINT_NAMED_ERROR("BehaviorDevCubeSpinnerConsole.BehaviorUpdate.UnknownLockState", "");
        break;
      }
    }

    _dVars.lastLockResult = CubeSpinnerGame::LockResult::Count;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::OnBehaviorDeactivated()
{
  _iConfig.cubeSpinnerGame->StopGame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::TransitionToDriveToCube()
{
  DriveToAlignWithObjectAction* action = new DriveToAlignWithObjectAction(_dVars.objID,
                                                                          kDistanceFromMarker_mm);
  action->SetPreActionPoseAngleTolerance(DEG_TO_RAD(kPreActionAngleTol_deg));

  auto* compoundAction = new CompoundActionSequential({
    new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::HIGH_DOCK),
    action
  });

  DelegateIfInControl(compoundAction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::TransitionToNextGame()
{
  if(_dVars.roundsLeftToPlay <= 0){
    _iConfig.cubeSpinnerGame->StopGame();
    CancelSelf();
  }else{
    _dVars.roundsLeftToPlay--;
    ResetGame();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::ResetGame()
{
  auto callback = [this](bool gameStartupSuccess, const ObjectID& id){
    if(gameStartupSuccess){
      _dVars.hasGameStarted = true;
      _dVars.objID = id;
    }else{
      CancelSelf();
    }
  };

  _dVars.hasGameStarted = false;
  _dVars.objID.UnSet();
  _iConfig.cubeSpinnerGame->RequestStartNewGame(callback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::MakeTapDecision()
{
  bool areLightsCycling = false;
  uint8_t currentLitLEDIdx = 0;
  bool isCurrentLightTarget = false;
  CubeSpinnerGame::LightsLocked lightsLocked; 
  TimeStamp_t timeUntilNextRotation = 0;

  _iConfig.cubeSpinnerGame->GetGameSnapshot(areLightsCycling, currentLitLEDIdx, isCurrentLightTarget,
                                            lightsLocked, timeUntilNextRotation);
  if(!areLightsCycling){
    return;
  }

  // calculate the current round number                                       
  auto roundNumber = 0;
  for(const auto& entry : lightsLocked){
    if(entry){
      roundNumber++;
    }
  }

  // If starting a new cycle, decide what action we'll take this cycle
  if(currentLitLEDIdx == 0){
    auto odds = GetBEI().GetRobotInfo().GetRNG().RandDbl();

    // failure because robot missed pattern
    if(_dVars.wasLastCycleTarget){
      if(odds < _iConfig.playerConfig.oddsOfTappingAfterCorrectPattern[roundNumber]){
        const bool shouldSelectLockedIdx = true;
        _dVars.lightIdxToLock = SelectIndexToLock(lightsLocked, shouldSelectLockedIdx);
      }
    }


    if(isCurrentLightTarget){
      // failure because robot tapped correct light, but on lock 
      if(odds < _iConfig.playerConfig.oddsOfTappingCorrectPatternOnLock[roundNumber]){
        const bool shouldSelectLockedIdx = true;
        _dVars.lightIdxToLock = SelectIndexToLock(lightsLocked, shouldSelectLockedIdx);
      }

      // success
      if(odds > _iConfig.playerConfig.oddsOfMissingPattern[roundNumber]){
        const bool shouldSelectLockedIdx = false;
        _dVars.lightIdxToLock = SelectIndexToLock(lightsLocked, shouldSelectLockedIdx);
      }
    }
  }

  _dVars.wasLastCycleTarget = isCurrentLightTarget;

  if((_dVars.lightIdxToLock < CubeLightAnimation::kNumCubeLEDs) && 
     (_dVars.lightIdxToLock == currentLitLEDIdx)){
    _dVars.lightIdxToLock = CubeLightAnimation::kNumCubeLEDs;
    DelegateIfInControl(new PlayAnimationAction(kLockAnimationName));
    _iConfig.cubeSpinnerGame->LockNow();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorVectorPlaysCubeSpinner::IsInPositionToTap() const
{
  const auto* obj = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.objID);
  if(obj != nullptr){
    f32 outDistanceSq = 0;
    if(ComputeDistanceSQBetween(obj->GetPose(), GetBEI().GetRobotInfo().GetPose(), outDistanceSq)){
      return outDistanceSq < (kMaxDistanceFromCube_mm * kMaxDistanceFromCube_mm);
    }
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::HandleWhileActivated(const EngineToGameEvent& event)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::ObjectTapped:
    {
      if(event.GetData().Get_ObjectTapped().objectID == _dVars.objID ){
        _iConfig.cubeSpinnerGame->LockNow();
      }
      break;
    }
      
    default:
      PRINT_NAMED_ERROR("BehaviorDevCubeSpinnerConsole.HandleWhileActivated.InvalidTag",
                        "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t BehaviorVectorPlaysCubeSpinner::SelectIndexToLock(const CubeSpinnerGame::LightsLocked& lights, bool shouldSelectLockedIdx)
{
  std::vector<uint8_t> validIdxs;
  int i = 0;
  for(const auto& entry: lights){
    if(entry == shouldSelectLockedIdx){
      validIdxs.push_back(i);
    }
    i++;
  }

  if(validIdxs.empty()){
    return 0;
  }

  unsigned int seed = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
  shuffle(validIdxs.begin(), validIdxs.end(), std::default_random_engine(seed));
  return validIdxs.front();
}



}
}
