/**
 * File: animation.cpp
 *
 * Authors: Andrew Stein
 * Created: 2015-06-25
 *
 * Description:
 *    Class for storing a single animation, which is made of
 *    tracks of keyframes. Also manages streaming those keyframes
 *    to a robot.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/animation.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/cozmo/basestation/soundManager.h"

#include "util/logging/logging.h"

#include "anki/common/basestation/utils/timer.h"

//#include <cassert>

#define DEBUG_ANIMATIONS 0
// Until we have a speaker in the robot, use this to play RobotAudioKeyFrames using
// the device's SoundManager
#define PLAY_ROBOT_AUDIO_ON_DEVICE 1

namespace Anki {
namespace Cozmo {
  
#pragma mark -
#pragma mark Animation::Track
  
  template<typename FRAME_TYPE>
  void Animation::Track<FRAME_TYPE>::Init()
  {
#   if DEBUG_ANIMATIONS
    PRINT_NAMED_INFO("Animation.Track.Init", "Initializing %s Track with %lu frames.\n",
                     FRAME_TYPE::GetClassName().c_str(),
                     _frames.size());
#   endif
    
    _frameIter = _frames.begin();
  }
  
  template<typename FRAME_TYPE>
  RobotMessage* Animation::Track<FRAME_TYPE>::GetCurrentStreamingMessage(TimeStamp_t startTime_ms,
                                                                         TimeStamp_t currTime_ms)
  {
    RobotMessage* msg = nullptr;
    
    if(HasFramesLeft()) {
      FRAME_TYPE& currentKeyFrame = GetCurrentKeyFrame();
      if(currentKeyFrame.IsTimeToPlay(startTime_ms, currTime_ms))
      {
        msg = currentKeyFrame.GetStreamMessage();
        if(currentKeyFrame.IsDone()) {
          MoveToNextKeyFrame();
        }
      }
    }
    
    return msg;
  }
  
  template<typename FRAME_TYPE>
  Result Animation::Track<FRAME_TYPE>::AddKeyFrame(const Json::Value &jsonRoot)
  {
    _frames.emplace_back();
    Result lastResult = _frames.back().DefineFromJson(jsonRoot);
    
#   if DEBUG_ANIMATIONS
    PRINT_NAMED_INFO("Animation.Track.AddKeyFrame",
                     "Adding %s keyframe to track to trigger at %dms.\n",
                     _frames.back().GetClassName().c_str(),
                     _frames.back().GetTriggerTime());
#   endif
    
    if(lastResult == RESULT_OK) {
      if(_frames.size() > 1) {
        auto nextToLastFrame = _frames.rbegin();
        ++nextToLastFrame;
        
        if(_frames.back().GetTriggerTime() <= nextToLastFrame->GetTriggerTime()) {
          PRINT_NAMED_ERROR("Animation.Track.AddKeyFrame.BadTriggerTime",
                            "New keyframe must be after the last keyframe.\n");
          _frames.pop_back();
          lastResult = RESULT_FAIL;
        }
      }
    }
    
    return lastResult;
  }
  
#pragma mark -
#pragma mark Animation
  
  Animation::Animation()
  : _isInitialized(false)
  {
    
  }
  
  
  Result Animation::DefineFromJson(const std::string& name, Json::Value &jsonRoot)
  {
    /*
    if(!jsonRoot.isMember("Name")) {
      PRINT_NAMED_ERROR("Animation.DefineFromJson.NoName",
                        "Missing 'Name' field for animation.\n");
      return RESULT_FAIL;
    }
     */
    
    _name = name; //jsonRoot["Name"].asString();
    
    // Clear whatever is in the existing animation
    Clear();
    
    const s32 numFrames = jsonRoot.size();
    for(s32 iFrame = 0; iFrame < numFrames; ++iFrame)
    {
      Json::Value& jsonFrame = jsonRoot[iFrame];
      
      if(!jsonFrame.isMember("Name")) {
        PRINT_NAMED_ERROR("Animation.DefineFromJson.NoFrameName",
                          "Missing 'Name' field for frame %d of '%s' animation.\n",
                          iFrame, _name.c_str());
        return RESULT_FAIL;
      }
      
      const std::string& frameName = jsonFrame["Name"].asString();
      
      Result addResult = RESULT_FAIL;
      
      // Map from string name of frame to which track we want to store it in:
      if(frameName == HeadAngleKeyFrame::GetClassName()) {
        addResult = _headTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == LiftHeightKeyFrame::GetClassName()) {
        addResult = _liftTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == FaceAnimationKeyFrame::GetClassName()) {
        addResult = _faceAnimTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == FacePositionKeyFrame::GetClassName()) {
        addResult = _facePosTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == DeviceAudioKeyFrame::GetClassName()) {
        addResult = _deviceAudioTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == BlinkKeyFrame::GetClassName()) {
        addResult = _blinkTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == RobotAudioKeyFrame::GetClassName()) {
        addResult = _robotAudioTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == BackpackLightsKeyFrame::GetClassName()) {
        addResult = _backpackLightsTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == BodyMotionKeyFrame::GetClassName()) {
        addResult = _bodyPosTrack.AddKeyFrame(jsonFrame);
      } else {
        PRINT_NAMED_ERROR("Animation.DefineFromJson.UnrecognizedFrameName",
                          "Frame %d in '%s' animation has unrecognized name '%s'.\n",
                          iFrame, _name.c_str(), frameName.c_str());
        return RESULT_FAIL;
      }
      
      if(addResult != RESULT_OK) {
        PRINT_NAMED_ERROR("Animation.DefineFromJson.AddKeyFrameFailure",
                          "Adding %s frame %d failed.\n",
                          frameName.c_str(), iFrame);
        return addResult;
      }
      
    } // for each frame
    
    return RESULT_OK;
  }
  
  /*
  Result Animation::AddKeyFrame(const HeadAngleKeyFrame& kf)
  {
    return _headTrack.AddKeyFrame(kf);
  }
   */
  
  // Helper macro for running a given method of all tracks and combining the result
  // in the specified way. To just call a method, use ";" for COMBINE_WITH, or
  // use "&&" or "||" to combine into a single result.
