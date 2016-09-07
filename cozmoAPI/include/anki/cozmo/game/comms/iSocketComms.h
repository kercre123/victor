/**
 * File: iSocketComms
 *
 * Author: Mark Wesley
 * Created: 05/14/16
 *
 * Description: Interface for any socket-based communications from e.g. Game/SDK to Engine
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Game_Comms_ISocketComms_H__
#define __Cozmo_Game_Comms_ISocketComms_H__

#include "clad/types/uiConnectionTypes.h"
#include "util/stats/recentStatsAccumulator.h"
#include <stddef.h>
#include <vector>


// Forward declarations
namespace Json
{
  class Value;
}

namespace Anki {
namespace Comms {
  class MsgPacket;
}}


namespace Anki {
namespace Cozmo {

namespace ExternalInterface{
  struct Ping;
}
  
class ISocketComms
{
public:
  
  using DeviceId = int;
  static constexpr DeviceId kDeviceIdInvalid = -1;
  
  explicit ISocketComms(bool isEnabled=true);
  virtual ~ISocketComms();
  
  virtual bool Init(UiConnectionType connectionType, const Json::Value& config) = 0;
  
  virtual void Update() = 0;

  // Describes whether an ISocketComms returns messages via a packet or a buffer
  enum class MessageType {
    Packet,
    Buffer
  };
  // describes whether this comms groups messages together in buffers
  // (returning false = every buffer contains exactly one message)
  virtual bool AreMessagesGrouped() const = 0;

  bool SendMessage(const Comms::MsgPacket& msgPacket) { return SendMessageInternal(msgPacket); }
  bool RecvMessage(std::vector<uint8_t>& outBuffer)   { return RecvMessageInternal(outBuffer); }

  virtual bool ConnectToDeviceByID(DeviceId deviceId) = 0;
  virtual bool DisconnectDeviceByID(DeviceId deviceId) = 0;
  
  virtual void GetAdvertisingDeviceIDs(std::vector<ISocketComms::DeviceId>& outDeviceIds) = 0;

  virtual uint32_t GetNumConnectedDevices() const = 0;

  bool HasDesiredDevices() const
  {
    const uint32_t numConnectedDevices = GetNumConnectedDevices();
    return (numConnectedDevices >= _desiredNumDevices) && (numConnectedDevices > 0);
  }

  uint32_t NextPingCounter() { return _pingCounter++; }
  void HandlePingResponse(const ExternalInterface::Ping& message);
  
  const Util::Stats::StatsAccumulator& GetLatencyStats() const { return _latencyStats.GetPrimaryAccumulator(); }
  
  bool IsConnectionEnabled() const { return _isEnabled; }
  void EnableConnection(bool newVal=true)
  {
    bool wasEnabled = _isEnabled;
    _isEnabled = newVal;
    OnEnableConnection(wasEnabled, _isEnabled);
  }
  
protected:
  
  virtual void OnEnableConnection(bool wasEnabled, bool isEnabled) { }
  
  uint32_t GetNumDesiredDevices() const { return _desiredNumDevices; }
  void SetNumDesiredNumDevices(uint32_t newVal) { _desiredNumDevices = newVal; }
  
private:
  
  virtual bool SendMessageInternal(const Comms::MsgPacket& msgPacket) = 0;
  virtual bool RecvMessageInternal(std::vector<uint8_t>& outBuffer) = 0;
  
  Util::Stats::RecentStatsAccumulator _latencyStats;
  uint32_t  _pingCounter;
  uint32_t  _desiredNumDevices = 0;
  bool      _isEnabled;
};
  
  
    
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Game_Comms_ISocketComms_H__

