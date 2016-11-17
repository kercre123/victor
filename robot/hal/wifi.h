/** @file Header file for C++ interface to the WiFi on the RTIP
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __WIFI_h
#define __WIFI_h

#include "anki/types.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      namespace WiFi
      {
        /** Receive data from the WiFi processor and queue it for delivery to CozmoBot main loop
         * @param data Pointer to received data
         * @param uint8_t length The number of bytes in data
         * @return True if the message was accepted, false if there wasn't room
         */
        bool ReceiveMessage(uint8_t* data, uint8_t length);
        
        /** Dispatch messages in main loop and do any other periodic update functions.
         * @return RESULT_OK normally or an error condition
         */
        Result Update();
        
        /** Get pending data to be sent to the WiFi processor
         * @param dest A pointer to write data to
         * @param maxLength The maximum number of bytes to write to dest
         * @return The number of bytes written to dest, may be zero if there was nothing pending
         */
        uint8_t GetTxData(uint8_t* dest, uint8_t maxLength);
      } // namespace WiFi
    } // Namespace HAL
  } // Namespace Cozmo
} // Namespace Anki

#endif
