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

#include <stdarg.h>
#include <stddef.h>
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"

namespace Anki {
namespace Cozmo {
  
// Forward declarations
class AnimationStreamer;
namespace Audio {
class EngineAudioInput;
}
  
namespace Messages {

  // Create all the dispatch function prototypes (all implemented
  // manually in messages.cpp).
  //#include "clad/robotInterface/messageEngineToRobot_declarations.def"

  Result Init(AnimationStreamer* animStreamer, Audio::EngineAudioInput* audioInput);

  void Update();
  
  bool SendToEngine(const void *buffer, const u16 size, const u8 msgID);
  bool SendToRobot(const RobotInterface::EngineToRobot& msg);
  
  
} // namespace Messages
} // namespace Cozmo
} // namespace Anki


#endif  // #ifndef COZMO_ANIM_ENGINE_MSG_HANDLER_H
