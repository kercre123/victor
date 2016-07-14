/*
 * File: animationMessageBuffer.h
 *
 * Author: Jordan Rivas
 * Created: 07/14/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#ifndef __Basestation_Animations_AnimationMessageBuffer_H__
#define __Basestation_Animations_AnimationMessageBuffer_H__

#include "anki/cozmo/basestation/animations/animationControllerTypes_Internal.h"
#include <functional>
#include <list>
#include <mutex>


namespace Anki {
namespace Cozmo {
  
class Robot;
  
namespace RobotInterface {
  class EngineToRobot;
}
  
namespace RobotAnimation {

class AnimationMessageBuffer {
  
public:
  
  // "Flow control" for not overrunning reliable transport in a single
  // update tick
  static const int32_t MAX_BYTES_FOR_RELIABLE_TRANSPORT;
  
  // "Flow control" for not getting too far ahead of the robot, to help prevent
  // too much delay when we want to layer something on "now". This is number of
  // audio frames.
  static const int32_t NUM_AUDIO_FRAMES_LEAD;
  
  // Get info about robot streaming buffer
  static uint32_t CalculateNumberOfBytesToSend(Robot& robot);
  static uint32_t CalculateNumberOfFramesToSend(Robot& robot);
  
  
  
  // Thread safe move messages into SendBuffer
  bool BufferMessageToSend(RobotInterface::EngineToRobot* msg);
  
  // TODO: Is there a way to move the pointer list? or do we need to copy it?
  bool BufferMessageToSend(EngineToRobotMessageList& msgList);
  
  // Move messages form SendBuffer to Robot Streaming Buffer
  bool SendMessages(Robot& robot, uint32_t maxByteCount);
  
  bool HasMessages() const { return !_sendBuffer.empty(); }
  
  
private:
  
  // TODO: May not need lock we are moving all messages in/out of SendBuffer on the same thread
  std::mutex _lock;
  EngineToRobotMessageList _sendBuffer;
  
  void PushMessageInSendBuffer(RobotInterface::EngineToRobot* msg);
  void PushMessageInSendBuffer(EngineToRobotMessageList& out_msgList);
  
  const RobotInterface::EngineToRobot* PopMessageFromSendBuffer();
  
  
};

} // namespace RobotAnimation
} // namespcae Cozmo
} // namespace Anki


#endif /* __Basestation_Animations_AnimationMessageBuffer_H__ */
