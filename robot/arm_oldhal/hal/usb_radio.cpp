/*
 * usb_radio.cpp
 *
 *   Implemenation of HAL radio functionality that actually uses UART.
 *   To be used in conjunction with CozmoCommsTranslator.
 *
 *   Use this OR radio.cpp. Not both!
 *
 *   Author: Kevin Yoon
 *
 */

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"

#include "anki/messaging/shared/utilMessaging.h"

// Hack to prevent linker error
// Once the intertwining between HAL and supervisor is fixed, this won't be required
//#define RUN_EMBEDDED_TESTS

namespace Anki {
  namespace Cozmo {

    namespace { // "Private members"
      const u16 RECV_BUFFER_SIZE = 2048;

      u8 recvBuf_[RECV_BUFFER_SIZE];
      s32 recvBufSize_ = 0;

      u8 somWifiState = 0;
      u8 somBlueState = 0;
    }

    Result InitSimRadio(s32 robotID)
    {
      return RESULT_OK;
    }

    void HAL::RadioUpdateState(u8 wifi, u8 blue)
    {
      if (wifi == 0 && blue == 0)
      {
        DisconnectRadio();
      }
      else
      {
        somWifiState = wifi;
        somBlueState = blue;
      }
    }

    bool HAL::RadioIsConnected(void)
    {
      // Always assumes radio is connected
      //return true;

      // 2.0 version
      //return HAL::WifiHasClient();

      if (somBlueState != 0) return true;
      if (somWifiState != 0) return true;
      return false;
    }

    void HAL::DisconnectRadio(void)
    {
      somBlueState = 0;
      somWifiState = 0;
      recvBufSize_ = 0;
    }

#ifndef RUN_EMBEDDED_TESTS
    bool HAL::RadioSendPacket(const void *buffer, const u32 length, const u8 socket)
    {
#if(USING_UART_RADIO)
        return UARTPutPacket((u8*)buffer, length, socket);
#else
        return true;
#endif

    } // RadioSendPacket()
#endif // #ifndef RUN_EMBEDDED_TESTS

    u32 RadioGetNumBytesAvailable(void)
    {
#if(USING_UART_RADIO)
      // Pull as many inbound chars as we can into our local buffer
      while (recvBufSize_ < RECV_BUFFER_SIZE)
      {
        int c = HAL::UARTGetChar(0);
        if (c < 0)    // Nothing more to grab
          return recvBufSize_;
        recvBuf_[recvBufSize_++] = c;
      }
#endif
      return recvBufSize_;

    } // RadioGetNumBytesAvailable()


#ifndef RUN_EMBEDDED_TESTS
    // TODO: would be nice to implement this in a way that is not specific to
    //       hardware vs. simulated radio receivers, and just calls lower-level
    //       radio functions.
    u32 HAL::RadioGetNextPacket(u8 *buffer)
    {
      u32 retVal = 0;

#if(USING_UART_RADIO)
//      if (server.HasClient()) {
        const u32 bytesAvailable = RadioGetNumBytesAvailable();
        if(bytesAvailable > 0) {

          const int headerSize = sizeof(RADIO_PACKET_HEADER);

          // Look for valid header
          u8* hPtr = NULL;
          for(int i = 0; i < recvBufSize_-1; ++i) {
            if (recvBuf_[i] == RADIO_PACKET_HEADER[0]) {
              if (recvBuf_[i+1] == RADIO_PACKET_HEADER[1]) {
                hPtr = &(recvBuf_[i]);
                break;
              }
            }
          }

          if (hPtr == NULL) {
            // Header not found at all
            // Delete everything
            recvBufSize_ = 0;
            return retVal;
          }

          const s32 n = (s32)hPtr - (s32)recvBuf_;
          if (n != 0) {
            // Header was not found at the beginning.
            // Delete everything up until the header.
            recvBufSize_ -= n;
            memcpy(recvBuf_, hPtr, recvBufSize_);
          }

          // Check if expected number of bytes are in the packet
          if (recvBufSize_ > headerSize) {
            u32 dataLen = recvBuf_[headerSize] +
                          (recvBuf_[headerSize+1] << 8) +
                          (recvBuf_[headerSize+2] << 16) +
                          (recvBuf_[headerSize+3] << 24);

            if (recvBufSize_ >= headerSize + 4 + dataLen) {

              // Copy message contents to buffer
              std::memcpy((void*)buffer, recvBuf_ + headerSize + 4, dataLen);
              retVal = dataLen;

              // Shift recvBuf contents down
              const u32 entireMsgSize = headerSize + 4 + dataLen;
              memcpy(recvBuf_, recvBuf_ + entireMsgSize, recvBufSize_ - entireMsgSize);
              recvBufSize_ -= entireMsgSize;

            }
          }

        } // if bytesAvailable > 0
//      }
#endif
      return retVal;
    } // RadioGetNextMessage()
#endif // #ifndef RUN_EMBEDDED_TESTS

    void RadioUpdate()
    {
    }
  } // namespace Cozmo
} // namespace Anki