# define ALL_TRACKS(__METHOD__, __COMBINE_WITH__) \
_headTrack.__METHOD__() __COMBINE_WITH__ \
_liftTrack.__METHOD__() __COMBINE_WITH__ \
_faceAnimTrack.__METHOD__() __COMBINE_WITH__ \
_facePosTrack.__METHOD__() __COMBINE_WITH__ \
_deviceAudioTrack.__METHOD__() __COMBINE_WITH__ \
_robotAudioTrack.__METHOD__() __COMBINE_WITH__ \
_backpackLightsTrack.__METHOD__() __COMBINE_WITH__ \
_bodyPosTrack.__METHOD__() __COMBINE_WITH__ \
_blinkTrack.__METHOD__()
  
  Result Animation::Init()
  {
#   if DEBUG_ANIMATIONS
    PRINT_NAMED_INFO("Animation.Init", "Initializing animation '%s'\n", GetName().c_str());
#   endif
    
    _startTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    
    // Initialize "fake" streaming time to the same start time so we can compare
    // to it for determining when its time to stream out a keyframe
    _streamingTime_ms = _startTime_ms;
    
#   if PLAY_ROBOT_AUDIO_ON_DEVICE
    // This prevents us from replaying the same keyframe
    _playedRobotAudio_ms = 0;
#   endif
    
    ALL_TRACKS(Init, ;);
    
    _isInitialized = true;
    
    return RESULT_OK;
  } // Animation::Init()
  
  
  Result Animation::Update(Robot& robot)
  {
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("Animation.Update", "Animation must be initialized before it can be played/updated.\n");
      return RESULT_FAIL;
    }
    
    const TimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    
    // Is it time to play device audio? (using actual basestation time)
    if(_deviceAudioTrack.HasFramesLeft() && 
       _deviceAudioTrack.GetCurrentKeyFrame().IsTimeToPlay(_startTime_ms, currTime_ms)) {
      _deviceAudioTrack.GetCurrentKeyFrame().PlayOnDevice();
      _deviceAudioTrack.MoveToNextKeyFrame();
    }

#   ifndef ANKI_IOS_BUILD
#   if PLAY_ROBOT_AUDIO_ON_DEVICE
    if(_robotAudioTrack.HasFramesLeft())
    {
      const RobotAudioKeyFrame& audioKF = _robotAudioTrack.GetCurrentKeyFrame();
      if((_playedRobotAudio_ms < _startTime_ms + audioKF.GetTriggerTime()) &&
         audioKF.IsTimeToPlay(_startTime_ms,  currTime_ms))
      {
        // TODO: Insert some kind of small delay to simulate latency?
        SoundManager::getInstance()->Play(audioKF.GetSoundName());
        _playedRobotAudio_ms = currTime_ms;
      }
    }
#   endif
#   endif
    
    // FlowControl: Don't send frames if robot has no space for them, and be
    // careful not to overwhel reliable transport either, in terms of bytes or
    // sheer number of messages

    // TODO: define this elsewhere
    const s32 MAX_BYTES_FOR_RELIABLE_TRANSPORT = 2000;
    
    s32 numBytesToSend = std::min(MAX_BYTES_FOR_RELIABLE_TRANSPORT,
                                  robot.GetNumAnimationBytesFree());
    
    s32 numFramesToSend = 10;
    
    while(numFramesToSend-- > 0 && numBytesToSend > 0 && !IsFinished())
    {
#     if DEBUG_ANIMATIONS
      //PRINT_NAMED_INFO("Animation.Update", "%d bytes left to send this Update.\n",
      //                 numBytesToSend);
#     endif
      
      RobotMessage* msg = nullptr;
      
      Result sendResult = RESULT_OK;
      
      // Have to always send an audio frame to keep time, whether that's the next
      // audio sample or a silent frame. This increments "streamingTime"
      // NOTE: Audio frame must be first!
      if(_robotAudioTrack.HasFramesLeft() &&
         _robotAudioTrack.GetCurrentKeyFrame().IsTimeToPlay(_startTime_ms, _streamingTime_ms))
      {
        msg = _robotAudioTrack.GetCurrentKeyFrame().GetStreamMessage();
        if(msg != nullptr) {
          // Still have samples to send, don't increment to the next frame in the track
          //PRINT_NAMED_INFO("Animation.Update", "Streaming AudioSampleKeyFrame.\n");
          robot.SendMessage(*msg, true, SEND_LARGE_KEYFRAMES_HOT);
          numBytesToSend -= msg->GetSize() + sizeof(RobotMessage::ID);
          if(sendResult != RESULT_OK) { return sendResult; }
        } else {
          // No samples left to send for this keyframe. Move to next keyframe,
          // and for now send silence.
          //PRINT_NAMED_INFO("Animation.Update", "Streaming AudioSilenceKeyFrame.\n");
          _robotAudioTrack.MoveToNextKeyFrame();
          robot.SendMessage(_silenceMsg);
          numBytesToSend -= _silenceMsg.GetSize() + sizeof(RobotMessage::ID);
          if(sendResult != RESULT_OK) { return sendResult; }
        }
      } else {
        // No frames left or not time to play next frame yet, so send silence
        //PRINT_NAMED_INFO("Animation.Update", "Streaming AudioSilenceKeyFrame.\n");
        robot.SendMessage(_silenceMsg);
        numBytesToSend -= _silenceMsg.GetSize() + sizeof(RobotMessage::ID);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      // Increment fake "streaming" time, so we can evaluate below whether
      // it's time to stream out any of the other tracks. Note that it is still
      // relative to the same start time.
      _streamingTime_ms += RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
      
      //
      // We are guaranteed to have sent some kind of audio frame at this point.
      // Now send any other frames that are ready, so they will be timed with
      // that audio frame (silent or not).
      //
      // Note that these frames don't actually use up additional slots in the
      // robot's keyframe buffer, so we don't have to decrement numFramesToSend
      // for each one, just once for each audio/silence frame.
      //
      
      msg = _headTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming HeadAngleKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
        sendResult = robot.SendMessage(*msg);
        numBytesToSend -= msg->GetSize() + sizeof(RobotMessage::ID);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      msg = _liftTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming LiftHeightKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
        sendResult = robot.SendMessage(*msg);
        numBytesToSend -= msg->GetSize() + sizeof(RobotMessage::ID);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      msg = _facePosTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming FacePositionKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
        sendResult = robot.SendMessage(*msg);
        numBytesToSend -= msg->GetSize() + sizeof(RobotMessage::ID);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      msg = _faceAnimTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming FaceAnimationKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
        sendResult = robot.SendMessage(*msg, true, SEND_LARGE_KEYFRAMES_HOT);
        numBytesToSend -= msg->GetSize() + sizeof(RobotMessage::ID);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      msg = _blinkTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming BlinkKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
        sendResult = robot.SendMessage(*msg, true);
        numBytesToSend -= msg->GetSize() + sizeof(RobotMessage::ID);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      msg = _backpackLightsTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming BackpackLightsKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
        sendResult = robot.SendMessage(*msg);
        numBytesToSend -= msg->GetSize() + sizeof(RobotMessage::ID);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      msg = _bodyPosTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming BodyMotionKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
        sendResult = robot.SendMessage(*msg);
        numBytesToSend -= msg->GetSize() + sizeof(RobotMessage::ID);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
    } // while(numFramesToSend > 0)
    
    if(IsFinished()) {
      // Send an end-of-animation keyframe
      
#     if DEBUG_ANIMATIONS
      PRINT_NAMED_INFO("Animation.Update", "Streaming EndOfAnimation at t=%dms.\n",
                       _streamingTime_ms - _startTime_ms);
#     endif
      
      MessageAnimKeyFrame_EndOfAnimation endMsg;
      Result sendResult = robot.SendMessage(endMsg);
      if(sendResult != RESULT_OK) { return sendResult; }
    }
    
    return RESULT_OK;
    
  } // Animation::Update()
  
  void Animation::Clear()
  {
    ALL_TRACKS(Clear, ;);
    _isInitialized = false;
  }
  
  bool Animation::IsEmpty() const
  {
    return ALL_TRACKS(IsEmpty, &&);
  }
  
  bool Animation::IsFinished() const
  {
    return !(ALL_TRACKS(HasFramesLeft, ||));
  }
  
} // namespace Cozmo
} // namespace Anki