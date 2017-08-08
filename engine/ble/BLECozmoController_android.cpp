/**
* File: BLECozmoController_android
*
* Author: Lee Crippen
* Created: 04/26/16
*
* Description:
*
* Copyright: Anki, Inc. 2016
*
**/

#include "engine/ble/BLECozmoController_android.h"
#include "engine/ble/BLESystem.h"
#include "engine/ble/IBLECozmoResponder.h"


namespace Anki {
namespace Cozmo {
  
struct BLECozmoControllerImpl
{
};
  
  
BLECozmoController::BLECozmoController(IBLECozmoResponder* bleResponder)
  : _impl(new BLECozmoControllerImpl{})
{

}
  
BLECozmoController::~BLECozmoController() = default;
  

void BLECozmoController::StartDiscoveringVehicles()
{
  // TODO: implement me
}

void BLECozmoController::StopDiscoveringVehicles()
{
  // TODO: implement me
}

void BLECozmoController::Connect(UUIDBytes vehicleId)
{
  // TODO: implement me
}

void BLECozmoController::Disconnect(UUIDBytes vehicleId)
{
  // TODO: implement me
}

void BLECozmoController::SendMessage(UUIDBytes vehicleId, const uint8_t *message, const size_t size) const
{
  // TODO: implement me
}
  
} // namespace Cozmo
} // namespace Anki
