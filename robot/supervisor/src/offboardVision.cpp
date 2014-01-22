#include "anki/cozmo/robot/hal.h"



namespace Anki
{
  namespace Cozmo
  {
   
    //
    // Stuff we don't need unless we are in offboard vision mode
    //
    
#if defined(USE_OFFBOARD_VISION) && USE_OFFBOARD_VISION
    
    void HAL::USBSendHeader(const u8 packetType)
    {
      using namespace HAL;
      USBPutChar(USB_PACKET_HEADER[0]);
      USBPutChar(USB_PACKET_HEADER[1]);
      USBPutChar(USB_PACKET_HEADER[2]);
      USBPutChar(USB_PACKET_HEADER[3]);
      USBPutChar(packetType);
    }
    
    void HAL::USBSendFooter(const u8 packetType)
    {
      using namespace HAL;
      USBPutChar(USB_PACKET_FOOTER[0]);
      USBPutChar(USB_PACKET_FOOTER[1]);
      USBPutChar(USB_PACKET_FOOTER[2]);
      USBPutChar(USB_PACKET_FOOTER[3]);
      USBPutChar(packetType);
      
#ifdef SIMULATOR
      USBFlush();
#endif
    }
    
    
    void HAL::USBSendPacket(const u8 packetType, const void* data, const u32 numBytes)
    {
      USBSendHeader(packetType);
      
      const u8* u8data = reinterpret_cast<const u8*>(data);
      for(u32 i=0; i<numBytes; ++i) {
        USBPutChar(u8data[i]);
      }
      
      USBSendFooter(packetType);
    }
    
    void HAL::SendMessageID(const char* name, const u8 msgID)
    {
      USBSendHeader(USB_DEFINE_MESSAGE_ID);
      
      USBPutChar(msgID);
      for(u8 i=0; i<strlen(name); ++i) {
        USBPutChar(static_cast<int>(name[i]));
      }
      
      USBSendFooter(USB_DEFINE_MESSAGE_ID);
    }
    
    void HAL::USBSendMessage(const void* buffer, const Messages::ID msgID)
    {
      USBSendHeader(USB_MESSAGE_HEADER);
      
      const u8* msgData = reinterpret_cast<const u8*>(buffer);
      
      USBPutChar(msgID);
      const u8 size = Messages::GetSize(msgID);
      for(u8 i=0; i<size; ++i) {
        USBPutChar(static_cast<int>(msgData[i]));
      }
      
      USBSendFooter(USB_MESSAGE_HEADER);
    }
    
    
    // TODO: pull the downsampling out of this function
    void HAL::USBSendFrame(const u8*        frame,
                           const TimeStamp  timestamp,
                           const CameraMode inputResolution,
                           const CameraMode sendResolution,
                           const u8         commandByte)
    {
      // Set window size for averaging when downsampling
      const u8 inc = 1 << CameraModeInfo[sendResolution].downsamplePower[inputResolution];
      
      // Tell the receiver what to do with the image once it gets it (and the
      // fact that this is an image packet at all)
      USBSendHeader(commandByte);
      
      // Tell the receiver the resolution of the frame we're sending
      const u8 frameResHeader = CameraModeInfo[sendResolution].header;
      USBPutChar(frameResHeader);
      
      // Send the timestamp
      static_assert(sizeof(TimeStamp) == 4,
                    "Currently assuming sizeof(TimeStamp)==4 when sending to "
                    "offboard vision processor in Matlab.");
      USBSendBuffer(reinterpret_cast<const u8*>(&timestamp), sizeof(TimeStamp));
      
      const u16 nrows = CameraModeInfo[inputResolution].height;
      const u16 ncols = CameraModeInfo[inputResolution].width;
      
      if(inc==1)
      {
        // No averaging
        for(int i=0; i < nrows*ncols; i++)
        {
          USBPutChar(frame[i]);
        }
        
      } else {
        // Average inc x inc windows
        for (int y = 0; y < nrows; y += inc)
        {
          for (int x = 0; x < ncols; x += inc)
          {
            int sum = 0;
            for (int y1 = y; y1 < y + inc; y1++)
            {
              for (int x1 = x; x1 < x + inc; x1++)
              {
                int index = x1 + y1 * ncols;
#ifdef USING_MOVIDIUS_COMPILER
                // Endian issue when running on the movidius
                index ^= 3;
#endif
                sum += frame[index];
              }
            }
            USBPutChar(sum / (inc * inc));
          }
        }
      } // IF / ELSE inc==1
      
      USBSendFooter(commandByte);
      
      PRINT("USBSendFrame(): sent %dx%d frame downsampled to %dx%d.\n",
            CameraModeInfo[inputResolution].width,
            CameraModeInfo[inputResolution].height,
            CameraModeInfo[sendResolution].width,
            CameraModeInfo[sendResolution].height);
      
    } // USBSendFrame()
    
#endif // if USE_OFFBOARD_VISION defined and true
    
    
    
