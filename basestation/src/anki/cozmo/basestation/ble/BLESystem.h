/**
* File: BLESystem
*
* Author: Lee Crippen
* Created: 04/26/16
*
* Description:
*
* Copyright: Anki, Inc. 2016
*
**/

#ifndef __Anki_Cozmo_Basestation_Ble_BLESystem_h__
#define __Anki_Cozmo_Basestation_Ble_BLESystem_h__

#include "anki/cozmo/basestation/ble/IBLECozmoResponder.h"
#include "util/helpers/noncopyable.h"
#include "util/UUID/UUID.h"

#include <set>
#include <vector>
#include <memory>

// Forward declarations
namespace Anki {
namespace Util {
namespace Dispatch {
  class Queue;
} // namespatch Dispatch
} // namespace Util
  
namespace Cozmo {
  class IBLECozmoController;
  class BLECozmoMessage;
  
  namespace RobotInterface {
    class EngineToRobot;
  }
} // namespace Cozmo
} // namespace Anki

namespace Anki {
namespace Cozmo {
  
class BLESystem : private Util::noncopyable, public IBLECozmoResponder
{
public:
  explicit BLESystem();
  ~BLESystem();
  
  enum class State
  {
    Listening,
    Connecting,
    Connected,
  };
  
  void Update();
  
  virtual void OnVehicleDiscovered(const UUIDBytes& vehicleId) override;
  virtual void OnVehicleDisappeared(const UUIDBytes& vehicleId) override;
  virtual void OnVehicleConnected(const UUIDBytes& vehicleId) override;
  virtual void OnVehicleDisconnected(const UUIDBytes& vehicleId) override;
  virtual void OnVehicleMessageReceived(const UUIDBytes& vehicleId, std::vector<uint8_t> messageBytes) override;
  virtual void OnVehicleProximityChanged(const UUIDBytes& vehicleId, int rssi, bool isClose) override;
  
  void SendMessage(const UUIDBytes& robotId, const RobotInterface::EngineToRobot& message) const;
  std::vector<UUIDBytes> GetAvailableRobots() const;
  
private:
  State _currentState = State::Listening;
  Util::Dispatch::Queue* _queue = nullptr;
  std::unique_ptr<IBLECozmoController> _bleCozmoController;
  std::unique_ptr<BLECozmoMessage> _bleMessageInProgress;
  
  using UUIDCompareFuncType = int(*)(const UUIDBytes&, const UUIDBytes&);
  std::set<UUIDBytes, UUIDCompareFuncType> _connectedCozmos;
  
}; // class BLESystem

  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_Ble_BLESystem_h__