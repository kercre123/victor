/**
* File: BLECozmoController_ios
*
* Author: Lee Crippen
* Created: 04/26/16
*
* Description:
*
* Copyright: Anki, Inc. 2016
*
**/

#ifndef __Anki_Cozmo_Basestation_Ble_BLECozmoController_ios_h__
#define __Anki_Cozmo_Basestation_Ble_BLECozmoController_ios_h__

#include "engine/ble/IBLECozmoController.h"

#include <memory>

namespace Anki {
namespace Cozmo {
  
struct BLECozmoControllerImpl;
class IBLECozmoResponder;
  
class BLECozmoController : public IBLECozmoController
{
public:
  BLECozmoController(IBLECozmoResponder* bleResponder);
  virtual ~BLECozmoController();
  
  virtual void StartDiscoveringVehicles() override;
  virtual void StopDiscoveringVehicles() override;
  virtual void Connect(UUIDBytes vehicleId) override;
  virtual void Disconnect(UUIDBytes vehicleId) override;
  virtual void SendMessage(UUIDBytes vehicleId, const uint8_t *message, const size_t size) const override;
  
private:
  std::unique_ptr<BLECozmoControllerImpl>     _impl;
};
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_Ble_BLECozmoController_ios_h__

