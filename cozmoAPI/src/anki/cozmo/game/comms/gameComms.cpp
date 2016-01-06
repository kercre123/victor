/**
 * File: gameComms.cpp
 *
 * Author: Kevin Yoon
 * Created: 12/16/2014
 *
 * Description: Interface class to allow UI to communicate with game
 *
 * Copyright: Anki, Inc. 2014
 *
 **/
#include "anki/cozmo/game/comms/gameComms.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "util/logging/logging.h"
#include "anki/common/basestation/utils/helpers/printByteArray.h"

// For strcpy
#include <stdio.h>
#include <string.h>

// For getting local host's IP address
#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <unistd.h>


namespace Anki {
namespace Cozmo {
  
  //const size_t HEADER_SIZE = sizeof(RADIO_PACKET_HEADER);
  
  GameComms::GameComms(int deviceID, int serverListenPort, const char* advertisementRegIP, int advertisementRegPort)
  : isInitialized_(false)
  , deviceID_(deviceID)
  , serverListenPort_(serverListenPort)
  , advertisementRegIP_(advertisementRegIP)
  , advertisementRegPort_(advertisementRegPort)
  {
    if (false == server_.StartListening(serverListenPort_)) {
      PRINT_NAMED_ERROR("GameComms.Constructor", "Failed to start listening on port %d\n", serverListenPort_);
    }
  }
 
  
  GameComms::~GameComms()
  {
    DisconnectClient();
  }
 
  
  // Returns true if we are ready to use TCP
  bool GameComms::IsInitialized()
  {
    return isInitialized_;
  }
  
