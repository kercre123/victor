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

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationTrigger.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
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
const float kDistanceFromMarker_mm =  30.0f;
const float kPreActionAngleTol_deg = 15.0f;
const int kMaxDistanceFromCube_mm = kDistanceFromMarker_mm + 20;
const int kMinTimeBetweenTaps_ms = 2000;

// game config
const char* kMinRoundsToPlayKey = "minRoundsToPlay";
const char* kMaxRoundsToPlayKey = "maxRoundsToPlay";
const char* kMaxTimeSearchForCube_ms = "maxTimeSearchForCube_ms";

// vector player config
const char* kVectorPlayerConfigKey                = "vectorPlayerConfig";
const char* kOddsOfMissingPatternKey              = "oddsOfMissingPattern";
const char* kOddsOfTappingAfterCorrectPatternKey  = "oddsOfTappingAfterCorrectPattern";
const char* kOddsOfTappingCorrectPatternOnLockKey = "oddsOfTappingCorrectPatternOnLock";

// Currently cube spinner relies on "faking" vector's tap in case there are gear/lift issues,
// cube sensitivity issues, animation variants that apply different amounts of pressure, light lag etc
// Despite the fact that Vector recognizing what light is on the cube is not handled through the vision system
// there is a concern from Hanns/Troy that "faking" the interaction is un-Vector-like
// So if you really really want to break cube spinner and introduce tons of potential bugs that will be extremely 
// hard to test for go ahead and flip this bool
CONSOLE_VAR(bool, kIReallyReallyWantToBreakCubeSpinner, "CubeSpinner", false);

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
  _iConfig.maxLengthSearchForCube_ms = JsonTools::ParseUInt32(config, kMaxTimeSearchForCube_ms,
                                                              (debugName + kMaxTimeSearchForCube_ms).c_str());

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
  delegates.insert(_iConfig.searchBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kGameConfigKey,
    kMinRoundsToPlayKey,
    kMaxRoundsToPlayKey,
    kVectorPlayerConfigKey,
    kMaxTimeSearchForCube_ms
  };
  expectedKeys.insert( std::begin(list), std::end(list) );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::InitBehavior()
{
  _iConfig.searchBehavior = GetBEI().GetBehaviorContainer().FindBehaviorByID(BEHAVIOR_ID(Hiking_LookInPlace360));

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
  
  if(_dVars.stage == BehaviorStage::SearchingForCube &&
     IsControlDelegated()){
    if(IsCubePositionKnown()){
      CancelDelegates();
      TransitionToFindCubeAndApproachCube();
    }else{
      const auto currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      if(currTime_ms > _dVars.timeSearchForCubeShouldEnd_ms){
        CancelSelf();
        return;
      }
    }
  }
  
  if((_dVars.stage == BehaviorStage::ApproachingCube) &&
     !IsCubePositionKnown()){
    TransitionToSearchForCube();
  }

  if(!IsControlDelegated() && !IsInPositionToTap()){
    TransitionToFindCubeAndApproachCube();
  }
  
  if(!IsControlDelegated() &&
     IsInPositionToTap() && 
     (_dVars.stage < BehaviorStage::WaitingForGameToStart)){
    if(_dVars.isCubeSpinnerGameReady){
      _iConfig.cubeSpinnerGame->StartGame();
      _dVars.stage = BehaviorStage::GameHasStarted;
    }else{
      _dVars.stage = BehaviorStage::WaitingForGameToStart;
    }
  }

  if(_dVars.stage < BehaviorStage::GameHasStarted){
    return;
  }

  CubeSpinnerGame::GameSnapshot snapshot;
  _iConfig.cubeSpinnerGame->GetGameSnapshot(snapshot);
  
  MakeTapDecision(snapshot);
  
  if(!IsControlDelegated()){
    TapIfAppropriate(snapshot);
  }
  
  _iConfig.cubeSpinnerGame->Update();

  if(_dVars.lastLockResult != CubeSpinnerGame::LockResult::Count){
    switch(_dVars.lastLockResult){
      case CubeSpinnerGame::LockResult::Locked:{
        break;
      }
      case CubeSpinnerGame::LockResult::Error:{
        _dVars.nextResponseAnimation = AnimationTrigger::DEPRECATED_OnSpeedtapRoundPlayerWinLowIntensity;
        break;
      }
      case CubeSpinnerGame::LockResult::Complete:{
        _dVars.nextResponseAnimation = AnimationTrigger::FistBumpSuccess;
        break;
      }
      default:{
        PRINT_NAMED_ERROR("BehaviorDevCubeSpinnerConsole.BehaviorUpdate.UnknownLockState", "");
        break;
      }
    }

    _dVars.lastLockResult = CubeSpinnerGame::LockResult::Count;
  }
 

  // Wait till current tap animation has finished, then respond to it
  if(!IsControlDelegated() && 
     (_dVars.nextResponseAnimation != AnimationTrigger::Count)){
    DelegateIfInControl(new TriggerAnimationAction(_dVars.nextResponseAnimation), [this](){
      TransitionToNextGame();
    });
    _dVars.nextResponseAnimation = AnimationTrigger::Count;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::OnBehaviorDeactivated()
{
  _iConfig.cubeSpinnerGame->StopGame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::TransitionToFindCubeAndApproachCube()
{
  ObjectID target;
  if(!IsCubePositionKnown()){
    TransitionToSearchForCube();
  }else if(GetBestGuessObjectID(target) && target.IsSet()){
    DriveToAlignWithObjectAction* action = new DriveToAlignWithObjectAction(target,
                                                                            kDistanceFromMarker_mm);
    action->SetPreActionPoseAngleTolerance(DEG_TO_RAD(kPreActionAngleTol_deg));

    auto* compoundAction = new CompoundActionSequential({
      new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::HIGH_DOCK),
      action
    });

    DelegateIfInControl(compoundAction);
    _dVars.stage = BehaviorStage::ApproachingCube;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::TransitionToSearchForCube()
{
  if(IsControlDelegated()){
    CancelDelegates(false);
  }
  
  ANKI_VERIFY(_iConfig.searchBehavior->WantsToBeActivated(),
              "BehaviorVectorPlaysCubeSpinner.TransitionToFindCubeAndApproachCube.SearchDoesntWantToBeActivated","");
  DelegateIfInControl(_iConfig.searchBehavior.get());
  const auto currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  _dVars.timeSearchForCubeShouldEnd_ms = (currTime_ms + _iConfig.maxLengthSearchForCube_ms);
  _dVars.stage = BehaviorStage::SearchingForCube;
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
      _dVars.isCubeSpinnerGameReady = true;
      _dVars.objID = id;
    }else{
      CancelSelf();
    }
  };

  _dVars.isCubeSpinnerGameReady = false;
  _dVars.objID.UnSet();
  _iConfig.cubeSpinnerGame->PrepareForNewGame(callback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::MakeTapDecision(const CubeSpinnerGame::GameSnapshot& snapshot)
{
  if(!snapshot.areLightsCycling){
    return;
  }


  const auto currTime_ms =  BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  if((_dVars.timeOfLastTap + kMinTimeBetweenTaps_ms) > currTime_ms){
    return;
  }

  // If starting a new cycle, decide what action we'll take this cycle
  if(snapshot.currentLitLEDIdx == 0){
    auto odds = GetBEI().GetRobotInfo().GetRNG().RandDbl();

    // failure because robot missed pattern
    if(_dVars.wasLastCycleTarget){
      if(odds < _iConfig.playerConfig.oddsOfTappingAfterCorrectPattern[snapshot.roundNumber]){
        const bool shouldSelectLockedIdx = true;
        _dVars.lightIdxToLock = SelectIndexToLock(snapshot.lightsLocked, shouldSelectLockedIdx);
      }
    }


    if(snapshot.isCurrentLightTarget){
      // failure because robot tapped correct light, but on lock 
      if(odds < _iConfig.playerConfig.oddsOfTappingCorrectPatternOnLock[snapshot.roundNumber]){
        const bool shouldSelectLockedIdx = true;
        _dVars.lightIdxToLock = SelectIndexToLock(snapshot.lightsLocked, shouldSelectLockedIdx);
      }

      // success
      if(odds > _iConfig.playerConfig.oddsOfMissingPattern[snapshot.roundNumber]){
        const bool shouldSelectLockedIdx = false;
        _dVars.lightIdxToLock = SelectIndexToLock(snapshot.lightsLocked, shouldSelectLockedIdx);
      }
    }
  }

  _dVars.wasLastCycleTarget = snapshot.isCurrentLightTarget;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::TapIfAppropriate(const CubeSpinnerGame::GameSnapshot& snapshot)
{
  if((_dVars.lightIdxToLock < CubeLightAnimation::kNumCubeLEDs) &&
     (_dVars.lightIdxToLock == snapshot.currentLitLEDIdx)){
    _dVars.timeOfLastTap = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    _dVars.lightIdxToLock = CubeLightAnimation::kNumCubeLEDs;
    DelegateIfInControl(new PlayAnimationAction(kLockAnimationName));
    if(!kIReallyReallyWantToBreakCubeSpinner){
      _iConfig.cubeSpinnerGame->LockNow();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorVectorPlaysCubeSpinner::IsCubePositionKnown() const
{
  if(_dVars.objID.IsSet()){
    const auto* obj = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.objID);
    return obj != nullptr;
  }else{
    const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
    const auto* obj = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose);
    return obj != nullptr;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorVectorPlaysCubeSpinner::IsInPositionToTap() const
{
  if(!_dVars.objID.IsSet()){
    return false;
  }

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
bool BehaviorVectorPlaysCubeSpinner::GetBestGuessObjectID(ObjectID& bestGuessID) const
{
  if(_dVars.objID.IsSet()){
    bestGuessID = _dVars.objID;
    return true;
  }else{
    const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
    const auto* obj = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose);
    if(obj != nullptr){
      bestGuessID = obj->GetID();
      return true;
    }else{
      return false;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVectorPlaysCubeSpinner::HandleWhileActivated(const EngineToGameEvent& event)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::ObjectTapped:
    {
      if((_dVars.stage == BehaviorStage::GameHasStarted) && 
         (event.GetData().Get_ObjectTapped().objectID == _dVars.objID)){
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
