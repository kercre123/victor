#include "anki/cozmo/robot/hal.h"

namespace Anki
{
  namespace Cozmo
  {
    void SendHeader(const u8 packetType)
    {
      USBPutChar(USB_PACKET_HEADER[0]);
      USBPutChar(USB_PACKET_HEADER[1]);
      USBPutChar(USB_PACKET_HEADER[2]);
      USBPutChar(USB_PACKET_HEADER[3]);
      USBPutChar(packetType);
    }
    
    void SendFooter(const u8 packetType)
    {
      USBPutChar(USB_PACKET_FOOTER[0]);
      USBPutChar(USB_PACKET_FOOTER[1]);
      USBPutChar(USB_PACKET_FOOTER[2]);
      USBPutChar(USB_PACKET_FOOTER[3]);
      USBPutChar(packetType);
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
    }
    
    // ANS: This was declared static. Is that necessary?
    void HAL::USBSendFrame()
    {
      const u8* image = frame;
      
      // Set window size for averaging when downsampling and send
      // a corresponding header
      u32 inc = 1;
      u8 frameResHeader;
      switch(frameResolution)
      {
        case CAMERA_MODE_QVGA:
          inc = 2;
          frameResHeader = CAMERA_MODE_QVGA_HEADER;
          break;
          
        case CAMERA_MODE_QQVGA:
          inc = 4;
          frameResHeader = CAMERA_MODE_QQVGA_HEADER;
          break;
          
        case CAMERA_MODE_QQQVGA:
          inc = 8;
          frameResHeader = CAMERA_MODE_QQQVGA_HEADER;
          break;
          
        case CAMERA_MODE_QQQQVGA:
          inc = 16;
          frameResHeader = CAMERA_MODE_QQQQVGA_HEADER;
          break;
          
        case CAMERA_MODE_VGA:
        default:
          inc = 1;
          frameResHeader = CAMERA_MODE_VGA_HEADER;
          
      } // SWITCH(frameResolution)
      
      SendHeader(frameResHeader);
      
      if(inc==1)
      {
        // No averaging
        for(int i=0; i < 640*480; i++)
        {
          USBPutChar(image[i]);
        }
        
      } else {
        // Average inc x inc windows
        for (int y = 0; y < 480; y += inc)
        {
          for (int x = 0; x < 640; x += inc)
          {
            int sum = 0;
            for (int y1 = y; y1 < y + inc; y1++)
            {
              for (int x1 = x; x1 < x + inc; x1++)
              {
                sum += image[(x1 + y1 * 640) ^ 3];
              }
            }
            USBPutChar(sum / (inc * inc));
          }
        }
      } // IF / ELSE inc==1
      
      SendFooter(frameResHeader);
      
    } // USBSendFrame()
    
  } // namespace Cozmo
} // namespace Anki