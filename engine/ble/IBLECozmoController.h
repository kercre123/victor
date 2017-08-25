/**
* File: IBLECozmoController
*
* Author: Lee Crippen
* Created: 04/26/16
*
* Description:
*
* Copyright: Anki, Inc. 2016
*
**/

#ifndef __Anki_Cozmo_Basestation_Ble_IBLECozmoController_h__
#define __Anki_Cozmo_Basestation_Ble_IBLECozmoController_h__

#include "util/UUID/UUID.h"

#include <cstdint>
#include <stddef.h>

// forward declarations
namespace Anki {
namespace Cozmo {
  class BLECozmoMessage;
}
}
struct UUIDBytes;

namespace Anki {
namespace Cozmo {
  
class IBLECozmoController
{
public:
  virtual ~IBLECozmoController() { }
  
  virtual void StartDiscoveringVehicles() = 0;
  virtual void StopDiscoveringVehicles() = 0;
  virtual void Connect(UUIDBytes vehicleId) = 0;
  virtual void Disconnect(UUIDBytes vehicleId) = 0;
  virtual void SendMessage(UUIDBytes vehicleId, const uint8_t *message, const size_t size) const = 0;
};
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_Ble_IBLECozmoController_h__

