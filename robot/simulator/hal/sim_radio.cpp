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

#include <webots/Supervisor.hpp>

namespace Anki {
  namespace Cozmo {
    
    namespace { // "Private members"
      const u16 RECV_BUFFER_SIZE = 1024;
      
      // For communications with basestation
      webots::Emitter *tx_;
      webots::Receiver *rx_;
      bool isConnected_;
      u8 recvBuf_[RECV_BUFFER_SIZE];
      s32 recvBufSize_;
    }

    ReturnCode InitSimRadio(webots::Robot& webotRobot, s32 robotID)
    {
      tx_ = webotRobot.getEmitter("radio_tx");
      rx_ = webotRobot.getReceiver("radio_rx");
      
      if(tx_==NULL || rx_==NULL) {
        return EXIT_FAILURE;
      }
      
      rx_->enable(TIME_STEP);
      rx_->setChannel(robotID);
      tx_->setChannel(robotID);
      recvBufSize_ = 0;
      
      return EXIT_SUCCESS;
    }
    
    bool HAL::RadioIsConnected(void)
    {
      return isConnected_;
    }
    
    bool HAL::RadioSendMessage(const Messages::ID msgID, const void *buffer)
    {
      // Send the message header (0xBEEF + robotID + msgID)
      const u8 HEADER_LENGTH = 4;
      const u8 header[HEADER_LENGTH] = {
        RADIO_PACKET_HEADER[0],
        RADIO_PACKET_HEADER[1],
        static_cast<u8>(HAL::GetRobotID()),
        static_cast<u8>(msgID)};
      
      tx_->send(header, HEADER_LENGTH);
      
      // Send the actual message
      const u8 size = Messages::GetSize(msgID);
      tx_->send(buffer, size);
      
      return true;
      
    } // RadioSendMessage()
    
    
    u32 HAL::RadioGetNumBytesAvailable(void)
    {
      // Check for incoming data and add it to receive buffer
      int dataSize;
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
      
      return retVal;
    } // RadioGetNextMessage()
    
    
  } // namespace Cozmo
} // namespace Anki