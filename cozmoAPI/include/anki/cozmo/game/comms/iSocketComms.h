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
  class MessageGameToEngine;
}
  
class ISocketComms
{
public:
  
  using DeviceId = int;
  static constexpr DeviceId kDeviceIdInvalid = -1;
  
  ISocketComms(UiConnectionType connectionType);
  virtual ~ISocketComms();
  
  virtual bool Init(UiConnectionType connectionType, const Json::Value& config) = 0;
  
  virtual void Update() = 0;
  
  virtual bool SendMessage(const Comms::MsgPacket& msgPacket) = 0;
  virtual bool RecvMessage(Comms::MsgPacket& outMsgPacket) = 0;
  
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
  void HandlePingResponse(const ExternalInterface::MessageGameToEngine& message);
  
private:
  Util::Stats::RecentStatsAccumulator _latencyStats;
  
protected:
  uint32_t  _pingCounter;
  uint32_t  _desiredNumDevices = 0;
};
  
  
    
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Game_Comms_ISocketComms_H__

