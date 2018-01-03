/**
 * File: cozmoAnimComms.cpp
 *
 * Author: Kevin Yoon
 * Created: 7/30/2017
 *
 * Description: Create sockets and manages low-level data transfer to engine and robot processes
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "cozmoAnim/cozmoAnimComms.h"
#include "osState/osState.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "util/logging/logging.h"

#include <assert.h>
#include <stdio.h>
#include <string>

#include "coretech/messaging/shared/UdpServer.h"
#include "coretech/messaging/shared/LocalUdpClient.h"

#define LOG_CHANNEL    "Transport"
#define LOG_ERROR      PRINT_NAMED_ERROR
#define LOG_INFO(...)  PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)
#define LOG_DEBUG(...) PRINT_CH_DEBUG(LOG_CHANNEL, ##_VA_ARGS__)

namespace Anki {
namespace Cozmo {
namespace CozmoAnimComms {

namespace { // "Private members"

  // For comms with engine
  UdpServer _server;

  // For comms with robot
  LocalUdpClient _robotClient;
}


  Result InitComms()
  {
    const RobotID_t robotID = OSState::getInstance()->GetRobotID();

    // Setup robot comms
    std::string client_path = std::string(ANIM_CLIENT_PATH) + std::to_string(robotID);
    std::string server_path = std::string(ROBOT_SERVER_PATH) + std::to_string(robotID);
    
    bool ok = _robotClient.Connect(client_path.c_str(), server_path.c_str());
    if (!ok) {
      LOG_ERROR("InitComms.ConnectFailed", "Unable to connect from %s to %s", 
        client_path.c_str(), server_path.c_str());
      return RESULT_FAIL_IO;
    }
    
    // Setup engine comms
    const u16 port = ANIM_PROCESS_SERVER_BASE_PORT + robotID;
    LOG_INFO("InitComms.StartListeningPort", "Listen on port %d", port);
    if (!_server.StartListening(port)) {
      LOG_ERROR("InitComms.UDPServerFailed", "Unable to listen on port %d", port);
      return RESULT_FAIL_IO;
    }
    
    return RESULT_OK;
  }

  bool EngineIsConnected(void)
  {
    return _server.HasClient();
  }


  void DisconnectEngine(void)
  {
    _server.DisconnectClient();
  }

  void UpdateEngineCommsState(u8 wifi)
  {
    if (wifi == 0) { DisconnectEngine(); }
  }

  bool SendPacketToEngine(const void *buffer, const u32 length)
  {
    if (_server.HasClient()) {

      const ssize_t bytesSent = _server.Send((char*)buffer, length);
      if (bytesSent < (ssize_t) length) {
        LOG_ERROR("SendPacketToEngine.FailedSend", "Failed to send msg contents (%zd of %d bytes sent)", bytesSent, length);
        DisconnectEngine();
        return false;
      }

      return true;
    }
    return false;

  }


  u32 GetNextPacketFromEngine(u8* buffer, u32 max_length)
  {
    // Read available datagram
    const ssize_t dataLen = _server.Recv((char*)buffer, max_length);
    if (dataLen < 0) {
      // Something went wrong
      LOG_ERROR("GetNextPacketFromEngine.FailedRecv", "Failed to receive from engine");
      DisconnectEngine();
      return 0;
    }

    return (u32) dataLen;
  }

  
  void DisconnectRobot() {
    _robotClient.Disconnect();
  }

  
  bool SendPacketToRobot(const void *buffer, const u32 length)
  {
    if (_robotClient.IsConnected()) {
      
      ssize_t bytesSent = _robotClient.Send((char*)buffer, length);
      if (bytesSent < (ssize_t) length) {
        LOG_ERROR("SendPacketToRobot.FailedSend", "Failed to send msg contents (%zd bytes sent)", bytesSent);
        DisconnectRobot();
        return false;
      }
      
      return true;
    }
    return false;
    
  }

  
  // TODO: Return s32?
  u32 GetNextPacketFromRobot(u8* buffer, u32 max_length)
  {
    // Read available datagram
    ssize_t dataLen = _robotClient.Recv((char*)buffer, max_length);
    if (dataLen < 0) {
      // Something went wrong
      LOG_ERROR("GetNextPacketFromRobot.FailedRecv", "Failed to receive from robot");
      DisconnectRobot();
      return 0;
    }
    
    return (u32) dataLen;
  }

      
  

} // namespace HAL
} // namespace Cozmo
} // namespace Anki
