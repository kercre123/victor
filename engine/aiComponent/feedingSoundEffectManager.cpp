/**
* File: feedingSoundEffectManager.cpp
*
* Author: Kevin M. Karol
* Created: 09/27/17
*
* Description: Coordinates sound effect events across various feeding
* cubes/activities etc
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/feedingSoundEffectManager.h"

#include "engine/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/helpers/templateHelpers.h"


namespace Anki{
namespace Cozmo{
  
namespace{
const uint8_t kInvalidMessageSTage = -1;
const float kSFXLevelsToFillCube = 30.f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FeedingSoundEffectManager::FeedingSoundEffectManager()
: _dominantObject()
, _dominantChargeLevel(0)
, _dominantChargeChange(ChargeStateChange::Charge_Stop)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingSoundEffectManager::NotifyChargeStateChange(IExternalInterface& externalInterface,
                                                        const ObjectID& objID, int currentChargeLevel,
                                                        ChargeStateChange changeEnumVal)
{
  ExternalInterface::FeedingSFXStageUpdate message;
  message.stage = kInvalidMessageSTage;
  
  bool updateDominantProperties = true;
  // If the dominant object is sending more messages, send the state changes up directly
  // otherwise, determine whether the new object has become dominant or its messages
  // should be ignored
  if(objID == _dominantObject){
    message.stage = Util::EnumToUnderlying(changeEnumVal);
    message.chargePercentage = currentChargeLevel/kSFXLevelsToFillCube;
    externalInterface.BroadcastToGame<ExternalInterface::FeedingSFXStageUpdate>(message);
    updateDominantProperties = true;
  }else{
    // Reset to listening for the new charge
    if((_dominantChargeChange != ChargeStateChange::Charge_Up) &&
       (changeEnumVal == ChargeStateChange::Charge_Start)){
      ResetChargeSound(externalInterface);
    }
    // Slur the sound effect to the appropriate level of the cube that's still
    // charging
    else if((_dominantChargeChange != ChargeStateChange::Charge_Up) &&
       (changeEnumVal == ChargeStateChange::Charge_Up)){
      ResetChargeSound(externalInterface);
      message.stage = Util::EnumToUnderlying(ChargeStateChange::Charge_Up);
      message.chargePercentage = currentChargeLevel/kSFXLevelsToFillCube;
      for(int i = 0; i < currentChargeLevel; i++){
        externalInterface.BroadcastToGame<ExternalInterface::FeedingSFXStageUpdate>(message);
      }
      message.stage = kInvalidMessageSTage;
    }
    
    else if((_dominantChargeChange == ChargeStateChange::Charge_Up) &&
            (changeEnumVal == ChargeStateChange::Charge_Up) &&
            (currentChargeLevel > _dominantChargeLevel)){
      // Update dominant properties and send the charge up message as normal
      // this is default behavior
    }
    else{
      updateDominantProperties = false;
    }
    
  }
  
  if(updateDominantProperties){
    _dominantObject = objID;
    _dominantChargeLevel = currentChargeLevel;
    _dominantChargeChange = changeEnumVal;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FeedingSoundEffectManager::ResetChargeSound(IExternalInterface& externalInterface)
{
  ExternalInterface::FeedingSFXStageUpdate message;
  message.stage = Util::EnumToUnderlying(ChargeStateChange::Charge_Stop);
  externalInterface.BroadcastToGame<ExternalInterface::FeedingSFXStageUpdate>(message);
  ExternalInterface::FeedingSFXStageUpdate message2;
  message2.stage = Util::EnumToUnderlying(ChargeStateChange::Charge_Start);
  externalInterface.BroadcastToGame<ExternalInterface::FeedingSFXStageUpdate>(message2);
  
}


} // namespace Cozmo
} // namespace Anki
