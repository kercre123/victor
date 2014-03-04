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

namespace Anki {
  namespace Cozmo {
    
    namespace { // "Private members"
      const u16 RECV_BUFFER_SIZE = 1024;
      
      // For communications with basestation
      TcpServer server;
      UdpClient advRegClient;

      u8 recvBuf_[RECV_BUFFER_SIZE];
      s32 recvBufSize_ = 0;
    }

    // Register robot with advertisement service
    void RegisterRobot()
    {
      RobotAdvertisementRegistration regMsg;
      regMsg.robotID = HAL::GetRobotID();
      regMsg.enableAdvertisement = 1;
      regMsg.port = ROBOT_RADIO_BASE_PORT + regMsg.robotID;
      
      PRINT("sim_radio: Sending registration for robot %d on port %d\n", regMsg.robotID, regMsg.port);
      advRegClient.Send((char*)&regMsg, sizeof(regMsg));
    }
    
    // Deregister robot with advertisement service
    void DeregisterRobot()
    {
      RobotAdvertisementRegistration regMsg;
      regMsg.robotID = HAL::GetRobotID();
      regMsg.enableAdvertisement = 0;
      
      PRINT("sim_radio: Sending deregistration for robot %d\n", regMsg.robotID);
      advRegClient.Send((char*)&regMsg, sizeof(regMsg));
    }
    
    
    ReturnCode InitSimRadio(s32 robotID)
    {
      server.StartListening(ROBOT_RADIO_BASE_PORT + robotID);
      
      // Register with advertising service by sending IP and port info
      // NOTE: Since there is no ACK robot_advertisement_controller must be running before this happens!
      advRegClient.Connect("127.0.0.1", ROBOT_ADVERTISEMENT_REGISTRATION_PORT);
      RegisterRobot();
      recvBufSize_ = 0;
      
      return EXIT_SUCCESS;
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
        const u8 HEADER_LENGTH = 7;
        u8 header[HEADER_LENGTH];
        UtilMsgError packRes = UtilMsgPack(header, HEADER_LENGTH, NULL, "ccic",
                    RADIO_PACKET_HEADER[0],
                    RADIO_PACKET_HEADER[1],
                    ts,
                    msgID);
        
        assert (packRes == UTILMSG_OK);
        
        // Send header and message content
        const u8 size = Messages::GetSize(msgID);
        server.Send((char*)header, HEADER_LENGTH);
        server.Send((char*)buffer, size);
        
        // Send footer
        if (server.Send((char*)RADIO_PACKET_FOOTER, sizeof(RADIO_PACKET_FOOTER)) < 0 ) {
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
    
    
    u32 HAL::RadioGetNumBytesAvailable(void)
    {
      if (!server.HasClient())
        return 0;
      
      
      // Check for incoming data and add it to receive buffer
      int dataSize;
      
      // Read available data
      dataSize = server.Recv((char*)&recvBuf_[recvBufSize_], RECV_BUFFER_SIZE - recvBufSize_);
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
      if(RadioGetData(&c, sizeof(u8)) == EXIT_SUCCESS) {
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
        const u32 bytesAvailable = RadioGetNumBytesAvailable();
        if(bytesAvailable > 0) {
    
          const int headerSize = sizeof(RADIO_PACKET_HEADER);
          const int footerSize = sizeof(RADIO_PACKET_FOOTER);
          
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
          
          
          // Look for footer
          n = strBuf.find((char*)RADIO_PACKET_FOOTER, 0, footerSize);
          if (n == std::string::npos) {
            // Footer not found at all
            return retVal;
          } else {
            // Footer was found
            
            // Check that message size is correct
            Messages::ID msgID = static_cast<Messages::ID>(recvBuf_[headerSize]);
            const u8 size = Messages::GetSize(msgID);
            int msgLen = n - headerSize - 1;  // Doesn't include msgID
            
            if (msgLen != size) {
              PRINT("WARNING: Message size mismatch: ID %d, expected %d bytes, but got %d bytes\n", msgID, size, msgLen);
            }
            
            // Copy message contents to buffer
            std::memcpy((void*)buffer, recvBuf_ + headerSize + 1, msgLen);
            retVal = msgID;
            
            // Shift recvBuf contents down
            memcpy(recvBuf_, recvBuf_ + n + footerSize, recvBufSize_ - 1 - msgLen - headerSize - footerSize);
            recvBufSize_ -= headerSize + 1 + msgLen + footerSize;
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