    // TODO: this should probably be elsewhere, since it is not specific to offboard vision.
    // Can't be in uart.cpp or sim_uart.cpp though since it is not specific to
    // hardware vs. simulator either.
    Messages::ID HAL::USBGetNextMessage(u8 *buffer)
    {
      // Note that this is looking for a packet that starts with a 4-byte
      // header plus a message ID byte.  Unlike the packets we are sending
      // out with USBSendFrame and USBSendMessage, there is no footer.
      
      Messages::ID retVal = Messages::NO_MESSAGE_ID;
      
      //PRINT("USBGetNextPacket(): %d bytes available to read.\n", USBGetNumBytesToRead());
      
      // We need there to be at least 5 bytes: 4 for the USB packet header, and
      // 1 for the msgID
      if(USBGetNumBytesToRead() > 5)
      {
        // Peek at the next four bytes to see if we have a header waiting
        if(USBPeekChar(0) == USB_PACKET_HEADER[0] &&
           USBPeekChar(1) == USB_PACKET_HEADER[1] &&
           USBPeekChar(2) == USB_PACKET_HEADER[2] &&
           USBPeekChar(3) == USB_PACKET_HEADER[3])
        {
          // We have a header, so next byte (which we know is availale since
          // we had 5 bytes to read above) will be the message ID.  We can
          // then look up the size of the incoming message to see if we have it
          // all available.
          Messages::ID msgID = static_cast<Messages::ID>(USBPeekChar(4));
          const u8 size = Messages::GetSize(msgID);
          
          // Check to see if we have the whole message (plus ID) available
          // (note that GetNumBytesToRead() will be including the header, so
          //  we have to add 5 to size, which only includes the message
          //  itself, not the header bytes or ID byte)
          if(USBGetNumBytesToRead() >= (size + 5) )
          {
            // If we got here, we're going to read out the whole message into
            // the return buffer.  First, though, get rid of the header
            // bytes.
            
            // Toss the 4 header bytes and 1 msgID byte
            USBGetChar(); // BE
            USBGetChar(); // EF
            USBGetChar(); // F0
            USBGetChar(); // FF
            USBGetChar(); // msgID
            
            // Read out the message
            for(u8 i=0; i<size; ++i) {
              buffer[i] = USBGetChar();
            }
            
            // Now that we've gotten a whole packet out, return the msg ID
            retVal = msgID;
            
          } // if enough bytes available
        } // if valid header
        else {
          // If we got here, we've got at least 4 bytes available, but they
          // are not a valid header, so toss the first one as garbage, so we
          // at least keep moving through what's available on subsequent calls
          // (Note that we could just be off by one, with the 4 available bytes
          //  being 0xXX, 0xBE, 0xEF, 0xF0, so we just want to toss that first
          //  0xXX byte, so that the next time around we will get 0xBEEFF0FF)
          USBGetChar();
        }
      } // if at least 5 bytes available
      
      return retVal;
      
    } // USBGetNextMessage()
    
    
  } // namespace Cozmo
} // namespace Anki

