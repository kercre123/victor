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

#include "anki/cozmo/shared/cozmoConfig.h"
#include "util/logging/logging.h"

#include <assert.h>
#include <stdio.h>
#include <string>

#include "anki/messaging/shared/UdpServer.h"
#include "anki/messaging/shared/UdpClient.h"

#define LOG_CHANNEL    "Robot"
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
  UdpClient _robotClient;
}


  Result InitComms()
  {
    // Setup robot comms
#ifdef SIMULATOR
    const u16 robotID = 1; // TODO: Extract this from proto instance name just like CozmoBot_1?
#else
    const u16 robotID = 0;
#endif
    _robotClient.Connect("127.0.0.1", ROBOT_RADIO_BASE_PORT + robotID);
    
    // Setup engine comms
    const u16 port = ANIM_PROCESS_SERVER_BASE_PORT + robotID;
    LOG_INFO("InitComms.StartListeningPort", "Listen on port %d", port);
    if (!_server.StartListening(port)) {
      LOG_ERROR("InitComms.UDPServerFailed", "Unable to listen on port %d", port);
      assert(false);
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
      if (bytesSent < length) {
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
      if (bytesSent < length) {
        LOG_ERROR("SendPacketToRobot.FailedSend", "Failed to send msg contents (%zd bytes sent)\n", bytesSent);
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
      DisconnectRobot();
      return 0;
    }
    
    return (u32) dataLen;
  }

      
  

} // namespace HAL
} // namespace Cozmo
} // namespace Anki
