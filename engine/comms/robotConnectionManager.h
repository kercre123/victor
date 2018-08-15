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
#ifndef __Cozmo_Engine_Comms_RobotConnectionManager_H_
#define __Cozmo_Engine_Comms_RobotConnectionManager_H_

#include "engine/comms/robotConnectionMessageData.h"
#include "coretech/messaging/shared/LocalUdpClient.h"
#include "util/stats/recentStatsAccumulator.h"
#include "util/signals/signalHolder.h"

#include <memory>
#include <deque>

namespace Anki {
namespace Vector {

class RobotManager;
class RobotConnectionData;

class RobotConnectionManager : private Util::SignalHolder {
public:
  RobotConnectionManager(RobotManager* robotManager);
  virtual ~RobotConnectionManager();
  
  void Init();

  bool IsValidConnection() const;

  void DisconnectCurrent();
  
  Result Update();

  void ProcessArrivedMessages();
  
  bool SendData(const uint8_t* buffer, unsigned int size);
  
  bool PopData(std::vector<uint8_t>& data_out);

  void ClearData();
  
  const Anki::Util::Stats::StatsAccumulator& GetQueuedTimes_ms() const;

  Result Connect(RobotID_t robotID);

  bool IsConnected(RobotID_t robotID) const;

private:
  void SendAndResetQueueStats();
  
  void HandleDataMessage(RobotConnectionMessageData& nextMessage);
  void HandleConnectionResponseMessage(RobotConnectionMessageData& nextMessage);
  void HandleDisconnectMessage(RobotConnectionMessageData& nextMessage);
  void HandleConnectionRequestMessage(RobotConnectionMessageData& nextMessage);

  std::unique_ptr<RobotConnectionData>      _currentConnectionData;
  RobotManager*                             _robotManager = nullptr;
  std::deque<std::vector<uint8_t>>          _readyData;
  
#if TRACK_INCOMING_PACKET_LATENCY
  Util::Stats::RecentStatsAccumulator _queuedTimes_ms = 100; // how many ms between packet arriving and it being passed onto game
#endif // TRACK_INCOMING_PACKET_LATENCY

  // track how large the incoming message queue gets in bytes
  Util::Stats::StatsAccumulator _queueSizeAccumulator;

  RobotID_t      _robotID = -1;
  LocalUdpClient _udpClient;
};

} // end namespace Vector
} // end namespace Anki


#endif //__Cozmo_Engine_Comms_RobotConnectionManager_H_
