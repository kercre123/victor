/*
 * radio.cpp
 *
 *   Implemenation of HAL "radio" functionality for Cozmo V2
 *   This is actually the robot interface to the engine
 *
 *   Author: Andrew Stein
 *
 */

#include "anki/cozmo/shared/cozmoConfig.h"
#include <assert.h>
#include <stdio.h>
#include <string>

#include "anki/messaging/shared/UdpServer.h"
#include "anki/messaging/shared/UdpClient.h"

// TODO: Fix underscore


namespace Anki {
namespace Cozmo {
namespace HAL {

    namespace { // "Private members"
      const size_t RECV_BUFFER_SIZE = 1024 * 4;

      // For comms with engine
      UdpServer server;

      // For comms with robot
      UdpClient _robotClient;
      
      u8 recvBuf_[RECV_BUFFER_SIZE];
      size_t recvBufSize_ = 0;
    }


    Result InitRadio()
    {
      // Setup robot comms
#ifdef SIMULATOR
      int robotID = 1; // TODO: Extract this from proto instance name just like CozmoBot_1?
#else
      int robotID = 0;
#endif
      _robotClient.Connect("127.0.0.1", ROBOT_RADIO_BASE_PORT + robotID);
     
      
      // Setup engine comms
      printf("HAL.InitRadio.StartListeningPort: %d\n", ANIM_PROCESS_SERVER_BASE_PORT + robotID);
      if (!server.StartListening(ANIM_PROCESS_SERVER_BASE_PORT + robotID)) {
        printf("HAL.InitRadio.UDPServerFailed\n");
        assert(false);
      }
      

      return RESULT_OK;
    }

    bool RadioIsConnected(void)
    {
      return server.HasClient();
    }

    
    void DisconnectRadio(void)
    {
      server.DisconnectClient();
      recvBufSize_ = 0;
    }

    void RadioUpdateState(u8 wifi)
    {
      if (wifi == 0) { DisconnectRadio(); }
    }

    bool RadioSendPacket(const void *buffer, const u32 length, u8 socket)
    {
      (void)socket;

      if (server.HasClient()) {

        u32 bytesSent = server.Send((char*)buffer, length);
        if (bytesSent < length) {
          printf("ERROR: Failed to send msg contents (%d bytes sent)\n", bytesSent);
          DisconnectRadio();
          return false;
        }

        /*
        printf("SENT: ");
        for (int i=0; i<HEADER_LENGTH;i++){
          u8 t = header[i];
          printf("0x%x ", t);
        }
        for (int i=0; i<size;i++){
          u8 t = ((char*)buffer)[i];
          printf("0x%x ", t);
        }
        for (int i=0; i<sizeof(RADIO_PACKET_FOOTER);i++){
          u8 t = RADIO_PACKET_FOOTER[i];
          printf("0x%x ", t);
        }
        printf("\n");
        */

        return true;
      }
      return false;

    } // RadioSendMessage()


    size_t RadioGetNumBytesAvailable(void)
    {
      // Check for incoming data and add it to receive buffer
      int dataSize;

      // Read available data
      const size_t tempSize = RECV_BUFFER_SIZE - recvBufSize_;
      assert(tempSize < std::numeric_limits<int>::max());
      dataSize = server.Recv((char*)&recvBuf_[recvBufSize_], static_cast<int>(tempSize));
      if (dataSize > 0) {
        recvBufSize_ += dataSize;
      }
      else if (dataSize < 0) {
        // Something went wrong
        DisconnectRadio();
      }

      return recvBufSize_;

    } // RadioGetNumBytesAvailable()

    /*
    s32 HAL::RadioPeekChar(u32 offset)
    {
      if(RadioGetNumBytesAvailable() <= offset) {
        return -1;
      }

      return static_cast<s32>(recvBuf_[offset]);
    }

    s32 HAL::RadioGetChar(void) { return RadioGetChar(0); }

    s32 HAL::RadioGetChar(u32 timeout)
    {
      u8 c;
      if(RadioGetData(&c, sizeof(u8)) == RESULT_OK) {
        return static_cast<s32>(c);
      }
      else {
        return -1;
      }
    }
     */

    // TODO: would be nice to implement this in a way that is not specific to
    //       hardware vs. simulated radio receivers, and just calls lower-level
    //       radio functions.
    u32 RadioGetNextPacket(u8* buffer)
    {
      u32 retVal = 0;

      // Read available datagram
      int dataLen = server.Recv((char*)recvBuf_, RECV_BUFFER_SIZE);
      if (dataLen > 0) {
        recvBufSize_ = dataLen;
      }
      else if (dataLen < 0) {
        // Something went wrong
        DisconnectRadio();
        return retVal;
      }
      else {
        return retVal;
      }

      // Copy message contents to buffer
      std::memcpy((void*)buffer, recvBuf_, dataLen);
      retVal = dataLen;

      return retVal;
    } // RadioGetNextMessage()
  
  // ======================================
  
  
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
    
  } // RadioSendMessage()

  
  // TODO: Return s32?
  u32 GetNextPacketFromRobot(u8* buffer, u32 max_length)
  {
    u32 retVal = 0;
    
    // Read available datagram
    int dataLen = _robotClient.Recv((char*)buffer, max_length);
    if (dataLen > 0) {
      recvBufSize_ = dataLen;
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
