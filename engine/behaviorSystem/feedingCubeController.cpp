/**
 * File: feedingCubeController.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2017-03-28
 *
 * Description: Controls the selection and setting of cube lights and cozmo's
 * lights during the feeding process.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/behaviorSystem/feedingCubeController.h"

#include "engine/activeObject.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/feedingSoundEffectManager.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/bodyLightComponent.h"
#include "engine/components/cubeAccelComponent.h"
#include "engine/components/cubeLightComponent.h"
#include "engine/robot.h"

#include "anki/common/basestation/colorRGBA.h"
#include "anki/common/basestation/utils/timer.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
namespace {

enum class ChargeState{
  NoCharge,
  Charging,
  LoosingCharge,
  FullyCharged
};

#define CONSOLE_GROUP "Activity.Feeding"
  
CONSOLE_VAR(f32, kTimeBetweenShakes_s,            CONSOLE_GROUP, 0.07f);
CONSOLE_VAR(f32, kTimeBeforeStartLosingCharge_s,  CONSOLE_GROUP, 1.0f);
CONSOLE_VAR(f32, kTimeBetweenLoosingCharge_s,     CONSOLE_GROUP, 0.1f);
CONSOLE_VAR(f32, kChargeLevelToFillSide,          CONSOLE_GROUP, 4.0f);
CONSOLE_VAR(f32, kShakeMinThresh,                 CONSOLE_GROUP, 1200.f);

// Constants for the Shake Component MovementListener:
const float kHighPassFiltCoef    = 0.4f;
const float kHighPassLowerThresh = 2.5f;
const float kHighPassUpperThresh = 3.9f;

ObjectLights kFillingBlockLights;
  
const char* ChargeStateToString(ChargeState state){
  switch(state){
    case ChargeState::NoCharge:     {return "NoCharge"; break;}
    case ChargeState::Charging:     {return "Charging"; break;}
    case ChargeState::LoosingCharge:{return "LoosingCharge"; break;}
    case ChargeState::FullyCharged: {return "FullyCharged"; break;}
  }
}
  
const char* ControllerStateToString(FeedingCubeController::ControllerState state){
  switch(state){
    case FeedingCubeController::ControllerState::Activated:  {return "Activated"; break;}
    case FeedingCubeController::ControllerState::Deactivated:{return "Deactivated"; break;}
    case FeedingCubeController::ControllerState::DrainCube:  {return "DrainCube"; break;}
  }
}

} // namespace
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct CubeStateTracker{
  CubeStateTracker(const ObjectID& objectID): _id(objectID){}
  ObjectID _id;
  std::shared_ptr<CubeAccelListeners::ShakeListener> _cubeMovementListener;
  
  // tracking shake info
  float _timeNextShakeCounts_s = 0.0f;
  float _timeLoseNextCharge_s = 0.0f;
  int   _currentChargeLevel = 0;
  
  // tracking drain
  float _timeStartedDraining_s = -1.0f;
  float _timeToDrainCube = -1.0f;
  
  // tracking light state
  CubeAnimationTrigger _currentAnimationTrigger = CubeAnimationTrigger::Count;
  CubeAnimationTrigger _desiredAnimationTrigger = CubeAnimationTrigger::Count;
  ObjectLights         _desiredModifier;
  bool                 _desiredModifierChanged = false;
  
  ChargeState GetChargeState() const { return _chargeState;};
  void SetChargeState(ChargeState newChargeState);
  
private:
  ChargeState _chargeState = ChargeState::NoCharge;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CubeStateTracker::SetChargeState(ChargeState newChargeState)
{
  if( _chargeState != newChargeState ) {
    PRINT_CH_INFO("Feeding",
                  "CubeStateTracker.SetChargeState.NewChargeState",
                  "Charge state for object %d set from %s to %s",
                  _id.GetValue(),
                  ChargeStateToString(_chargeState),
                  ChargeStateToString(newChargeState));
  }
  _chargeState = newChargeState;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FeedingCubeController::FeedingCubeController(Robot& robot, const ObjectID& objectControlling)
: _currentState(ControllerState::Deactivated)
, _cubeStateTracker(std::make_unique<CubeStateTracker>(objectControlling))
{
  kFillingBlockLights.onColors = {{NamedColors::CYAN, NamedColors::CYAN,
                                   NamedColors::CYAN, NamedColors::CYAN}};
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FeedingCubeController::~FeedingCubeController()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::SetControllerState(Robot& robot, ControllerState newState, float eatingDuration_s)
{
  if(_currentState == newState){
    PRINT_NAMED_WARNING("FeedingCubeController.SetControllerState.StateAlreadySet",
                        "Attempting to set new controller state %s, but that is already the state",
                        ControllerStateToString(newState));
    return;
  }
  
  switch(newState){
    case ControllerState::Activated:
    {
      StartListeningForShake(robot);
      InitializeController(robot);
      break;
    }
    case ControllerState::Deactivated:
    {
      ClearController(robot);
      break;
    }
    case ControllerState::DrainCube:
    {
      StartCubeDrain(robot, eatingDuration_s);
      break;
    }
  }

  if( newState != _currentState ) {
    PRINT_CH_INFO("Feeding",
                  "FeedingCubeController.SetControllerState.NewState",
                  "Feeding cube controller for id %d switched to state %s",
                  _cubeStateTracker->_id.GetValue(),
                  ControllerStateToString(newState));
  }
  
  _currentState = newState;
}

  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::InitializeController(Robot& robot)
{
  _cubeStateTracker->_currentAnimationTrigger = CubeAnimationTrigger::Count;
  ReInitializeController(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::ReInitializeController(Robot& robot)
{
  _cubeStateTracker->SetChargeState(ChargeState::NoCharge);
  _cubeStateTracker->_timeNextShakeCounts_s = 0.0f;
  _cubeStateTracker->_timeLoseNextCharge_s   = 0.0f;
  _cubeStateTracker->_currentChargeLevel     = 0;
  _cubeStateTracker->_timeStartedDraining_s = -1.0f;
  _cubeStateTracker->_desiredAnimationTrigger = CubeAnimationTrigger::FeedingCyanCycle;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::ClearController(Robot& robot)
{
  // Re-set lights
  _cubeStateTracker->_desiredAnimationTrigger = CubeAnimationTrigger::Count;
  SetCubeLights(robot);
  
  // Re-set shake listener
  robot.GetCubeAccelComponent().RemoveListener(_cubeStateTracker->_id,
                                               _cubeStateTracker->_cubeMovementListener);
  _cubeStateTracker->_cubeMovementListener.reset();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::Update(Robot& robot)
{
  switch(_currentState){
    case ControllerState::Activated:
    {
      CheckForChargeStateChanges(robot);
      break;
    }
    case ControllerState::DrainCube:
    {
      if(FLT_GT(_cubeStateTracker->_timeStartedDraining_s, 0)){
        UpdateCubeDrain(robot);
      }
      break;
    }
    case ControllerState::Deactivated:
    {
      return;
    }
  }
  
  SetCubeLights(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::CheckForChargeStateChanges(Robot& robot)
{
  if(_cubeStateTracker->GetChargeState() != ChargeState::FullyCharged){
    // first check for lost shakes
    const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const bool shouldLoseCharge = (_cubeStateTracker->_timeLoseNextCharge_s < currentTime_s) &&
                                 (_cubeStateTracker->_currentChargeLevel > 0);
    if(shouldLoseCharge){
      if(_cubeStateTracker->GetChargeState() == ChargeState::Charging){
        _cubeStateTracker->SetChargeState(ChargeState::LoosingCharge);
      }
      _cubeStateTracker->_timeLoseNextCharge_s = currentTime_s + kTimeBetweenLoosingCharge_s;
      _cubeStateTracker->_currentChargeLevel--;
      
      PRINT_CH_DEBUG("Feeding",
                     "FeedingCubeController.CheckForChargeStateChanges.ShakeDecreasing",
                     "Cube with id %d has not detected new shake, so decreasing shake count to %d",
                     _cubeStateTracker->_id.GetValue(),
                     _cubeStateTracker->_currentChargeLevel);
      
      // update charge audio to match
      if(_cubeStateTracker->_currentChargeLevel == 0){
        UpdateChargeAudioRound(robot, ChargeStateChange::Charge_Stop);
      }else{
        // Currently audio has 30 steps while lights only have 16 - so increments
        // and decrements come in pairs
        UpdateChargeAudioRound(robot, ChargeStateChange::Charge_Down);
        UpdateChargeAudioRound(robot, ChargeStateChange::Charge_Down);
      }
    }
    
    const bool didLoseAllCharge = (_cubeStateTracker->_currentChargeLevel <= 0) &&
                                     !FLT_NEAR(_cubeStateTracker->_timeLoseNextCharge_s, 0.0f);
    const bool didCompleteCharge = _cubeStateTracker->_currentChargeLevel >= 4*kChargeLevelToFillSide;
    const bool hasAnyCharge = _cubeStateTracker->_currentChargeLevel > 0;
    
    // now check if we're back to waiting or fully charged
    if(didLoseAllCharge){
      // re-initialize to no charge state
      ReInitializeController(robot);
    }else if(didCompleteCharge) {
      if(_cubeStateTracker->_currentChargeLevel == 4*kChargeLevelToFillSide){
        UpdateChargeAudioRound(robot, ChargeStateChange::Charge_Stop);
        UpdateChargeAudioRound(robot, ChargeStateChange::Charge_Complete);
      }
      
      UpdateChargeLights(robot);
      _cubeStateTracker->SetChargeState(ChargeState::FullyCharged);
      
    }else if(hasAnyCharge){
      UpdateChargeLights(robot);
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::StartCubeDrain(Robot& robot, float eatingDuration_s)
{
  _currentState = ControllerState::DrainCube;
  _cubeStateTracker->_timeStartedDraining_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _cubeStateTracker->_timeToDrainCube = eatingDuration_s;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::UpdateCubeDrain(Robot& robot)
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // Continuously drain the cube lights as a precentage of the time remaining to drain it
  // until the cube is fully drained
  if((_cubeStateTracker->_timeStartedDraining_s > 0) &&
     (_cubeStateTracker->_timeStartedDraining_s + _cubeStateTracker->_timeToDrainCube > currentTime_s)){
    const float percentRemaining = 1.0f - (
      (currentTime_s - _cubeStateTracker->_timeStartedDraining_s) / _cubeStateTracker->_timeToDrainCube);
    
    // Calculate how much to fade all light sides together
    ObjectLights modifiableLights;
    ColorRGBA onColors = kFillingBlockLights.onColors[0];
    ColorRGBA drainedColor = ColorRGBA(static_cast<u8>(onColors.r() * percentRemaining),
                                       static_cast<u8>(onColors.g() * percentRemaining),
                                       static_cast<u8>(onColors.b() * percentRemaining));

    
    ObjectLEDArray onColorsModif = {{drainedColor, drainedColor,
                                       drainedColor, drainedColor}};
    modifiableLights.onColors = onColorsModif;
    
    // Set the appropraite animation and calculated modifier to be set by SetCubeLights
    _cubeStateTracker->_desiredAnimationTrigger = CubeAnimationTrigger::FeedingBlank;
    _cubeStateTracker->_desiredModifier = modifiableLights;
    _cubeStateTracker->_desiredModifierChanged = true;
  }else if(_cubeStateTracker->_timeStartedDraining_s + _cubeStateTracker->_timeToDrainCube < currentTime_s){
    ReInitializeController(robot);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::UpdateChargeLights(Robot& robot)
{
  // Calculate the percentage each side should be filled
  float percentSide1 = 1.0f;
  if(_cubeStateTracker->_currentChargeLevel < kChargeLevelToFillSide){
    percentSide1 = (_cubeStateTracker->_currentChargeLevel/(kChargeLevelToFillSide));
  }
  
  float percentSide2 = 1.0f;
  if(_cubeStateTracker->_currentChargeLevel < (kChargeLevelToFillSide)){
    percentSide2 = 0.0f;
  } else if(_cubeStateTracker->_currentChargeLevel < (kChargeLevelToFillSide * 2)){
    percentSide2 = ((_cubeStateTracker->_currentChargeLevel - kChargeLevelToFillSide) /(kChargeLevelToFillSide));
  }
  
  float percentSide3 = 1.0f;
  if(_cubeStateTracker->_currentChargeLevel < (kChargeLevelToFillSide * 2)){
    percentSide3 = 0.0f;
  } else if(_cubeStateTracker->_currentChargeLevel < (kChargeLevelToFillSide * 3)){
    percentSide3 = ((_cubeStateTracker->_currentChargeLevel - (kChargeLevelToFillSide*2))/(kChargeLevelToFillSide ));
  }
  
  float percentSide4 = 1.0f;
  if(_cubeStateTracker->_currentChargeLevel < (kChargeLevelToFillSide * 3)){
    percentSide4 = 0.0f;
  } else if(_cubeStateTracker->_currentChargeLevel < (kChargeLevelToFillSide * 4)){
    percentSide4 = ((_cubeStateTracker->_currentChargeLevel - (kChargeLevelToFillSide*3))/(kChargeLevelToFillSide));
  }
  
  // Translate those fill percentages into ObjectLights on cycle
  ObjectLights modifiableLights;
  ColorRGBA onColors = kFillingBlockLights.onColors[0];
  ColorRGBA Side1 = ColorRGBA(static_cast<u8>(onColors.r() * percentSide1),
                              static_cast<u8>(onColors.g() * percentSide1),
                              static_cast<u8>(onColors.b() * percentSide1));
  
  ColorRGBA Side2 = ColorRGBA(static_cast<u8>(onColors.r() * percentSide2),
                              static_cast<u8>(onColors.g() * percentSide2),
                              static_cast<u8>(onColors.b() * percentSide2));
  
  ColorRGBA Side3 = ColorRGBA(static_cast<u8>(onColors.r() * percentSide3),
                              static_cast<u8>(onColors.g() * percentSide3),
                              static_cast<u8>(onColors.b() * percentSide3));
  
  ColorRGBA Side4 = ColorRGBA(static_cast<u8>(onColors.r() * percentSide4),
                              static_cast<u8>(onColors.g() * percentSide4),
                              static_cast<u8>(onColors.b() * percentSide4));
  
  ObjectLEDArray onColorsModif = {{Side1, Side2, Side3, Side4}};
  modifiableLights.onColors = onColorsModif;
  
  // Set the appropraite animation and calculated modifier to be set by SetCubeLights
  _cubeStateTracker->_desiredAnimationTrigger = CubeAnimationTrigger::FeedingBlank;
  _cubeStateTracker->_desiredModifier = modifiableLights;
  _cubeStateTracker->_desiredModifierChanged = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::StartListeningForShake(Robot& robot)
{
  // generic lambda closure for cube accel listeners
  auto movementDetectedCallback = [this, &robot] (const float movementScore) {
    ShakeDetected(robot, movementScore);
  };
  
  auto listener = std::make_shared<CubeAccelListeners::ShakeListener>(kHighPassFiltCoef,
                                                                      kHighPassLowerThresh,
                                                                      kHighPassUpperThresh,
                                                                      movementDetectedCallback);
  robot.GetCubeAccelComponent().AddListener(_cubeStateTracker->_id, listener);
  
  DEV_ASSERT(_cubeStateTracker->_cubeMovementListener.get() == nullptr,
             "FeedingCubeController.StartListeningForShake.PreviousListenerAlreadySetup");
  _cubeStateTracker->_cubeMovementListener = listener;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::ShakeDetected(Robot& robot, const float shakeScore)
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool cubeCanCharge =
              (_currentState == ControllerState::Activated) &&
              (_cubeStateTracker->GetChargeState() != ChargeState::FullyCharged);
  
  if(cubeCanCharge &&
     (shakeScore > kShakeMinThresh) &&
     (_cubeStateTracker->_timeNextShakeCounts_s < currentTime_s)){
    
    // send updates to the audio system
    if(_cubeStateTracker->_currentChargeLevel > 1){
      // Currently audio has 30 steps while lights only have 16 - so increments
      // and decrements come in pairs, and we skip the first shake since it might
      // be a false positive
      UpdateChargeAudioRound(robot, ChargeStateChange::Charge_Up);
      UpdateChargeAudioRound(robot, ChargeStateChange::Charge_Up);
      
      PRINT_CH_DEBUG("Feeding",
                     "FeedingCubeController.ShakeDetected.ShakeIncreasing",
                     "Cube with id %d now has %d shakes",
                     _cubeStateTracker->_id.GetValue(),
                     _cubeStateTracker->_currentChargeLevel);
    }else if(_cubeStateTracker->_currentChargeLevel == 1){
      UpdateChargeAudioRound(robot, ChargeStateChange::Charge_Start);
    }
    
    // Update tracker properties
    _cubeStateTracker->SetChargeState(ChargeState::Charging);
    _cubeStateTracker->_currentChargeLevel++;
    _cubeStateTracker->_timeLoseNextCharge_s   = currentTime_s + kTimeBeforeStartLosingCharge_s;
    _cubeStateTracker->_timeNextShakeCounts_s = currentTime_s + kTimeBetweenShakes_s;
    
    PRINT_CH_DEBUG("Feeding",
                   "FeedingCubeController.IncrementingShakeCount",
                   "Shake count for active ID %d is now %d",
                   _cubeStateTracker->_id.GetValue(),
                   _cubeStateTracker->_currentChargeLevel);
    
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::SetCubeLights(Robot& robot)
{
  // If engine isn't in control of lights, the current animation will always be count
  if(!robot.GetCubeLightComponent().CanEngineSetLightsOnCube(_cubeStateTracker->_id)){
    _cubeStateTracker->_currentAnimationTrigger = CubeAnimationTrigger::Count;
    return;
  }
  
  // Remove any lights that are currently set
  if((_cubeStateTracker->_currentAnimationTrigger != _cubeStateTracker->_desiredAnimationTrigger) ||
     _cubeStateTracker->_desiredModifierChanged)
  {
    bool wasLightTransitionSuccessful = false;
    if(_cubeStateTracker->_currentAnimationTrigger == CubeAnimationTrigger::Count){
      wasLightTransitionSuccessful = robot.GetCubeLightComponent().PlayLightAnim(
                                       _cubeStateTracker->_id,
                                       _cubeStateTracker->_desiredAnimationTrigger);
    }
    else if(_cubeStateTracker->_desiredAnimationTrigger != CubeAnimationTrigger::Count)
    {
      wasLightTransitionSuccessful = robot.GetCubeLightComponent().StopAndPlayLightAnim(
                                       _cubeStateTracker->_id,
                                       _cubeStateTracker->_currentAnimationTrigger,
                                       _cubeStateTracker->_desiredAnimationTrigger,
                                       nullptr,
                                       _cubeStateTracker->_desiredModifierChanged,
                                       _cubeStateTracker->_desiredModifier);
    }
    else
    {
      wasLightTransitionSuccessful = robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(
                                       _cubeStateTracker->_currentAnimationTrigger,
                                       _cubeStateTracker->_id);
    }
    
    if(wasLightTransitionSuccessful){
      _cubeStateTracker->_currentAnimationTrigger = _cubeStateTracker->_desiredAnimationTrigger;
      _cubeStateTracker->_desiredModifierChanged = false;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::UpdateChargeAudioRound(Robot& robot, ChargeStateChange changeEnumVal)
{
  PRINT_CH_DEBUG("Feeding",
                 "FeedingCubeController.UpdateChargeAudioRound.ChargeStateChange",
                 "Cube with id %d is broadcasting audio state change %s",
                 _cubeStateTracker->_id.GetValue(),
                 ChargeStateChangeToString(changeEnumVal));
  
  robot.GetAIComponent().GetFeedingSoundEffectManager().NotifyChargeStateChange(
    robot, _cubeStateTracker->_id, _cubeStateTracker->_currentChargeLevel, changeEnumVal);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FeedingCubeController::IsCubeCharging() const
{
  return _cubeStateTracker->GetChargeState() == ChargeState::Charging;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FeedingCubeController::IsCubeCharged() const
{
  return _cubeStateTracker->GetChargeState() == ChargeState::FullyCharged;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* FeedingCubeController::ChargeStateChangeToString(ChargeStateChange state){
  switch(state){
    case ChargeStateChange::Charge_Start:    {return "Charge_Start"; break;}
    case ChargeStateChange::Charge_Up:       {return "Charge_Up"; break;}
    case ChargeStateChange::Charge_Down:     {return "Charge_Down"; break;}
    case ChargeStateChange::Charge_Complete: {return "Charge_Complete"; break;}
    case ChargeStateChange::Charge_Stop:     {return "Charge_Stop"; break;}
      
  }
}
  
} // namespace Cozmo
} // namespace Anki

