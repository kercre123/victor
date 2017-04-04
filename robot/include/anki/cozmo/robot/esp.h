/**
 * File: esp.h
 *
 * Author: Daniel Casner <daniel@anki.com>
 * Created: 10/22/2015
 *
 *
 * Description:
 *
 *   This file collects commonly used functions that are needed by lots of code running on the Espressif or its
 *   its simulator thread. Since no application code on the Espressif directly manipulates the HAL, here is no
 *   hal.h for the Espressif and this file takes its place.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

extern "C" {
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
/// Returns the robot's serial number or 0xFFFFffff if not set
uint32_t getSerialNumber(void);
}

#define memset os_memset
#define memcpy os_memcpy
#define malloc os_zalloc
#define free   os_free
#define vsnprintf ets_vsnprintf

/** PRINT macro direction */
#define PRINT_TO_RADIO  1
#define PRINT_TO_UART   2
#define PRINT_TO_SCREEN 3

#define PRINT_DESTINATION PRINT_TO_RADIO

#if   PRINT_DESTINATION == PRINT_TO_RADIO
#include "messages.h"
#define PRINT(...) Anki::Cozmo::Messages::SendText(__VA_ARGS__)
#elif PRINT_DESTINATION == PRINT_TO_UART
#define PRINT os_printf
#elif PRINT_DESTINATION == PRINT_TO_SCREEN
#include "Face.h"
#define PRINT Face::FacePrintf
#endif

#define assert(x) while(!(x)) PRINT("ASSERT in %s line %d\r\n", __FILE__, __LINE__)

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      /** Wrapper method for sending messages NOT PACKETS
       * @param msgID The ID (tag) of the message to be sent
       * @param buffer A pointer to the message to be sent
       * @param msgID the tag to attach to the given message
       * @return True if sucessfully queued, false otherwise
       */
      bool RadioSendMessage(const void *buffer, const u16 size, const u8 msgID);
      
      void FaceAnimate(u8* frame, const u16 length);
    }
  }
}
