/**
 * File: cozmoAnimComms.h
 *
 * Author: Kevin Yoon
 * Created: 7/30/2017
 *
 * Description: Create sockets and manages low-level data transfer to engine and robot processes
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef COZMO_ANIM_COMMS_H
#define COZMO_ANIM_COMMS_H

#include "anki/common/types.h"

namespace Anki {
namespace Cozmo {
namespace CozmoAnimComms {
  
  Result InitComms();
  
  // Gets the next packet from the engine socket
  u32 GetNextPacketFromEngine(u8* buffer, u32 max_length);

  // Get the next packet from robot socket
  u32 GetNextPacketFromRobot(u8* buffer, u32 max_length);
  
  // Send a packet to engine
  bool SendPacketToEngine(const void *buffer, const u32 length);
  
  // Send a packet to robot
  bool SendPacketToRobot(const void *buffer, const u32 length);

  void DisconnectEngine();
  void DisconnectRobot();
  
  // TODO: Is this necessary?
  void UpdateEngineCommsState(u8 wifi);
  
  bool EngineIsConnected();
  
} // namespace CozmoAnimComms
} // namespace Cozmo
} // namespace Anki

#endif  // #ifndef COZMO_ANIM_COMMS_H
