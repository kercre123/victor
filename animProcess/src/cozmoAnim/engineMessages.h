/**
 * File: engineMessages.h
 *
 * Author: Kevin Yoon
 * Created: 6/30/2017
 *
 * Description: Shuttles messages between engine and robot processes. 
 *              Responds to engine messages pertaining to animations 
 *              and inserts messages as appropriate into robot-bound stream.
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef COZMO_ANIM_ENGINE_MSG_HANDLER_H
#define COZMO_ANIM_ENGINE_MSG_HANDLER_H

#include "anki/common/types.h"
//#include "anki/types.h"

#include <stdarg.h>
#include <stddef.h>
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"

namespace Anki {
namespace Cozmo {

namespace HAL {
  
    extern "C" TimeStamp_t GetTimeStamp(void);
  
    Result InitRadio();
  
    bool RadioIsConnected();
    
    void RadioUpdateState(u8 wifi);
    
    int RadioQueueAvailable();
    
    void DisconnectRadio();
    
    /** Gets the next packet from the radio
     * @param buffer [out] A buffer into which to copy the packet. Must have MTU bytes available
     * return The number of bytes of the packet or 0 if no packet was available.
     */
    u32 RadioGetNextPacket(u8* buffer);
    
    /** Send a packet on the radio.
     * @param buffer [in] A pointer to the data to be sent
     * @param length [in] The number of bytes to be sent
     * @param socket [in] Socket number, default 0 (base station)
     * @return true if the packet was queued for transmission, false if it couldn't be queued.
     */
    bool RadioSendPacket(const void *buffer, const u32 length, const u8 socket=0);
    
    /** Wrapper method for sending messages NOT PACKETS
     * @param msgID The ID (tag) of the message to be sent
     * @param buffer A pointer to the message to be sent
     * @return True if sucessfully queued, false otherwise
     */
    bool RadioSendMessage(const void *buffer, const u16 size, const u8 msgID);
  
  
  
  void DisconnectRobot();
  bool SendPacketToRobot(const void *buffer, const u32 length);
  u32 GetNextPacketFromRobot(u8* buffer, u32 max_length);

  
}// end namespace HAL
    
    
namespace Messages {

      // Create all the dispatch function prototypes (all implemented
      // manually in messages.cpp).
      //#include "clad/robotInterface/messageEngineToRobot_declarations.def"

      void ProcessBadTag_EngineToRobot(const RobotInterface::EngineToRobot::Tag tag);

      Result Init();
      extern "C" void ProcessMessage(u8* buffer, u16 bufferSize);

      void Update();

      void ProcessMessage(RobotInterface::EngineToRobot& msg);


} // namespace Messages
} // namespace Cozmo
} // namespace Anki


#endif  // #ifndef COZMO_ANIM_ENGINE_MSG_HANDLER_H