  size_t GameComms::Send(const Comms::MsgPacket &p)
  {

    if (HasClient()) {
      
      // Wrap message in header/footer
      // TODO: Include timestamp too?
      char sendBuf[Comms::MsgPacket::MAX_SIZE];
      int sendBufLen = 0;
      
#if(USE_UDP_UI_COMMS)
      assert(p.dataLen < sizeof(sendBuf));
      memcpy(sendBuf, p.data, p.dataLen);
      sendBufLen = p.dataLen;
#else
      memcpy(sendBuf, RADIO_PACKET_HEADER, sizeof(RADIO_PACKET_HEADER));
      sendBufLen += sizeof(RADIO_PACKET_HEADER);
      sendBuf[sendBufLen++] = p.dataLen;
      sendBuf[sendBufLen++] = p.dataLen >> 8;
      sendBuf[sendBufLen++] = 0;
      sendBuf[sendBufLen++] = 0;
      memcpy(sendBuf + sendBufLen, p.data, p.dataLen);
      sendBufLen += p.dataLen;
#endif
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
  
  u32 GameComms::GetNumMsgPacketsInSendQueue(int devID)
  {
    // TODO: This function isn't used on the game side, sent messages aren't queued anyway, so just returning 0.
    return 0;
  }
  
  void GameComms::Update(bool send_queued_msgs)
  {
    if(!IsInitialized()) {
      // Register with advertisement service
      if (regClient_.Connect(advertisementRegIP_, advertisementRegPort_)) {
        regMsg_.id = deviceID_;
        strcpy((char*)regMsg_.ip, GetLocalIP());
        regMsg_.port = serverListenPort_;
        regMsg_.protocol = USE_UDP_UI_COMMS ? Anki::Comms::UDP : Anki::Comms::TCP;
        
        isInitialized_ = true;
      } else {
        printf("GameComms: waiting to connect to advertisement service...\n");
        return;
      }
    }
    
#if(USE_UDP_UI_COMMS)
    if (!server_.HasClient()) {
      AdvertiseToService();
    }
#else
    if (!server_.HasClient()) {
      if (!server_.Accept()) {
        AdvertiseToService();
      }
    }
#endif
    
    // Read all messages from all connected robots
    ReadAllMsgPackets();
  
  }
  
  
  void GameComms::PrintRecvBuf()
  {
      for (int i=0; i<recvDataSize;i++){
        u8 t = recvBuf[i];
        printf("0x%x ", t);
      }
      printf("\n");
  }
  
  void GameComms::ReadAllMsgPackets()
  {
    
    // Read from all connected clients.
    // Enqueue complete messages.
#if(!USE_UDP_UI_COMMS)
    if ( HasClient() ) {
      
      int bytes_recvd = server_.Recv((char*)(recvBuf + recvDataSize), MAX_RECV_BUF_SIZE - recvDataSize);
      if (bytes_recvd == 0) {
        return;
      }
      if (bytes_recvd < 0) {
        // Disconnect client
        printf("GameComms: Recv failed. Disconnecting client\n");
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
          PRINT_NAMED_WARNING("GameComms.PartialMsgRecvd", "Header not found where expected. Dropping preceding %zu bytes\n", n);
          recvDataSize -= n;
          memcpy(recvBuf, hPtr, recvDataSize);
        }
        
        // Check if expected number of bytes are in the msg
        if (recvDataSize > HEADER_SIZE) {
          u32 dataLen = recvBuf[HEADER_SIZE];
          
          if (dataLen > Comms::MsgPacket::MAX_SIZE) {
            PRINT_NAMED_WARNING("GameComms.MsgTooBig", "Can't handle messages larger than %d\n", Comms::MsgPacket::MAX_SIZE);
            dataLen = Comms::MsgPacket::MAX_SIZE;
          }
          
          if (recvDataSize >= HEADER_SIZE + 4 + dataLen) {
            recvdMsgPackets_.emplace_back( (s32)(0),  // Source device ID. Not used for anything now so just 0.
                                           (s32)-1,
                                           dataLen,
                                           (u8*)(&recvBuf[HEADER_SIZE+4])
                                          );
            
            // Shift recvBuf contents down
            const u8 entireMsgSize = HEADER_SIZE + 4 + dataLen;
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
#else
    
    
    // Process all datagrams
    while( (recvDataSize = server_.Recv((char*)(recvBuf), MAX_RECV_BUF_SIZE)) > 0) {
      recvdMsgPackets_.emplace_back( (s32)(0),  // Source device ID. Not used for anything now so just 0.
                                    (s32)-1,
                                    recvDataSize,
                                    (u8*)(recvBuf));
      
    }
    
    if (recvDataSize < 0) {
      // Disconnect client
      printf("GameComms: Recv failed. Disconnecting client\n");
      server_.DisconnectClient();
    }
    
#endif
    
  }
  
  
  
  
  
  // Returns true if a MsgPacket was successfully gotten
  bool GameComms::GetNextMsgPacket(Comms::MsgPacket& p)
  {
    if (!recvdMsgPackets_.empty()) {
      p = recvdMsgPackets_.front();
      recvdMsgPackets_.pop_front();
      return true;
    }
    
    return false;
  }
  
  
  u32 GameComms::GetNumPendingMsgPackets()
  {
    return (u32)recvdMsgPackets_.size();
  };
  
  void GameComms::ClearMsgPackets()
  {
    recvdMsgPackets_.clear();
    
  };
  
  
  bool GameComms::HasClient()
  {
    return server_.HasClient();
  }
  
  void GameComms::DisconnectClient()
  {
    server_.DisconnectClient();
  }
  
  
  // Register this UI device with advertisement service
  void GameComms::AdvertiseToService()
  {
    regMsg_.enableAdvertisement = 1;
    regMsg_.oneShot = 1;
    
    printf("GameComms: Sending registration for UI device %d at address %s on port %d\n", regMsg_.id, regMsg_.ip, regMsg_.port);
    regClient_.Send((char*)&regMsg_, sizeof(regMsg_));
  }
  
  
  // Hacky function to grab localhost IP address starting with 192.
  const char* const GameComms::GetLocalIP()
  {
    // Get robot's IPv4 address.
    // Looking for (and assuming there is only one) address that starts with 192.
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) != 0) {
      printf("getifaddrs failed\n");
      assert(false);
    }
    
    int family, s, n;
    static char host[NI_MAXHOST];
    for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
      if (ifa->ifa_addr == NULL)
        continue;
      
      family = ifa->ifa_addr->sa_family;
      
      // Display IPv4 addresses only
      if (family == AF_INET) {
        s = getnameinfo(ifa->ifa_addr,
                        (family == AF_INET) ? sizeof(struct sockaddr_in) :
                        sizeof(struct sockaddr_in6),
                        host, NI_MAXHOST,
                        NULL, 0, NI_NUMERICHOST);
        if (s != 0) {
          printf("getnameinfo() failed\n");
          assert(false);
        }
        
        // Does address start with 192?
        if (strncmp(host, "192.", 4) == 0)
        {
          printf("GameComms: Local host IP = %s\n", host);
          break;
        }
        // Does address start with 172? (Cozmo3)
        if (strncmp(host, "172.", 4) == 0)
        {
          printf("GameComms: Local host IP = %s\n", host);
          break;
        }

      }
    }
    freeifaddrs(ifaddr);
    
    return host;
  }

  
}  // namespace Cozmo
}  // namespace Anki


