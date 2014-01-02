#include "anki/cozmo/robot/hal.h"

/*
 * usbPrint.cpp
 *
 * Author: Andrew Stein
 *
 * Description:  Implementation of the USB print buffer for queuing characters
 *               from PRINT commands until we want to send them from the long
 *               execution thread using USBSendPrintBuffer().
 *
 * Copyright 2013, Anki, Inc.
 */

namespace Anki
{
  namespace Cozmo
  {
    
    // "Private member variables" for the USB print buffer
    namespace {
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
      
    } // private namespace
    
    void HAL::USBSendPrintBuffer()
    {
      // Send all the characters in the buffer as of right now
      const u32 endIndex = writeIndex; // make a copy of where we should stop
      
      // Nothing to send
      if (endIndex == readIndex) {
        return;
      }
      
#if defined(USE_OFFBOARD_VISION) && USE_OFFBOARD_VISION
      USBSendHeader(USB_MESSAGE_HEADER);
#endif
      
      while(readIndex != endIndex)
      {
        // Send the character and circularly increment the read index:
        HAL::USBPutChar(buffer[readIndex]);
        IncrementIndex(readIndex);
      }
      
#if defined(USE_OFFBOARD_VISION) && USE_OFFBOARD_VISION
      USBSendFooter(USB_MESSAGE_HEADER);
#endif
      
    } // USBSendPrintBuffer()
    
    
    
    // Add a character to the ring buffer
    int HAL::USBBufferChar(int c)
    {
      buffer[writeIndex] = (char) c;
      IncrementIndex(writeIndex);
      return c;
    }
    
  } // namespace Cozmo
} // namespace Anki

