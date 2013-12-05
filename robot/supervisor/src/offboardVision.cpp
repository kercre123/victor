#include "anki/cozmo/robot/hal.h"

#if !defined(USE_OFFBOARD_VISION) || !USE_OFFBOARD_VISION
#warning offboardVision.cpp should only be included with USE_OFFBOARD_VISION=1
#endif

namespace Anki
{
  namespace Cozmo
  {
    
    namespace {
      // The resolution at which images will be sent over the serial connection
      u8 frameResolution_ = HAL::CAMERA_MODE_QQQVGA;
      u8 msgBuffer_[256];
    }
    
    void SendHeader(const u8 packetType)
    {
      using namespace HAL;
      USBPutChar(USB_PACKET_HEADER[0]);
      USBPutChar(USB_PACKET_HEADER[1]);
      USBPutChar(USB_PACKET_HEADER[2]);
      USBPutChar(USB_PACKET_HEADER[3]);
      USBPutChar(packetType);
    }
    
    void SendFooter(const u8 packetType)
    {
      using namespace HAL;
      USBPutChar(USB_PACKET_FOOTER[0]);
      USBPutChar(USB_PACKET_FOOTER[1]);
      USBPutChar(USB_PACKET_FOOTER[2]);
      USBPutChar(USB_PACKET_FOOTER[3]);
      USBPutChar(packetType);
      
      USBFlush();
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
    
    void HAL::USBSendMessage()
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
      
    } // USBSendMessage()
    
    // Add a character to the ring buffer
    int HAL::USBBufferChar(int c)
    {
      using namespace USBprintBuffer;
      buffer[writeIndex] = (char) c;
      IncrementIndex(writeIndex);
      return c;
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
      
      // Tell the receiver the resolution of the frame we're sending
      const u8 frameResHeader = CameraModeInfo[sendResolution].header;
      SendHeader(frameResHeader);
      
      // Tell the receiver what to do with the image once it gets it
      USBPutChar(commandByte);
      
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
                sum += frame[(x1 + y1 * ncols) ^ 3];
              }
            }
            USBPutChar(sum / (inc * inc));
          }
        }
      } // IF / ELSE inc==1
      
      SendFooter(frameResHeader);
      
    } // USBSendFrame()
    
    
    ReturnCode HAL::USBGetNextPacket(u8 *buffer)
    {
      
      ReturnCode retVal = EXIT_FAILURE;
      
      // We need there to be at least 6 bytes: 4 for the header, 1 for the
      // size byte and one for the msgID
      if(USBGetNumBytesToRead() > 4)
      {
        // Peek at the next four bytes to see if we have a header waiting
        if(USBPeekChar(0) == USB_PACKET_HEADER[0] &&
           USBPeekChar(1) == USB_PACKET_HEADER[1] &&
           USBPeekChar(2) == USB_PACKET_HEADER[2] &&
           USBPeekChar(3) == USB_PACKET_HEADER[3])
        {
          // Peek at the next byte to see how large this message will be
          int size  = USBPeekChar(4);
          
          // Check to see if we have the whole message available
          // (note that GetNumBytesToRead() will be including the header, so
          //  we have to add 4 to size, which only includes the message
          //  itself)
          if( size >= 0 && USBGetNumBytesToRead() >= (size + 4) )
          {
            // If got here, we're going to read out the whole message into
            // the return buffer.  First, though, get rid of the header
            // bytes.
            
            // Toss the 4 header bytes
            USBGetChar();
            USBGetChar();
            USBGetChar();
            USBGetChar();
            
            // Read out the message
            for(u8 i=0; i<size; ++i) {
              buffer[i] = USBGetChar();
            }
            
            // Now that we've gotten a whole packet out, return success
            retVal = EXIT_SUCCESS;
            
          } // if enough bytes available
        }
        else {
          // If we got here, we've got at least 4 bytes available, but they
          // are not a valid header, so toss the first one as garbage, so we
          // at least keep moving through what's available on subsequent calls
          // (Note that we could just be off by one, with the 4 available bytes
          //  being 0xXX, 0xBE, 0xEF, 0xF0, so we just want to toss that first
          //  0xXX byte, so that the next time around we will get 0xBEEFF0FF)
          USBGetChar();
        }
      } // if at least 4 bytes available
      
      return retVal;
      
    } // USBGetNextPacket()
    
  } // namespace Cozmo
} // namespace Anki