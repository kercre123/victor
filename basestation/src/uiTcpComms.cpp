/**
 * File: uiTcpComms.cpp
 *
 * Author: Kevin Yoon
 * Created: 7/11/2014
 *
 * Description: Interface class to allow the basestation
 * to accept connections from UI client devices on TCP
 *
 * Copyright: Anki, Inc. 2014
 *
 **/
#include "anki/cozmo/basestation/uiTcpComms.h"

#include "anki/common/basestation/utils/logging/logging.h"
#include "anki/common/basestation/utils/helpers/printByteArray.h"

#include "anki/cozmo/robot/cozmoConfig.h"


namespace Anki {
namespace Cozmo {
  
  const size_t HEADER_SIZE = sizeof(RADIO_PACKET_HEADER);
  
  UiTCPComms::UiTCPComms()
  : isInitialized_(false)
  {
    // Start server
    if (server_.StartListening(UI_MESSAGE_SERVER_LISTEN_PORT)) {
      isInitialized_ = true;
    }
  }
  
  UiTCPComms::~UiTCPComms()
  {
    server_.DisconnectClient();
  }
 
  
  // Returns true if we are ready to use TCP
  bool UiTCPComms::IsInitialized()
  {
    return isInitialized_;
  }
  
  size_t UiTCPComms::Send(const Comms::MsgPacket &p)
  {

    if (HasClient()) {
      
      // Wrap message in header/footer
      // TODO: Include timestamp too?
      char sendBuf[128];
      int sendBufLen = 0;
      memcpy(sendBuf, RADIO_PACKET_HEADER, sizeof(RADIO_PACKET_HEADER));
      sendBufLen += sizeof(RADIO_PACKET_HEADER);
      sendBuf[sendBufLen++] = p.dataLen;
      memcpy(sendBuf + sendBufLen, p.data, p.dataLen);
      sendBufLen += p.dataLen;

      /*
      printf("SENDBUF (hex): ");
      PrintBytesHex(sendBuf, sendBufLen);
      printf("\nSENDBUF (uint): ");
      PrintBytesUInt(sendBuf, sendBufLen);
      printf("\n");
      */
      
      return server_.Send(sendBuf, sendBufLen);
    }
    return -1;
    
  }
  
  
  void UiTCPComms::Update()
  {
    
    // Listen for client if don't already have one.
    if (!server_.HasClient()) {
      if (server_.Accept()) {
        printf("User input device connected!\n");
      }
    }
    
    // Read all messages from all connected robots
    ReadAllMsgPackets();
  
  }
  
  
  void UiTCPComms::PrintRecvBuf()
  {
      for (int i=0; i<recvDataSize;i++){
        u8 t = recvBuf[i];
        printf("0x%x ", t);
      }
      printf("\n");
  }
  
  void UiTCPComms::ReadAllMsgPackets()
  {
    
    // Read from all connected clients.
    // Enqueue complete messages.

    if ( HasClient() ) {
      
      int bytes_recvd = server_.Recv((char*)(recvBuf + recvDataSize), MAX_RECV_BUF_SIZE - recvDataSize);
      if (bytes_recvd == 0) {
        return;
      }
      if (bytes_recvd < 0) {
        // Disconnect client
        printf("uiTcpComms: Recv failed. Disconnecting client\n");
        server_.DisconnectClient();
        return;
      }
      recvDataSize += bytes_recvd;
      
      //printf("Got %d bytes\n", recvDataSize);
      //PrintRecvBuf();
      
      
      
      // Look for valid header
      while (recvDataSize >= sizeof(RADIO_PACKET_HEADER)) {
        
        // Look for 0xBEEF
        u8* hPtr = NULL;
        for(int i = 0; i < recvDataSize-1; ++i) {
          if (recvBuf[i] == RADIO_PACKET_HEADER[0]) {
            if (recvBuf[i+1] == RADIO_PACKET_HEADER[1]) {
              hPtr = &(recvBuf[i]);
              break;
            }
          }
        }
        
        if (hPtr == NULL) {
          // Header not found at all
          // Delete everything
          recvDataSize = 0;
          break;
        }
        
        int n = hPtr - recvBuf;
        if (n != 0) {
          // Header was not found at the beginning.
          // Delete everything up until the header.
          PRINT_NAMED_WARNING("UiTCPComms.PartialMsgRecvd", "Header not found where expected. Dropping preceding %zu bytes\n", n);
          recvDataSize -= n;
          memcpy(recvBuf, hPtr, recvDataSize);
        }
        
        // Check if expected number of bytes are in the msg
        if (recvDataSize > HEADER_SIZE) {
          u32 dataLen = recvBuf[HEADER_SIZE];
          
          if (dataLen > 255) {
            PRINT_NAMED_WARNING("UiTCPComms.MsgTooBig", "Can't handle messages larger than 255\n");
            dataLen = 255;
          }
          
          if (recvDataSize >= HEADER_SIZE + 1 + dataLen) {
            
            recvdMsgPackets_.emplace_back( (s32)(0),  // Source device ID. Not used for anything now so just 0.
                                           (s32)-1,
                                           dataLen,
                                           (u8*)(&recvBuf[HEADER_SIZE+1])
                                          );
            
            // Shift recvBuf contents down
            const u8 entireMsgSize = HEADER_SIZE + 1 + dataLen;
            memcpy(recvBuf, recvBuf + entireMsgSize, recvDataSize - entireMsgSize);
            recvDataSize -= entireMsgSize;
            
          } else {
            break;
          }
        } else {
          break;
        }
        
      } // end while (there are still messages in the recvBuf)
      
    } // end if (HasClient)
  }
  
  
  
  
  
  // Returns true if a MsgPacket was successfully gotten
  bool UiTCPComms::GetNextMsgPacket(Comms::MsgPacket& p)
  {
    if (!recvdMsgPackets_.empty()) {
      p = recvdMsgPackets_.front();
      recvdMsgPackets_.pop_front();
      return true;
    }
    
    return false;
  }
  
  
  u32 UiTCPComms::GetNumPendingMsgPackets()
  {
    return (u32)recvdMsgPackets_.size();
  };
  
  void UiTCPComms::ClearMsgPackets()
  {
    recvdMsgPackets_.clear();
    
  };
  
  
  bool UiTCPComms::HasClient()
  {
    return server_.HasClient();
  }
  
}  // namespace Cozmo
}  // namespace Anki


