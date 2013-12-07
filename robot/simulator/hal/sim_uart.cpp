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
        //const s32 SIMULATED_USB_CHANNEL = 88;
        
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
        
        usbTX_ = Sim::CozmoBot->getEmitter("usb_tx");
        if(usbTX_ == NULL) {
          PRINT("Emitter 'usb_tx' not found.\n");
        }
        
        usbRX_ = Sim::CozmoBot->getReceiver("usb_rx");
        if(usbRX_ == NULL) {
          PRINT("Receiver 'usb_rx' not found.\n");
        }
        PRINT("USB Receiver operating on channel %d.\n", usbRX_->getChannel());
        if(usbTX_->getChannel() != usbRX_->getChannel()) {
          PRINT("USB Receiver and Emitter not on the same channel!\n");
        }
        
        //usbTX_->setChannel(SIMULATED_USB_CHANNEL);
        usbRX_->enable(TIME_STEP);
        
        PRINT("Robot's USB TX buffer size is %d bytes.\n",
              usbTX_->getBufferSize());
        
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
        if(not USBsendBuffer_.empty())
        {
          const u32 sendSize = USBsendBuffer_.size()*sizeof(u8);
          if(usbTX_->send(&(USBsendBuffer_[0]), sendSize) == 0)
          {
            PRINT("USBFlush(): Send buffer full!\n");
          }
          else {
            PRINT("USBFlush(): Sent %d bytes on channel %d.\n",
                  sendSize, usbTX_->getChannel());
          }
          USBsendBuffer_.clear();
        }
      }
      
      s32 USBGetChar(u32 timeout)
      {
        if(USBGetNumBytesToRead() == 0) {
          return -1;
        }
        
        // Get the next character in the packet
        s32 retVal = static_cast<s32>(USBrecvBuffer_[getCharIndex_]);
        
        ++getCharIndex_;
        
        return retVal;
        
      } // USBGetChar()
      
      s32 USBPeekChar(u32 lookAhead)
      {
        // If trying to peek further than there are characters in the buffer
        // exit now.
        if(USBGetNumBytesToRead() <= lookAhead) {
          return -1;
        }
        
        // Get the next character in the packet
        return static_cast<s32>(USBrecvBuffer_[getCharIndex_ + lookAhead]);
      }
    
      void USBSendBuffer(u8* buffer, u32 size)
      {
        for(u32 i=0; i<size; ++i) {
          USBPutChar(static_cast<s32>(buffer[i]));
        }
      }
      
      u32 USBGetNumBytesToRead(void)
      {
        if(usbRX_->getQueueLength() > 0) {
          // If another packet is waiting, read it into the buffer
          const u8* packet = static_cast<const u8*>(usbRX_->getData());
          const s32 packetLength = usbRX_->getDataSize();
          for(s32 i=0; i<packetLength; ++i) {
            USBrecvBuffer_.push_back(packet[i]);
          }
          usbRX_->nextPacket();
        }
        
        // NOTE: this is just returning number of bytes to read *in the current packet*
        return USBrecvBuffer_.size() - getCharIndex_;
      }
      
      
    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki
