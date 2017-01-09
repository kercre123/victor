/*
 * File: animationMessageBuffer.cpp
 *
 * Author: Jordan Rivas
 * Created: 07/14/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#include "anki/cozmo/basestation/animations/animationControllerTypes.h"
#include "anki/cozmo/basestation/animations/animationMessageBuffer.h"
#include "anki/cozmo/basestation/robot.h"


#include "util/logging/logging.h"



namespace Anki {
namespace Cozmo {
namespace RobotAnimation {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Don't send more than 1000 bytes every 2ms
const s32 AnimationMessageBuffer::MAX_BYTES_FOR_RELIABLE_TRANSPORT = (1000/2) * BS_TIME_STEP;
// This is roughly (2 x ExpectedOneWayLatency_ms + BasestationTick_ms) / AudioSampleLength_ms
const s32 AnimationMessageBuffer::NUM_AUDIO_FRAMES_LEAD = std::ceil((2.f * 200.f + BS_TIME_STEP) / static_cast<f32>(IKeyFrame::SAMPLE_LENGTH_MS));
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t AnimationMessageBuffer::CalculateNumberOfBytesToSend(Robot& robot)
{
  // Compute number of bytes free in robot animation buffer.
  // This is a lower bound since this is computed from a delayed measure
  // of the number of animation bytes already played on the robot.
  s32 totalNumBytesStreamed = robot.GetNumAnimationBytesStreamed();
  s32 totalNumBytesPlayed = robot.GetNumAnimationBytesPlayed();
  
  bool overflow = (totalNumBytesStreamed < 0) && (totalNumBytesPlayed > 0);
  DEV_ASSERT_MSG((totalNumBytesStreamed >= totalNumBytesPlayed) || overflow,
                 "AnimationStreamer.UpdateAmountToSend.BytesPlayedExceedsStreamed",
                 "totalNumBytesStreamed: %d, totalNumBytesPlayed: %d, overflow: %s",
                 totalNumBytesStreamed,
                 totalNumBytesPlayed,
                 (overflow ? "Yes" : "No"));
  
  s32 minBytesFreeInRobotBuffer = static_cast<size_t>(AnimConstants::KEYFRAME_BUFFER_SIZE) - (totalNumBytesStreamed - totalNumBytesPlayed);
  if (overflow) {
    // Computation for minBytesFreeInRobotBuffer still works out in overflow case
    PRINT_CH_INFO(KAnimationControllerLogChannel,
                  "AnimationMessageBuffer.UpdateAmountToSend.BytesStreamedOverflow",
                  "free %d (streamed = %d, played %d)",
                  minBytesFreeInRobotBuffer, totalNumBytesStreamed, totalNumBytesPlayed);
  }
  
  if (minBytesFreeInRobotBuffer < 0) {
    PRINT_NAMED_WARNING("AnimationMessageBuffer.UpdateAmountToSend.NegativeMinBytesFreeInRobot",
                        "minBytesFree: %d numBytesStreamed: %d numBytesPlayed: %d",
                        minBytesFreeInRobotBuffer, totalNumBytesStreamed, totalNumBytesPlayed);
    minBytesFreeInRobotBuffer = 0;
  }
  
  // Reset the number of bytes we can send each Update() as a form of
  // flow control: Don't send frames if robot has no space for them, and be
  // careful not to overwhelm reliable transport either, in terms of bytes or
  // sheer number of messages. These get decremenged on each call to
  // SendBufferedMessages() below
  return std::min(MAX_BYTES_FOR_RELIABLE_TRANSPORT,
                             minBytesFreeInRobotBuffer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t AnimationMessageBuffer::CalculateNumberOfFramesToSend(Robot& robot)
{
  // Compute number of bytes free in robot animation buffer.
  // This is a lower bound since this is computed from a delayed measure
  // of the number of animation bytes already played on the robot.
  s32 totalNumAudioFramesStreamed = robot.GetNumAnimationAudioFramesStreamed();
  s32 totalNumAudioFramesPlayed = robot.GetNumAnimationAudioFramesPlayed();
  
  s32 audioFramesInBuffer = totalNumAudioFramesStreamed - totalNumAudioFramesPlayed;
  
  if (K_ANIMATION_CONTROLLER_UPDATE_TICK_LOG_LEVEL) {
    
    if (K_ANIMATION_CONTROLLER_UPDATE_TICK_LOG_LEVEL >= 2)
      PRINT_CH_INFO(KAnimationControllerLogChannel,
                    "AnimationMessageBuffer.UpdateAmountToSend",
                    "Streamed = %d, Played = %d, InBuffer = %d",
                    totalNumAudioFramesStreamed, totalNumAudioFramesPlayed, audioFramesInBuffer);
  }
  return std::max(0, NUM_AUDIO_FRAMES_LEAD-audioFramesInBuffer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AnimationMessageBuffer::BufferMessageToSend(RobotInterface::EngineToRobot* msg)
{
  if(msg != nullptr) {
    PushMessageInSendBuffer(msg);
    if (K_ANIMATION_CONTROLLER_UPDATE_TICK_LOG_LEVEL >= 2) {
      PRINT_CH_INFO(KAnimationControllerLogChannel,
                    "AnimationMessageBuffer.BufferMessageToSend.msg",
                    "Msg tag: %s",
                    EngineToRobotTagToString(msg->GetTag()));
    }
    
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AnimationMessageBuffer::BufferMessageToSend(EngineToRobotMessageList& out_msgList)
{
  if (!out_msgList.empty()) {
    PushMessageInSendBuffer(out_msgList);
    if (K_ANIMATION_CONTROLLER_UPDATE_TICK_LOG_LEVEL >= 2) {
      PRINT_CH_INFO(KAnimationControllerLogChannel,
                    "AnimationMessageBuffer.BufferMessageToSend.out_msgList",
                    "MsgList Size: %d",
                    (uint32_t)out_msgList.size());
    }
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationMessageBuffer::PushMessageInSendBuffer(RobotInterface::EngineToRobot* msg)
{
  std::lock_guard<std::mutex> l{_lock};
  _sendBuffer.push_back(msg);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationMessageBuffer::PushMessageInSendBuffer(EngineToRobotMessageList& out_msgList)
{
  std::lock_guard<std::mutex> l{_lock};
  _sendBuffer.splice(_sendBuffer.end(), out_msgList);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const RobotInterface::EngineToRobot* AnimationMessageBuffer::PopMessageFromSendBuffer()
{
  std::lock_guard<std::mutex> l{_lock};
  const RobotInterface::EngineToRobot* front = _sendBuffer.front();
  _sendBuffer.pop_front();
  return front;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AnimationMessageBuffer::SendMessages(Robot& robot, uint32_t maxByteCount)
{
# if K_ANIMATION_CONTROLLER_UPDATE_TICK_LOG_LEVEL
  int32_t numSent = 0;
# endif
  
  
  uint32_t byteSentCount = 0;
  // Empty out anything waiting in the send buffer:
  const RobotInterface::EngineToRobot* msg = nullptr;
  while(!_sendBuffer.empty()) {
    if (K_ANIMATION_CONTROLLER_UPDATE_TICK_LOG_LEVEL) {
      PRINT_CH_INFO(KAnimationControllerLogChannel,
                    "AnimationMessageBuffer.SendMessages",
                    "Send buffer length=%lu.", (unsigned long)_sendBuffer.size());
    }
    
    // Compute number of bytes required and whether the robot is accepting any
    // further audio frames yet
    msg = _sendBuffer.front();
    const size_t numBytesRequired = msg->Size();
    
    s32 numAudioFramesRequired = 0;
    if(RobotInterface::EngineToRobotTag::animAudioSample == msg->GetTag() ||
       RobotInterface::EngineToRobotTag::animAudioSilence == msg->GetTag())
    {
      ++numAudioFramesRequired;
    }
    
    if(numBytesRequired + byteSentCount <= maxByteCount)
    {
      if (K_ANIMATION_CONTROLLER_UPDATE_TICK_LOG_LEVEL) {
        PRINT_CH_INFO(KAnimationControllerLogChannel,
                      "AnimationMessageBuffer.SendMessages.Robot.SendMessage",
                      "SentMessage: '%s'",
                      RobotInterface::EngineToRobotTagToString(msg->GetTag()));
      }
      
      
      Result sendResult = robot.SendMessage(*msg);
      if(sendResult != RESULT_OK) {
        return false;
      }
      
#     if K_ANIMATION_CONTROLLER_UPDATE_TICK_LOG_LEVEL
      ++numSent;
#     endif
      
      // Increment total number of bytes and audio frames streamed to robot
      robot.IncrementNumAnimationBytesStreamed((int32_t)numBytesRequired);
      robot.IncrementNumAnimationAudioFramesStreamed(numAudioFramesRequired);
      
      PopMessageFromSendBuffer();
      delete msg;
    }
    else {
      // Out of bytes or audio frames to send, continue on next Update()
      if (K_ANIMATION_CONTROLLER_UPDATE_TICK_LOG_LEVEL) {
        PRINT_CH_INFO(KAnimationControllerLogChannel,
                      "AnimationMessageBuffer.SendMessages.SentMax",
                      "Sent %d messages, but ran out of bytes or audio frames to "
                      "send from buffer. %lu bytes remain, so will continue next Update().",
                      numSent, (unsigned long)_sendBuffer.size());
      }
      return true;// RESULT_OK;
    }
  }
  
  // Sanity check
  // If we got here, we've finished streaming out everything in the send
  // buffer -- i.e., all the frames associated with the last audio keyframe
//  assert(_numBytesToSend >= 0);
  assert(_sendBuffer.empty());
  
  
  if (K_ANIMATION_CONTROLLER_UPDATE_TICK_LOG_LEVEL) {
    if(numSent > 0) {
      PRINT_CH_INFO(KAnimationControllerLogChannel,
                    "AnimationMessageBuffer.SendMessages.Sent",
                    "Sent %d messages, %lu remain in buffer.",
                    numSent, (unsigned long)_sendBuffer.size());
    }
  }
  
  return  true; // RESULT_OK;
}


} // namespace RobotAnimation
} // namespcae Cozmo
} // namespace Anki
