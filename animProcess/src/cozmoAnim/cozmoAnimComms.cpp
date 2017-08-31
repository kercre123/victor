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
      const size_t RECV_BUFFER_SIZE = 1024 * 4;

      // For comms with engine
      UdpServer _server;

      // For comms with robot
      UdpClient _robotClient;
      
      u8 recvBuf_[RECV_BUFFER_SIZE];
      size_t _recvBufSize = 0;
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
    _recvBufSize = 0;
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
        printf("ERROR: Failed to send msg contents (%d bytes sent)\n", bytesSent);
        DisconnectEngine();
        return false;
      }

      return true;
    }
    return false;

  }


//  size_t RadioGetNumBytesAvailable(void)
//  {
//    // Check for incoming data and add it to receive buffer
//    int dataSize;
//
//    // Read available data
//    const size_t tempSize = RECV_BUFFER_SIZE - _recvBufSize;
//    assert(tempSize < std::numeric_limits<int>::max());
//    dataSize = _server.Recv((char*)&recvBuf_[_recvBufSize], static_cast<int>(tempSize));
//    if (dataSize > 0) {
//      _recvBufSize += dataSize;
//    }
//    else if (dataSize < 0) {
//      // Something went wrong
//      DisconnectEngine();
//    }
//
//    return _recvBufSize;
//
//  } // RadioGetNumBytesAvailable()
//
//
//  s32 HAL::RadioPeekChar(u32 offset)
//  {
//    if(RadioGetNumBytesAvailable() <= offset) {
//      return -1;
//    }
//
//    return static_cast<s32>(recvBuf_[offset]);
//  }
//
//  s32 HAL::RadioGetChar(void) { return RadioGetChar(0); }
//
//  s32 HAL::RadioGetChar(u32 timeout)
//  {
//    u8 c;
//    if(RadioGetData(&c, sizeof(u8)) == RESULT_OK) {
//      return static_cast<s32>(c);
//    }
//    else {
//      return -1;
//    }
//  }


  u32 GetNextPacketFromEngine(u8* buffer)
  {
    u32 retVal = 0;

    // Read available datagram
    int dataLen = _server.Recv((char*)recvBuf_, RECV_BUFFER_SIZE);
    if (dataLen > 0) {
      _recvBufSize = dataLen;
    }
    else if (dataLen < 0) {
      // Something went wrong
      DisconnectEngine();
      return retVal;
    }
    else {
      return retVal;
    }

    // Copy message contents to buffer
    std::memcpy((void*)buffer, recvBuf_, dataLen);
    retVal = dataLen;

    return retVal;
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
    u32 retVal = 0;
    
    // Read available datagram
    int dataLen = _robotClient.Recv((char*)buffer, max_length);
    if (dataLen > 0) {
      _recvBufSize = dataLen;
    }
    else if (dataLen < 0) {
      // Something went wrong
      DisconnectRobot();
      return retVal;
    }
    
    return dataLen;
  }

      
  

} // namespace HAL
} // namespace Cozmo
} // namespace Anki
