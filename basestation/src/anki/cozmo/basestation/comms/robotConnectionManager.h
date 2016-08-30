/**
* File: RobotConnectionManager
*
* Author: Lee Crippen
* Created: 7/6/2016
*
* Description: Holds onto current RobotConnections
*
* Copyright: Anki, inc. 2016
*
*/
#ifndef __Cozmo_Basestation_Comms_RobotConnectionManager_H_
#define __Cozmo_Basestation_Comms_RobotConnectionManager_H_

#include "anki/cozmo/basestation/comms/robotConnectionMessageData.h"
#include "util/stats/recentStatsAccumulator.h"

#include <memory>
#include <deque>

namespace Anki {
namespace Util {
  class TransportAddress;
  class UDPTransport;
  class ReliableTransport;
}
}

namespace Anki {
namespace Cozmo {

class RobotManager;
class RobotConnectionData;

class RobotConnectionManager {
public:
  RobotConnectionManager(RobotManager* robotManager);
  virtual ~RobotConnectionManager();
  
  void Init();
  void ConfigureReliableTransport();
  
  bool IsValidConnection() const;
  void Connect(const Util::TransportAddress& address);
  void DisconnectCurrent();
  
  void Update();
  void ProcessArrivedMessages();
  
  bool SendData(const uint8_t* buffer, unsigned int size);
  
  bool PopData(std::vector<uint8_t>& data_out);
  void ClearData();
  
  const Anki::Util::Stats::StatsAccumulator& GetQueuedTimes_ms() const;
  
  void SetReliableTransportRunMode(bool isSync);
  
private:
  std::unique_ptr<RobotConnectionData>      _currentConnectionData;
  std::unique_ptr<Util::UDPTransport>       _udpTransport;
  std::unique_ptr<Util::ReliableTransport>  _reliableTransport;
  RobotManager*                             _robotManager = nullptr;
  std::deque<std::vector<uint8_t>>          _readyData;
  
#if TRACK_INCOMING_PACKET_LATENCY
  Util::Stats::RecentStatsAccumulator _queuedTimes_ms = 100; // how many ms between packet arriving and it being passed onto game
#endif // TRACK_INCOMING_PACKET_LATENCY
  
  void HandleDataMessage(RobotConnectionMessageData& nextMessage);
  void HandleConnectionResponseMessage(RobotConnectionMessageData& nextMessage);
  void HandleDisconnectMessage(RobotConnectionMessageData& nextMessage);
  void HandleConnectionRequestMessage(RobotConnectionMessageData& nextMessage);
};

} // end namespace Cozmo
} // end namespace Anki


#endif //__Cozmo_Basestation_Comms_RobotConnectionManager_H_
