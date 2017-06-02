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

#include "anki/cozmo/basestation/behaviorSystem/feedingCubeController.h"

#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/bodyLightComponent.h"
#include "anki/cozmo/basestation/components/cubeAccelComponent.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/colorRGBA.h"
#include "anki/common/basestation/utils/timer.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
namespace{

enum class ChargeState{
  NoCharge,
  Charging,
  LoosingCharge,
  FullyCharged,
  Drained
};

  
CONSOLE_VAR(f32, kTimeBetweenShakes_s,            "Behavior.Feeding", 0.1f);
CONSOLE_VAR(f32, kTimeBeforeStartLosingCharge_s,  "Behavior.Feeding", 1.0f);
CONSOLE_VAR(f32, kTimeBetweenLoosingCharge_s,     "Behavior.Feeding", 0.1f);
CONSOLE_VAR(f32, kTimeToDrainCube,                "Behavior.Feeding", 2.5f);
CONSOLE_VAR(f32, kChargeLevelToFillSide,          "Behavior.Feeding", 4.0f);

// Constants for the Shake Component MovementListener:
const float kHighPassFiltCoef    = 0.4f;
const float kHighPassLowerThresh = 2.5f;
const float kHighPassUpperThresh = 3.9f;
const float kShakeMinThresh      = 3000.f;

ObjectLights kFillingBlockLights;
  
const char* ChargeStateToString(ChargeState state){
  switch(state){
    case ChargeState::NoCharge:     {return "NoCharge"; break;}
    case ChargeState::Charging:     {return "Charging"; break;}
    case ChargeState::LoosingCharge:{return "LoosingCharge"; break;}
    case ChargeState::FullyCharged: {return "FullyCharged"; break;}
    case ChargeState::Drained:      {return "Drained"; break;}
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
  
void CubeStateTracker::SetChargeState(ChargeState newChargeState)
{
  PRINT_CH_INFO("Feeding",
                "CubeStateTracker.SetChargeState.NewChargeState",
                "Charge state for object %d set from %s to %s",
                _id.GetValue(),
                ChargeStateToString(_chargeState),
                ChargeStateToString(newChargeState));
  _chargeState = newChargeState;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FeedingCubeController::FeedingCubeController(Robot& robot, const ObjectID& objectControlling)
: _currentStage(ControllerState::Deactivated)
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
void FeedingCubeController::SetControllerState(Robot& robot, ControllerState newState)
{
  if(_currentStage == newState){
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
      StartCubeDrain(robot);
      break;
    }
  }
  
  PRINT_CH_INFO("Feeding",
                "FeedingCubeController.SetControllerState.NewState",
                "Feeding cube controller for id %d switched to state %s",
                _cubeStateTracker->_id.GetValue(),
                ControllerStateToString(newState));
  _currentStage = newState;
}

  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::InitializeController(Robot& robot)
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
  robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(_cubeStateTracker->_currentAnimationTrigger,
                                                               _cubeStateTracker->_id);
  robot.GetCubeAccelComponent().RemoveListener(_cubeStateTracker->_id,
                                               _cubeStateTracker->_cubeMovementListener);
  _cubeStateTracker->_cubeMovementListener.reset();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::Update(Robot& robot)
{
  switch(_currentStage){
    case ControllerState::Activated:
    {
      CheckForChargeStateChanges(robot);
      break;
    }
    case ControllerState::DrainCube:
    {
      if(FLT_GT(_cubeStateTracker->_timeStartedDraining_s, 0)){
        UpdateCubeDrain(robot);
      }else{
        StartCubeDrain(robot);
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
      
      PRINT_CH_INFO("Feeding",
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
      _cubeStateTracker->SetChargeState(ChargeState::NoCharge);
      InitializeController(robot);
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
void FeedingCubeController::StartCubeDrain(Robot& robot)
{
  _currentStage = ControllerState::DrainCube;
  _cubeStateTracker->_timeStartedDraining_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::UpdateCubeDrain(Robot& robot)
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // Continuously drain the cube lights as a precentage of the time remaining to drain it
  // until the cube is fully drained
  if((_cubeStateTracker->_timeStartedDraining_s > 0) &&
     (_cubeStateTracker->_timeStartedDraining_s + kTimeToDrainCube > currentTime_s)){
    const float percentRemaining = 1.0f - ( (currentTime_s - _cubeStateTracker->_timeStartedDraining_s) / kTimeToDrainCube);
    
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
  }else if(_cubeStateTracker->_timeStartedDraining_s + kTimeToDrainCube < currentTime_s){
    if(_cubeStateTracker->GetChargeState() != ChargeState::Drained){
      // If the cube has been fully drained re-set the state and de-activate the cube
      _cubeStateTracker->SetChargeState(ChargeState::Drained);
    }
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
              (_currentStage == ControllerState::Activated) &&
              (_cubeStateTracker->GetChargeState() != ChargeState::FullyCharged) &&
              (_cubeStateTracker->GetChargeState() != ChargeState::Drained);
  
  if(cubeCanCharge &&
     (shakeScore > kShakeMinThresh) &&
     (_cubeStateTracker->_timeNextShakeCounts_s < currentTime_s)){
    
    // send updates to the audio system
    if(_cubeStateTracker->_currentChargeLevel > 0){
      // Currently audio has 30 steps while lights only have 16 - so increments
      // and decrements come in pairs
      UpdateChargeAudioRound(robot, ChargeStateChange::Charge_Up);
      UpdateChargeAudioRound(robot, ChargeStateChange::Charge_Up);
      
      PRINT_CH_INFO("Feeding",
                    "FeedingCubeController.ShakeDetected.ShakeIncreasing",
                    "Cube with id %d now has %d shakes",
                    _cubeStateTracker->_id.GetValue(),
                    _cubeStateTracker->_currentChargeLevel);
    }else{
      UpdateChargeAudioRound(robot, ChargeStateChange::Charge_Start);
    }
    
    // Update tracker properties
    _cubeStateTracker->SetChargeState(ChargeState::Charging);
    _cubeStateTracker->_currentChargeLevel++;
    _cubeStateTracker->_timeLoseNextCharge_s   = currentTime_s + kTimeBeforeStartLosingCharge_s;
    _cubeStateTracker->_timeNextShakeCounts_s = currentTime_s + kTimeBetweenShakes_s;
    
    PRINT_CH_INFO("Feeding",
                  "FeedingCubeController.IncrementingShakeCount",
                  "Shake count for active ID %d is now %d",
                  _cubeStateTracker->_id.GetValue(),
                  _cubeStateTracker->_currentChargeLevel);
    
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::SetCubeLights(Robot& robot)
{
  // Remove any lights that are currently set
  if((_cubeStateTracker->_currentAnimationTrigger != _cubeStateTracker->_desiredAnimationTrigger) ||
     _cubeStateTracker->_desiredModifierChanged)
  {
    if(_cubeStateTracker->_desiredAnimationTrigger != CubeAnimationTrigger::Count)
    {
      robot.GetCubeLightComponent().StopAndPlayLightAnim(_cubeStateTracker->_id,
                                                         _cubeStateTracker->_currentAnimationTrigger,
                                                         _cubeStateTracker->_desiredAnimationTrigger,
                                                         nullptr,
                                                         _cubeStateTracker->_desiredModifierChanged,
                                                         _cubeStateTracker->_desiredModifier);
    }
    else
    {
      robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(_cubeStateTracker->_currentAnimationTrigger,
                                                                   _cubeStateTracker->_id);
    }
    
    _cubeStateTracker->_currentAnimationTrigger = _cubeStateTracker->_desiredAnimationTrigger;
    _cubeStateTracker->_desiredModifierChanged = false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingCubeController::UpdateChargeAudioRound(Robot& robot, ChargeStateChange changeEnumVal)
{
  PRINT_CH_INFO("Feeding",
                "FeedingCubeController.UpdateChargeAudioRound.ChargeStateChange",
                "Cube with id %d is broadcasting audio state change %s",
                _cubeStateTracker->_id.GetValue(),
                ChargeStateChangeToString(changeEnumVal));
  
  ExternalInterface::FeedingSFXStageUpdate message;
  message.stage = Util::EnumToUnderlying(changeEnumVal);
  robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::FeedingSFXStageUpdate>(message);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FeedingCubeController::IsCubeCharged() const
{
  return _cubeStateTracker->GetChargeState() == ChargeState::FullyCharged;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FeedingCubeController::IsCubeDrained() const
{
  return _cubeStateTracker->GetChargeState() == ChargeState::Drained;
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

