/*
 * sim_radio.cpp
 *
 *   Implemenation of HAL radio functionality for the simulator.
 *
 *   Author: Andrew Stein
 *
 */

#ifndef SIMULATOR
#error This file (sim_radio.cpp) should not be used without SIMULATOR defined.
#endif

#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include <stdio.h>
#include <string>

#include "anki/messaging/shared/TcpServer.h"
#include "anki/messaging/shared/UdpClient.h"
#include "anki/messaging/shared/utilMessaging.h"

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
    
    namespace { // "Private members"
      const size_t RECV_BUFFER_SIZE = 1024;
      
      // For communications with basestation
      TcpServer server;
      UdpClient advRegClient;

      u8 recvBuf_[RECV_BUFFER_SIZE];
      size_t recvBufSize_ = 0;
      
      RobotAdvertisementRegistration regMsg;
    }

    // Register robot with advertisement service
    void RegisterRobot()
    {
      regMsg.enableAdvertisement = 1;
      
      PRINT("sim_radio: Sending registration for robot %d at address %s on port %d\n", regMsg.robotID, regMsg.robotAddr, regMsg.port);
      advRegClient.Send((char*)&regMsg, sizeof(regMsg));
    }
    
    // Deregister robot with advertisement service
    void DeregisterRobot()
    {
      regMsg.enableAdvertisement = 0;
      
      PRINT("sim_radio: Sending deregistration for robot %d\n", regMsg.robotID);
      advRegClient.Send((char*)&regMsg, sizeof(regMsg));
    }
    
    const char* const HAL::GetLocalIP()
    {
      // Get robot's IPv4 address.
      // Looking for (and assuming there is only one) address that starts with 192.
      struct ifaddrs *ifaddr, *ifa;
      if (getifaddrs(&ifaddr) != 0) {
        PRINT("getifaddrs failed\n");
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
            PRINT("getnameinfo() failed\n");
            assert(false);
          }
          
          // Does address start with 192?
          if (strncmp(host, "192.", 4) == 0)
          {
            PRINT("Local host IP: %s\n", host);
            break;
          }
        }
      }
      freeifaddrs(ifaddr);
      
      return host;
    }
    
    
    Result InitSimRadio(s32 robotID)
    {
      server.StartListening(ROBOT_RADIO_BASE_PORT + robotID);
      
      // Register with advertising service by sending IP and port info
      // NOTE: Since there is no ACK robot_advertisement_controller must be running before this happens!
      //       We also assume that when working with simluated robots on Webots, the advertisement service is running on the same host.
      advRegClient.Connect("127.0.0.1", ROBOT_ADVERTISEMENT_REGISTRATION_PORT);

      
      // Fill in advertisement registration message
      regMsg.robotID = (u8)HAL::GetRobotID();
      regMsg.port = ROBOT_RADIO_BASE_PORT + regMsg.robotID;
      memcpy(regMsg.robotAddr, HAL::GetLocalIP(), sizeof(regMsg.robotAddr));
      
      RegisterRobot();
      recvBufSize_ = 0;
      
      return RESULT_OK;
    }
    
    bool HAL::RadioIsConnected(void)
    {
      return server.HasClient();
    }

    void DisconnectRadio(void)
    {
      server.DisconnectClient();
      recvBufSize_ = 0;
      
      RegisterRobot();
    }
    
    bool HAL::RadioSendMessage(const Messages::ID msgID, const void *buffer, TimeStamp_t ts)
    {
      if (server.HasClient()) {

        // Send the message header (0xBEEF + timestamp + robotID + msgID)
        // For TCP comms, send timestamp immediately after the header.
        // This is needed on the basestation side to properly order messages.
        const u8 HEADER_LENGTH = 11;
        u8 header[HEADER_LENGTH];
        const u8 size = Messages::GetSize(msgID);
        UtilMsgError packRes = SafeUtilMsgPack(header, HEADER_LENGTH, NULL, "cciic",
                    RADIO_PACKET_HEADER[0],
                    RADIO_PACKET_HEADER[1],
                    ts,
                    size + 1,
                    msgID);
        
        assert (packRes == UTILMSG_OK);
        
        // Send header and message content
        u32 bytesSent = 0;
        bytesSent = server.Send((char*)header, HEADER_LENGTH);
        if (bytesSent < HEADER_LENGTH) {
          printf("ERROR: Failed to send header (%d bytes sent)\n", bytesSent);
        }
        bytesSent = server.Send((char*)buffer, size);
        if (bytesSent < size) {
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
      if (!server.HasClient()) {
        return 0;
      }
      
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
    Messages::ID HAL::RadioGetNextMessage(u8 *buffer)
    {
      Messages::ID retVal = Messages::NO_MESSAGE_ID;
      
      if (server.HasClient()) {
        const size_t bytesAvailable = RadioGetNumBytesAvailable();
        const u32 headerSize = sizeof(RADIO_PACKET_HEADER);
        if(bytesAvailable >= headerSize) {
          
          // Look for valid header
          std::string strBuf(recvBuf_, recvBuf_ + recvBufSize_);  // TODO: Just make recvBuf a string
          std::size_t n = strBuf.find((char*)RADIO_PACKET_HEADER, 0, headerSize);
          if (n == std::string::npos) {
            // Header not found at all
            // Delete everything
            recvBufSize_ = 0;
            return retVal;
          } else if (n != 0) {
            // Header was not found at the beginning.
            // Delete everything up until the header.
            strBuf = strBuf.substr(n);
            memcpy(recvBuf_, strBuf.c_str(), strBuf.length());
            recvBufSize_ = strBuf.length();
          }
          
          // Check if expected number of bytes are in the msg
          if (recvBufSize_ > headerSize) {
            const u32 dataLen = recvBuf_[headerSize] +
                                (recvBuf_[headerSize+1] << 8) +
                                (recvBuf_[headerSize+2] << 16) +
                                (recvBuf_[headerSize+3] << 24);
            if (recvBufSize_ >= headerSize + 4 + dataLen) {
            
              // Check that message size is correct
              Messages::ID msgID = static_cast<Messages::ID>(recvBuf_[headerSize+4]);
              const u8 size = Messages::GetSize(msgID);
              const u32 msgLen = dataLen - 1;  // Doesn't include msgID
              
              if (msgLen != size) {
                PRINT("WARNING: Message size mismatch: ID %d, expected %d bytes, but got %d bytes\n", msgID, size, msgLen);
              }
              
              // Copy message contents to buffer
              std::memcpy((void*)buffer, recvBuf_ + headerSize + 4 + 1, msgLen);
              retVal = msgID;
              
              // Shift recvBuf contents down
              const u32 entireMsgSize = headerSize + 4 + 1 + msgLen;
              memcpy(recvBuf_, recvBuf_ + entireMsgSize, recvBufSize_ - entireMsgSize);
              recvBufSize_ -= entireMsgSize;
              
            }
          }
        } // if bytesAvailable > 0
      }
      
      return retVal;
    } // RadioGetNextMessage()
    

    void RadioUpdate()
    {
      if(!server.HasClient()) {
        if (server.Accept()) {
          DeregisterRobot();
        }
      }
    }

    
  } // namespace Cozmo
} // namespace Anki