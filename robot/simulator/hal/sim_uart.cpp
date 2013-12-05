#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"

#include <vector>

#ifndef SIMULATOR
#warning If building with sim_uart.cpp, SIMULATOR should be 1.
#endif

// Webots includes
#include <webots/Supervisor.hpp>

namespace Anki
{
  namespace Cozmo
  {
    namespace Sim {  
      extern webots::Supervisor* CozmoBot;
    }
    
    namespace HAL
    {
      namespace {
        const s32 SIMULATED_USB_CHANNEL = 88;
        
        webots::Receiver* usbRX_ = NULL;
        webots::Emitter*  usbTX_ = NULL;
        
        // Use a buffer to avoid a sending every character as its own Webots
        // packet
        std::vector<u8> USBsendBuffer_;
        std::vector<u8> USBrecvBuffer_;
        u32 getCharIndex_ = 0;
      }

      void UARTInit()
      {
        assert(Sim::CozmoBot != NULL);
        
        usbTX_ = Sim::CozmoBot->getEmitter("USB_TX");
        if(usbTX_ == NULL) {
          PRINT("Emitter 'USB_TX' not found.\n");
        }
        
        usbRX_ = Sim::CozmoBot->getReceiver("USB_RX");
        if(usbRX_ == NULL) {
          PRINT("Receiver 'USB_RX' not found.\n");
        }
        if(usbRX_->getChannel() != SIMULATED_USB_CHANNEL)
        {
          PRINT("USB Receiver is set to the wrong channel (expecting %d)\n",
                SIMULATED_USB_CHANNEL);
        }
        
        usbTX_->setChannel(SIMULATED_USB_CHANNEL);
        usbRX_->enable(TIME_STEP);
        
        USBsendBuffer_.reserve(640*480);
        USBrecvBuffer_.reserve(1024);
      }
      
      int USBPutChar(int c)
      {
        // This just buffers chars until we call USBFlush()
        USBsendBuffer_.push_back(static_cast<u8>(c));
        return c;
      }
      
      void USBFlush(void)
      {
        if(not USBsendBuffer_.empty()) {
          usbTX_->send(&(USBsendBuffer_[0]), USBsendBuffer_.size());
          USBsendBuffer_.clear();
        }
      }
      
      // When lookAhead < 0, do a normal getChar()
      // When lookAhead >= 0, do a peek that many bytes ahead
      s32 USBGetChar(u32 timeout, s32 lookAhead)
      {
        // Determine if we're peeking (lookAhead>=0) or not (lookAhead<0)
        bool peek = true;
        if(lookAhead < 0) {
          peek = false;
          lookAhead = 0;
        }
        else if(getCharIndex_+lookAhead >= USBrecvBuffer_.size())
        {
          // We are peeking (possibly ahead), but there isn't enough data left
          return -1;
        }

        if(getCharIndex_+lookAhead == USBrecvBuffer_.size())
        {
          // We've returned all of the last packet we got, so clear the buffer
          // and start again with the next packet
          USBrecvBuffer_.clear();
          getCharIndex_ = 0;
          
          if(usbRX_->getQueueLength() > 0) {
            // If another packet is waiting, read it into the buffer
            const u8* packet = static_cast<const u8*>(usbRX_->getData());
            const s32 packetLength = usbRX_->getDataSize();
            for(s32 i=0; i<packetLength; ++i) {
              USBrecvBuffer_.push_back(packet[i]);
            }
            usbRX_->nextPacket();
          }
          else {
            // There is not currently another packet waitig
            return -1;
          }
        }
        
        // Get the next character in the packet
        s32 retVal = static_cast<s32>(USBrecvBuffer_[getCharIndex_ + lookAhead]);
        
        if(not peek) {
          ++getCharIndex_;
        }
        
        return retVal;
      }
      
      s32 USBGetChar(u32 timeout)
      {
        return USBGetChar(timeout, -1);
      }
      
      s32 USBPeekChar(s32 lookAhead)
      {
        return USBGetChar(0, lookAhead);
      }
    
      void USBSendBuffer(u8* buffer, u32 size)
      {
        usbTX_->send(buffer, size*sizeof(u8));
      }
      
      u32 USBGetNumBytesToRead(void)
      {
        // NOTE: this is just returning number of bytes to read *in the next packet*
        u32 retVal = 0;
        
        if(usbRX_->getQueueLength() > 0) {
          retVal = usbRX_->getDataSize();
        }
        
        return retVal;
      }
      
      
    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki
