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
#include "anki/cozmo/robot/hal.h"
#include <assert.h>
#include <stdio.h>
#include <string>

#include "anki/messaging/shared/UdpServer.h"

#define ARRAY_SIZE(inArray)   (sizeof(inArray) / sizeof((inArray)[0]))


namespace Anki {
  namespace Cozmo {
    
    namespace { // "Private members"
      const size_t RECV_BUFFER_SIZE = 1024 * 4;

      // For communications with basestation
      UdpServer server;

      u8 recvBuf_[RECV_BUFFER_SIZE];
      size_t recvBufSize_ = 0;
    }


    Result InitRadio(const char* advertisementIP)
    {
      printf("HAL.InitRadio.StartListeningPort: %d\n", ROBOT_RADIO_BASE_PORT);
      if (!server.StartListening(ROBOT_RADIO_BASE_PORT)) {
        printf("HAL.InitRadio.UDPServerFailed\n");
        assert(false);
      }

      return RESULT_OK;
    }

    bool HAL::RadioIsConnected(void)
    {
      return server.HasClient();
    }


    void HAL::RadioUpdateState(u8 wifi)
    {
      if (wifi == 0) DisconnectRadio();
    }

    void HAL::DisconnectRadio(void)
    {
      server.DisconnectClient();
      recvBufSize_ = 0;
    }

    bool HAL::RadioSendPacket(const void *buffer, const u32 length, u8 socket)
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
      } else if (dataSize < 0) {
        // Something went wrong
        HAL::DisconnectRadio();
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
    u32 HAL::RadioGetNextPacket(u8* buffer)
    {
      u32 retVal = 0;

      // Read available datagram
      int dataLen = server.Recv((char*)recvBuf_, RECV_BUFFER_SIZE);
      if (dataLen > 0) {
        recvBufSize_ = dataLen;
      } else if (dataLen < 0) {
        // Something went wrong
        DisconnectRadio();
        return retVal;
      } else {
        return retVal;
      }

      // Copy message contents to buffer
      std::memcpy((void*)buffer, recvBuf_, dataLen);
      retVal = dataLen;

      return retVal;
    } // RadioGetNextMessage()



  } // namespace Cozmo
} // namespace Anki
