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

#include "engine/ble/BLESystem.h"
#ifdef __APPLE__
#include "engine/ble/BLECozmoController_iososx.h"
#elif defined (ANDROID)
#include "engine/ble/BLECozmoController_android.h"
#else
  #define NO_BLE_CONTROLLER
  namespace Anki { namespace Cozmo { class IBLECozmoController{}; } }
#endif
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"

#include "BLECozmoMessage.h"

#include <sstream>
#include <iomanip>

namespace Anki {
namespace Cozmo {


static int UUIDDirectCompare(const UUIDBytes& first, const UUIDBytes& second)
{
  UUIDBytesRef firstRef = const_cast<UUIDBytes* const>(&first);
  UUIDBytesRef secondRef = const_cast<UUIDBytes* const>(&second);
  return UUIDCompare(firstRef, secondRef);
}

BLESystem::BLESystem()
  : _queue(Util::Dispatch::Create("BLESystem"))
  , _bleMessageInProgress(new BLECozmoMessage())
  , _connectedCozmos(UUIDDirectCompare)
{
  #ifndef NO_BLE_CONTROLLER
  _bleCozmoController.reset(new BLECozmoController(this));
  _bleCozmoController->StartDiscoveringVehicles();
  #endif
}
  
BLESystem::~BLESystem()
{
  Util::Dispatch::Stop(_queue);
  Util::Dispatch::Release(_queue);
}
  
void BLESystem::OnVehicleDiscovered(const UUIDBytes& vehicleId)
{
  #ifndef NO_BLE_CONTROLLER
  Util::Dispatch::Async(_queue, [vehicleId, this] {
    PRINT_NAMED_DEBUG("BLESystem.OnVehicleDiscovered", "ID: %s", StringFromUUIDBytes(const_cast<UUIDBytes* const>(&vehicleId)));
    _bleCozmoController->Connect(vehicleId);
  });
  #endif
}

void BLESystem::OnVehicleDisappeared(const UUIDBytes& vehicleId)
{
#if ALLOW_DEBUG_LOGGING
  Util::Dispatch::Async(_queue, [vehicleId] {
    PRINT_NAMED_DEBUG("BLESystem.OnVehicleDisappeared", "ID: %s", StringFromUUIDBytes(const_cast<UUIDBytes* const>(&vehicleId)));
  });
#endif
}

void BLESystem::OnVehicleConnected(const UUIDBytes& vehicleId)
{
  Util::Dispatch::Async(_queue, [vehicleId, this] {
    PRINT_NAMED_DEBUG("BLESystem.OnVehicleConnected", "ID: %s", StringFromUUIDBytes(const_cast<UUIDBytes* const>(&vehicleId)));
    _connectedCozmos.insert(vehicleId);
  });
  
  Util::Dispatch::After(_queue, std::chrono::milliseconds(15000), [vehicleId, this] {
    Anki::Cozmo::LightState bluLight{};
    bluLight.onColor = (uint32_t)Anki::Cozmo::LEDColor::LED_BLUE;
    bluLight.onFrames = bluLight.offFrames = 0x02;
    Anki::Cozmo::LightState redLight{};
    redLight.onColor = (uint32_t)Anki::Cozmo::LEDColor::LED_RED;
    redLight.onFrames = redLight.offFrames = 0x02;

    Anki::Cozmo::RobotInterface::SetBackpackLights lightsData{};
    lightsData.lights[0] = lightsData.lights[2] = bluLight;
    lightsData.lights[1] = redLight;

    Anki::Cozmo::RobotInterface::EngineToRobot robotMsg{};
    robotMsg.Set_setBackpackLights(std::move(lightsData));
    
    SendMessage(vehicleId, robotMsg);
  });
  
  auto robots = GetAvailableRobots();
  for (auto& robot __attribute__((unused)) : robots)
  {
    PRINT_NAMED_DEBUG("BLESystem.OnVehicleConnected.RobotAvailable","%s", StringFromUUIDBytes(&robot));
  }
}

void BLESystem::OnVehicleDisconnected(const UUIDBytes& vehicleId)
{
  Util::Dispatch::Async(_queue, [vehicleId, this] {
    PRINT_NAMED_DEBUG("BLESystem.OnVehicleDisconnected", "ID: %s", StringFromUUIDBytes(const_cast<UUIDBytes* const>(&vehicleId)));
    _connectedCozmos.erase(vehicleId);
  });
}

void BLESystem::OnVehicleMessageReceived(const UUIDBytes& vehicleId, std::vector<uint8_t> messageBytes)
{
  Util::Dispatch::Async(_queue, [vehicleId, messageBytes = std::move(messageBytes), this] {
    (void)vehicleId; // silence compiler warning
    PRINT_NAMED_DEBUG("BLESystem.OnVehicleMessageReceived", "ID: %s message chunk received", StringFromUUIDBytes(const_cast<UUIDBytes* const>(&vehicleId)));
    
    DEV_ASSERT(!_bleMessageInProgress->IsMessageComplete(), "BLESystem.OnVehicleMessageReceived.MessageAlreadyComplete");
    DEV_ASSERT(Anki::Cozmo::BLECozmoMessage::kMessageExactMessageLength == messageBytes.size(), "BLESystem.OnVehicleMessageReceived.MessageChunkIncorrectSize");
    if (!_bleMessageInProgress->AppendChunk(static_cast<const uint8_t*>(messageBytes.data()), (uint32_t) messageBytes.size()))
    {
      PRINT_NAMED_ERROR("BLESystem.OnVehicleMessageReceived.MessageChunkAppendFail", "Unable to append message chunk");
    }
    
    if (_bleMessageInProgress->IsMessageComplete())
    {
      uint8_t completeMessage[Anki::Cozmo::BLECozmoMessage::kMessageMaxTotalSize];
      uint8_t numBytes = _bleMessageInProgress->DechunkifyMessage(completeMessage, Anki::Cozmo::BLECozmoMessage::kMessageMaxTotalSize);
      
      // TODO:(lc) do something with incoming messages besides printing them and clearing
      std::stringstream ss;
      ss << std::hex;
      for (int idx = 0; idx < numBytes; ++idx) {
        ss << completeMessage[idx];
      }
      PRINT_NAMED_DEBUG("BLESystem.OnVehicleMessageReceived",
                        "ID: %s message: %s",
                        StringFromUUIDBytes(const_cast<UUIDBytes* const>(&vehicleId)),
                        ss.str().c_str());
      
      // TODO:(lc) Need to unpack the message into a RobotInterface::RobotMessage type and deal with it.
      _bleMessageInProgress->Clear();
    }
  });
}

void BLESystem::OnVehicleProximityChanged(const UUIDBytes& vehicleId, int rssi, bool isClose)
{
#if ALLOW_DEBUG_LOGGING
  Util::Dispatch::Async(_queue, [vehicleId, rssi] {
    PRINT_NAMED_DEBUG("BLESystem.OnVehicleProximityChanged", "ID: %s RSSI: %d", StringFromUUIDBytes(const_cast<UUIDBytes* const>(&vehicleId)), rssi);
  });
#endif
}

void BLESystem::SendMessage(const UUIDBytes& robotId, const RobotInterface::EngineToRobot& message) const
{
  uint8_t rawMessageBuffer[BLECozmoMessage::kMessageMaxTotalSize];
  uint32_t actualMessageSize = Util::numeric_cast<uint32_t>(message.Pack(rawMessageBuffer, BLECozmoMessage::kMessageMaxTotalSize));
  
  BLECozmoMessage cozmoMessage;
  uint8_t numChunks = cozmoMessage.ChunkifyMessage(rawMessageBuffer, actualMessageSize);
  
  for (int i=0; i < numChunks; i++)
  {
    #ifndef NO_BLE_CONTROLLER
    _bleCozmoController->SendMessage(robotId, cozmoMessage.GetChunkData(i), BLECozmoMessage::kMessageExactMessageLength);
    #endif
  }
}

std::vector<UUIDBytes> BLESystem::GetAvailableRobots() const
{
  std::vector<UUIDBytes> bots;
  Util::Dispatch::Sync(_queue, [this, &bots] () {
    bots.insert(bots.end(), _connectedCozmos.begin(), _connectedCozmos.end());
  });
  return bots;
}

} // namespace Cozmo
} // namespace Anki
