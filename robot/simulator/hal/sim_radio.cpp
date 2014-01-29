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

#if(USE_WEBOTS_TXRX)
#include <webots/Supervisor.hpp>
#else
#include "anki/messaging/shared/TcpServer.h"
#include "anki/messaging/shared/UdpClient.h"
#include "anki/messaging/shared/utilMessaging.h"
#endif

namespace Anki {
  namespace Cozmo {
    
    namespace { // "Private members"
      const u16 RECV_BUFFER_SIZE = 1024;
      
      // For communications with basestation
#if(USE_WEBOTS_TXRX)
      webots::Emitter *tx_;
      webots::Receiver *rx_;
#else
      TcpServer server;
      UdpClient advRegClient;
#endif
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
      advRegClient.Send((char*)&regMsg, sizeof(regMsg));
    }
    
    // Deregister robot with advertisement service
    void DeregisterRobot()
    {
      RobotAdvertisementRegistration regMsg;
      regMsg.robotID = HAL::GetRobotID();
      regMsg.enableAdvertisement = 0;
      advRegClient.Send((char*)&regMsg, sizeof(regMsg));
    }
    
    
#if(USE_WEBOTS_TXRX)
    ReturnCode InitSimRadio(webots::Robot& webotRobot, s32 robotID)
#else
    ReturnCode InitSimRadio(s32 robotID)
#endif
    {
#if(USE_WEBOTS_TXRX)
      tx_ = webotRobot.getEmitter("radio_tx");
      rx_ = webotRobot.getReceiver("radio_rx");
      
      if(tx_==NULL || rx_==NULL) {
        return EXIT_FAILURE;
      }
      
      rx_->enable(TIME_STEP);
      rx_->setChannel(robotID);
      tx_->setChannel(robotID);
#else
      server.StartListening(ROBOT_RADIO_BASE_PORT + robotID);
      
      // Register with advertising service by sending IP and port info
      // NOTE: Since there is no ACK robot_advertisement_controller must be running before this happens!
      advRegClient.Connect(ROBOT_SIM_WORLD_HOST, ROBOT_ADVERTISEMENT_REGISTRATION_PORT);
      RegisterRobot();
      
#endif
      
      recvBufSize_ = 0;
      
      return EXIT_SUCCESS;
    }
    
    bool HAL::RadioIsConnected(void)
    {
      return server.HasClient();
    }

#if(!USE_WEBOTS_TXRX)
    void DisconnectRadio(void)
    {
      server.DisconnectClient();
      recvBufSize_ = 0;
      
      RegisterRobot();
    }
#endif
    
    bool HAL::RadioSendMessage(const Messages::ID msgID, const void *buffer, TimeStamp_t ts)
    {
      if (server.HasClient()) {
#if(USE_WEBOTS_TXRX)
        // Send the message header (0xBEEF + robotID + msgID)
        const u8 HEADER_LENGTH = 4;
        const u8 header[HEADER_LENGTH] = {
          RADIO_PACKET_HEADER[0],
          RADIO_PACKET_HEADER[1],
          static_cast<u8>(HAL::GetRobotID()),
          static_cast<u8>(msgID)};
        
        // Send the actual message
        const u8 size = Messages::GetSize(msgID);

        tx_->send(header, HEADER_LENGTH);
        tx_->send(buffer, size);
#else

        // Send the message header (0xBEEF + timestamp + robotID + msgID)
        // For TCP comms, send timestamp immediately after the header.
        // This is needed on the basestation side to properly order messages.
        const u8 HEADER_LENGTH = 8;
        u8 header[HEADER_LENGTH];
        UtilMsgError packRes = UtilMsgPack(header, HEADER_LENGTH, NULL, "ccicc",
                    RADIO_PACKET_HEADER[0],
                    RADIO_PACKET_HEADER[1],
                    ts,
                    HAL::GetRobotID(),
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
#endif
        
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
      
#if(USE_WEBOTS_TXRX)
      const void* data;
      
      // Read receiver for as long as it is not empty.
      while (rx_->getQueueLength() > 0) {
        
        // Get head packet
        data = rx_->getData();
        dataSize = rx_->getDataSize();
        
        if(recvBufSize_ + dataSize > RECV_BUFFER_SIZE) {
          PRINT("Radio receive buffer full!");
          return recvBufSize_;
        }
        
        // Copy data to receive buffer
        memcpy(&recvBuf_[recvBufSize_], data, dataSize);
        recvBufSize_ += dataSize;
        
        // Delete processed packet from queue
        rx_->nextPacket();
      }
#else
      // Read available data
      dataSize = server.Recv((char*)&recvBuf_[recvBufSize_], RECV_BUFFER_SIZE - recvBufSize_);
      if (dataSize > 0) {
        recvBufSize_ += dataSize;
      } else if (dataSize < 0) {
        // Something went wrong
        DisconnectRadio();
      }
#endif
      
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
          Messages::ID msgID = static_cast<Messages::ID>(recvBuf_[0]);
          
          if(msgID == Messages::NO_MESSAGE_ID) {
            PRINT("Received NO_MESSAGE_ID over radio.\n");
            // TODO: Not sure what to do here. Toss everything in the buffer? Advance one byte?
          }
          else {
            const u8 size = Messages::GetSize(msgID);
            
            // Check to see if we have the whole message (plus ID) available
            if(bytesAvailable >= size + 1)
            {
              // Copy the data (not including message ID) out of the receive
              // buffer and into the location provided
              memcpy(buffer, recvBuf_+1, size);
              
              // Shift the data in the receive buffer down to get the message
              // and its ID byte out of the buffer
              recvBufSize_ -= size+1;
              memmove(recvBuf_, recvBuf_+size+1, recvBufSize_);
              
              retVal = msgID;
            } // if bytesAvailable >= size+1
            
          } // if/else msgID == NO_MESSAGE_ID
          
        } // if bytesAvailable > 0
      }
      
      return retVal;
    } // RadioGetNextMessage()
    

    void RadioUpdate()
    {
#if(!USE_WEBOTS_TXRX)
      if(!server.HasClient()) {
        if (server.Accept()) {
          DeregisterRobot();
        }
      }
#endif
    }

    
  } // namespace Cozmo
} // namespace Anki