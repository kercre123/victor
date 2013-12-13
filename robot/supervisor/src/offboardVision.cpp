#include "anki/cozmo/robot/hal.h"

#if !defined(USE_OFFBOARD_VISION) || !USE_OFFBOARD_VISION
#warning offboardVision.cpp should only be included with USE_OFFBOARD_VISION=1
#endif

namespace Anki
{
  namespace Cozmo
  {
    
        
    void SendHeader(const u8 packetType)
    {
      using namespace HAL;
      USBPutChar(Messages::USB_PACKET_HEADER[0]);
      USBPutChar(Messages::USB_PACKET_HEADER[1]);
      USBPutChar(Messages::USB_PACKET_HEADER[2]);
      USBPutChar(Messages::USB_PACKET_HEADER[3]);
      USBPutChar(packetType);
    }
    
    void SendFooter(const u8 packetType)
    {
      using namespace HAL;
      USBPutChar(Messages::USB_PACKET_FOOTER[0]);
      USBPutChar(Messages::USB_PACKET_FOOTER[1]);
      USBPutChar(Messages::USB_PACKET_FOOTER[2]);
      USBPutChar(Messages::USB_PACKET_FOOTER[3]);
      USBPutChar(packetType);
      
#ifdef SIMULATOR
      USBFlush();
#endif
    }
    
    namespace USBprintBuffer
    {
      // This is a (ring) buffer to store messages created using printf in main
      // execution, until they can be picked up and actually sent over the USB
      // UART by long execution when we are also using the UART to send image
      // data and don't want to step on that data with frequent main execution
      // messages.
      static const u32  BUFFER_LENGTH = 512;
      
      static char  buffer[BUFFER_LENGTH];
      static u32   readIndex = 0;
      static u32   writeIndex = 0;
      
      void IncrementIndex(u32& index) {
        ++index;
        if(index == BUFFER_LENGTH) {
          index = 0;
        }
      }
      
    } // namespace USBprintBuffer
    
    void HAL::SendMessageID(const char* name, const u8 msgID)
    {
      SendHeader(USB_DEFINE_MESSAGE_ID);
      
      USBPutChar(msgID);
      for(u8 i=0; i<strlen(name); ++i) {
        USBPutChar(static_cast<int>(name[i]));
      }
      
      SendFooter(USB_DEFINE_MESSAGE_ID);
    }
    
    void HAL::USBSendMessage(const void* buffer, const Messages::ID msgID)
    {
      SendHeader(USB_MESSAGE_HEADER);
      
      const u8* msgData = reinterpret_cast<const u8*>(buffer);
      
      USBPutChar(msgID);
      const u8 size = Messages::LookupTable[msgID].size;
      for(u8 i=0; i<size; ++i) {
        USBPutChar(static_cast<int>(msgData[i]));
      }
      
      SendFooter(USB_MESSAGE_HEADER);
    }
   
    
    void HAL::USBSendPrintBuffer()
    {
      using namespace USBprintBuffer;
      
      // Send all the characters in the buffer as of right now
      const u32 endIndex = writeIndex; // make a copy of where we should stop
      
      // Nothing to send
      if (endIndex == readIndex) {
        return;
      }
      
      SendHeader(USB_MESSAGE_HEADER);
      
      while(readIndex != endIndex)
      {
        // Send the character and circularly increment the read index:
        HAL::USBPutChar(buffer[readIndex]);
        IncrementIndex(readIndex);
      }
      
      SendFooter(USB_MESSAGE_HEADER);
      
    } // USBSendPrintBuffer()
    
    // Add a character to the ring buffer
    int HAL::USBBufferChar(int c)
    {
      using namespace USBprintBuffer;
      buffer[writeIndex] = (char) c;
      IncrementIndex(writeIndex);
      return c;
    }
    
    
    void HAL::USBSendPacket(const u8 packetType, const void* data, const u32 numBytes)
    {
      SendHeader(packetType);
      
      const u8* u8data = reinterpret_cast<const u8*>(data);
      for(u32 i=0; i<numBytes; ++i) {
        USBPutChar(u8data[i]);
      }
      
      SendFooter(packetType);
    }
    
  
    // TODO: pull the downsampling out of this function
    void HAL::USBSendFrame(const u8* frame,
                           const CameraMode inputResolution,
                           const CameraMode sendResolution,
                           const u8 commandByte)
    {
      // Set window size for averaging when downsampling
      const u8 inc = CameraModeInfo[sendResolution].downsampleInc[inputResolution];
      if(inc == 0) {
        PRINT("USBSendFrame(): send/input resolutions not compatible.\n");
        return;
      }
      
      // Tell the receiver what to do with the image once it gets it (and the
      // fact that this is an image packet at all)
      SendHeader(commandByte);
      
      // Tell the receiver the resolution of the frame we're sending
      const u8 frameResHeader = CameraModeInfo[sendResolution].header;
      USBPutChar(frameResHeader);
      
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
      
      SendFooter(commandByte);
      
      PRINT("USBSendFrame(): sent %dx%d frame downsampled to %dx%d.\n",
            CameraModeInfo[inputResolution].width,
            CameraModeInfo[inputResolution].height,
            CameraModeInfo[sendResolution].width,
            CameraModeInfo[sendResolution].height);
      
    } // USBSendFrame()
    
    

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
        if(USBPeekChar(0) == Messages::USB_PACKET_HEADER[0] &&
           USBPeekChar(1) == Messages::USB_PACKET_HEADER[1] &&
           USBPeekChar(2) == Messages::USB_PACKET_HEADER[2] &&
           USBPeekChar(3) == Messages::USB_PACKET_HEADER[3])
        {
          // We have a header, so next byte (which we know is availale since
          // we had 5 bytes to read above) will be the message ID.  We can
          // then look up the size of the incoming message to see if we have it
          // all available.
          Messages::ID msgID = static_cast<Messages::ID>(USBPeekChar(4));
          const u8 size = Messages::LookupTable[msgID].size;
          
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