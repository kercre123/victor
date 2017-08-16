/**
* File: IBLECozmoResponder
*
* Author: Lee Crippen
* Created: 04/26/16
*
* Description:
*
* Copyright: Anki, Inc. 2016
*
**/

#ifndef __Anki_Cozmo_Basestation_Ble_IBLECozmoResponder_h__
#define __Anki_Cozmo_Basestation_Ble_IBLECozmoResponder_h__

#include "util/UUID/UUID.h"
#include <cstdint>
#include <vector>

// forward declarations
namespace Anki {
namespace Cozmo {
  class BLECozmoMessage;
}
}
struct UUIDBytes;

namespace Anki {
namespace Cozmo {
  
class IBLECozmoResponder
{
public:
  virtual ~IBLECozmoResponder() { }
  
  virtual void OnVehicleDiscovered(const UUIDBytes& vehicleId) = 0;
  virtual void OnVehicleDisappeared(const UUIDBytes& vehicleId) = 0;
  virtual void OnVehicleConnected(const UUIDBytes& vehicleId) = 0;
  virtual void OnVehicleDisconnected(const UUIDBytes& vehicleId) = 0;
  virtual void OnVehicleMessageReceived(const UUIDBytes& vehicleId, std::vector<uint8_t> messageBytes) = 0;
  virtual void OnVehicleProximityChanged(const UUIDBytes& vehicleId, int rssi, bool isClose) = 0;
};
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_Ble_IBLECozmoResponder_h__

