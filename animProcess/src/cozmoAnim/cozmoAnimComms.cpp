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
#include <assert.h>
#include <stdio.h>
#include <string>

#include "anki/messaging/shared/UdpServer.h"
#include "anki/messaging/shared/UdpClient.h"


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
    int robotID = 1; // TODO: Extract this from proto instance name just like CozmoBot_1?
#else
    int robotID = 0;
#endif
    _robotClient.Connect("127.0.0.1", ROBOT_RADIO_BASE_PORT + robotID);
    
    // Setup engine comms
    printf("InitComms.StartListeningPort: %d\n", ANIM_PROCESS_SERVER_BASE_PORT + robotID);
    if (!_server.StartListening(ANIM_PROCESS_SERVER_BASE_PORT + robotID)) {
      printf("InitComms.UDPServerFailed\n");
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

      u32 bytesSent = _server.Send((char*)buffer, length);
      if (bytesSent < length) {
        printf("ERROR: Failed to send msg contents (%d of %d bytes sent)\n", bytesSent, length);
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
    int dataLen = _server.Recv((char*)buffer, max_length);
    if (dataLen < 0) {
      // Something went wrong
      DisconnectEngine();
      return 0;
    }

    return dataLen;
  }

  
  void DisconnectRobot() {
    _robotClient.Disconnect();
  }

  
  bool SendPacketToRobot(const void *buffer, const u32 length)
  {
    if (_robotClient.IsConnected()) {
      
      u32 bytesSent = _robotClient.Send((char*)buffer, length);
      if (bytesSent < length) {
        printf("ERROR: Failed to send msg contents (%d bytes sent)\n", bytesSent);
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
    int dataLen = _robotClient.Recv((char*)buffer, max_length);
    if (dataLen < 0) {
      // Something went wrong
      DisconnectRobot();
      return 0;
    }
    
    return dataLen;
  }

      
  

} // namespace HAL
} // namespace Cozmo
} // namespace Anki
