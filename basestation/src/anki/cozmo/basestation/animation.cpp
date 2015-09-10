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
#include "anki/cozmo/shared/cozmoEngineConfig.h"

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
  
  const s32 Animation::MAX_BYTES_FOR_RELIABLE_TRANSPORT = (1000/2) * BS_TIME_STEP; // Don't send more than 1000 bytes every 2ms
  
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
    _lastClearedLiveFrame = _frameIter;
  }
  
  template<typename FRAME_TYPE>
  void Animation::Track<FRAME_TYPE>::MoveToNextKeyFrame()
  {
    ++_frameIter;
  }
  
  template<typename FRAME_TYPE>
  void Animation::Track<FRAME_TYPE>::ClearPlayedLiveFrames()
  {
#   if DEBUG_ANIMATIONS
    s32 numCleared = 0;
#   endif
    
    while(_lastClearedLiveFrame != _frameIter) {
      if(_lastClearedLiveFrame->IsLive()){
        _lastClearedLiveFrame = _frames.erase(_lastClearedLiveFrame);
#       if DEBUG_ANIMATIONS
        ++numCleared;
#       endif
      } else {
        ++_lastClearedLiveFrame;
      }
    }
   
#   if DEBUG_ANIMATIONS
    if(numCleared > 0) {
      PRINT_NAMED_INFO("Animation.Track.ClearPlayedLiveFrames", "Cleared %d frames.", numCleared);
    }
#   endif
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
  
  // Specialization for ProceduralFace track because it needs look-back for interpolation
  template<>
  RobotMessage* Animation::Track<ProceduralFaceKeyFrame>::GetCurrentStreamingMessage(TimeStamp_t startTime_ms,
                                                                                     TimeStamp_t currTime_ms)
  {
    RobotMessage* msg = nullptr;
    
    if(HasFramesLeft()) {
      ProceduralFaceKeyFrame& currentKeyFrame = GetCurrentKeyFrame();
      if(currentKeyFrame.IsTimeToPlay(startTime_ms, currTime_ms))
      {
        if(currentKeyFrame.IsLive()) {
          // The AnimationStreamer will take care of interpolation for live
          // live streaming
          // TODO: Maybe we could also do it here somehow?
          msg = currentKeyFrame.GetStreamMessage();
          
        } else {
          auto nextIter = _frameIter;
          ++nextIter;
          if(nextIter != _frames.end()) {
            // If we have another frame coming, use it to interpolate.
            // This will only be "done" once 
            msg = currentKeyFrame.GetInterpolatedStreamMessage(*nextIter);
          } else {
            // Otherwise, we'll just send the last frame
            msg = currentKeyFrame.GetStreamMessage();
          }
        }
          
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
    Result lastResult = AddKeyFrame(FRAME_TYPE());
    if(RESULT_OK != lastResult) {
      return lastResult;
    }
    
    lastResult = _frames.back().DefineFromJson(jsonRoot);
    
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
  
  Animation::Animation(const std::string& name)
  : _name(name)
  , _isInitialized(false)
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
      } else if(frameName == ProceduralFaceKeyFrame::GetClassName()) {
        addResult = _proceduralFaceTrack.AddKeyFrame(jsonFrame);
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
  
  template<>
  Animation::Track<HeadAngleKeyFrame>& Animation::GetTrack() {
    return _headTrack;
  }
  
  template<>
  Animation::Track<LiftHeightKeyFrame>& Animation::GetTrack() {
    return _liftTrack;
  }
  
  template<>
  Animation::Track<FaceAnimationKeyFrame>& Animation::GetTrack() {
    return _faceAnimTrack;
  }
  
  template<>
  Animation::Track<FacePositionKeyFrame>& Animation::GetTrack() {
    return _facePosTrack;
  }
  
  template<>
  Animation::Track<DeviceAudioKeyFrame>& Animation::GetTrack() {
    return _deviceAudioTrack;
  }
  
  template<>
  Animation::Track<RobotAudioKeyFrame>& Animation::GetTrack() {
    return _robotAudioTrack;
  }
  
  template<>
  Animation::Track<BackpackLightsKeyFrame>& Animation::GetTrack() {
    return _backpackLightsTrack;
  }
  
  template<>
  Animation::Track<BodyMotionKeyFrame>& Animation::GetTrack() {
    return _bodyPosTrack;
  }
  
  template<>
  Animation::Track<BlinkKeyFrame>& Animation::GetTrack() {
    return _blinkTrack;
  }
  
  template<>
  Animation::Track<ProceduralFaceKeyFrame>& Animation::GetTrack() {
    return _proceduralFaceTrack;
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
_proceduralFaceTrack.__METHOD__() __COMBINE_WITH__ \
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
    
    if(!_sendBuffer.empty()) {
      PRINT_NAMED_WARNING("Animation.Init", "Expecting SendBuffer to be empty. Will clear.\n");
      _sendBuffer.clear();
    }
    
    // If this is an empty (e.g. live) animation, there is no need to
    // send end of animation keyframe until we actually send a keyframe
    _endOfAnimationSent = IsEmpty();
    
    _isInitialized = true;
    
    return RESULT_OK;
  } // Animation::Init()
  
  bool Animation::BufferMessageToSend(RobotMessage* msg)
  {
    if(msg != nullptr) {
      _sendBuffer.push_back(msg);
      return true;
    }
    return false;
  }
  
  Result Animation::SendBufferedMessages(Robot& robot)
  {
#   if DEBUG_ANIMATIONS
    s32 numSent = 0;
#   endif
    
    // Empty out anything waiting in the send buffer:
    RobotMessage* msg = nullptr;
    while(!_sendBuffer.empty())
    {
#     if DEBUG_ANIMATIONS
      PRINT_NAMED_INFO("Animation.SendBufferedMessages",
                       "Send buffer length=%lu.\n", _sendBuffer.size());
#     endif
     
      msg = _sendBuffer.front();
      const s32 numBytesRequired = msg->GetSize() + sizeof(RobotMessage::ID);
      if(numBytesRequired <= _numBytesToSend) {
        Result sendResult = robot.SendMessage(*msg);
        if(sendResult != RESULT_OK) {
          return sendResult;
        }
        
#       if DEBUG_ANIMATIONS
        ++numSent;
#       endif
        
        _numBytesToSend -= numBytesRequired;
        
        // Increment total number of bytes streamed to robot
        robot.IncrementNumAnimationBytesStreamed(numBytesRequired);
        
        _sendBuffer.pop_front();
      } else {
        // Out of bytes to send, continue on next Update()
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.SendBufferedMessages",
                         "Sent %d messages, but ran out of bytes to send from "
                         "buffer. %lu remain, so will continue next Update().",
                         numSent, _sendBuffer.size());
#       endif
        return RESULT_OK;
      }
    }
    
    // Sanity check
    // If we got here, we've finished streaming out everything in the send
    // buffer -- i.e., all the frames associated with the last audio keyframe
    assert(_numBytesToSend >= 0);
    assert(_sendBuffer.empty());
    
    // If there's nothing waiting to go out, we are safe to clean out Live frames
    // that have already been seen
    ALL_TRACKS(ClearPlayedLiveFrames, ;);
    
#   if DEBUG_ANIMATIONS
    if(numSent > 0) {
      PRINT_NAMED_INFO("Animation.SendBufferedMessages.Sent",
                       "Sent %d messages, %lu remain in buffer.",
                       numSent, _sendBuffer.size());
    }
#   endif
    
    return RESULT_OK;
  }
  
  Result Animation::Update(Robot& robot)
  {
    Result lastResult = RESULT_OK;
    
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("Animation.Update", "Animation must be initialized before it can be played/updated.\n");
      return RESULT_FAIL;
    }
    
    const TimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    
//#   if DEBUG_ANIMATIONS
//    PRINT_NAMED_INFO("Animation.Update", "Current time = %dms\n", currTime_ms);
//#   endif
    
    // Is it time to play device audio? (using actual basestation time)
    if(_deviceAudioTrack.HasFramesLeft() &&
       _deviceAudioTrack.GetCurrentKeyFrame().IsTimeToPlay(_startTime_ms, currTime_ms)) {
      _deviceAudioTrack.GetCurrentKeyFrame().PlayOnDevice();
      _deviceAudioTrack.MoveToNextKeyFrame();
    }
    
    // Compute number of bytes free in robot animation buffer.
    // This is a lower bound since this is computed from a delayed measure
    // of the number of animation bytes already played on the robot.
    s32 totalNumBytesStreamed = robot.GetNumAnimationBytesStreamed();
    s32 totalNumBytesPlayed = robot.GetNumAnimationBytesPlayed();
    bool overflow = (totalNumBytesStreamed < 0) && (totalNumBytesPlayed > 0);
    assert((totalNumBytesStreamed >= totalNumBytesPlayed) || overflow);
    
    s32 minBytesFreeInRobotBuffer = KEYFRAME_BUFFER_SIZE - (totalNumBytesStreamed - totalNumBytesPlayed);
    if (overflow) {
      // Computation for minBytesFreeInRobotBuffer still works out in overflow case
      PRINT_NAMED_INFO("Animation.Update.BytesStreamedOverflow", "free %d (streamed = %d, played %d)", minBytesFreeInRobotBuffer, totalNumBytesStreamed, totalNumBytesPlayed);
    }
    assert(minBytesFreeInRobotBuffer >= 0);
    
    // Reset the number of bytes we can send each Update() as a form of
    // flow control: Don't send frames if robot has no space for them, and be
    // careful not to overwhelm reliable transport either, in terms of bytes or
    // sheer number of messages. These get decremenged on each call to
    // SendBufferedMessages() below
    _numBytesToSend = std::min(Animation::MAX_BYTES_FOR_RELIABLE_TRANSPORT,
                               minBytesFreeInRobotBuffer);
    
    
    // Send anything still left in the buffer after last Update()
    lastResult = SendBufferedMessages(robot);
    if(RESULT_OK != lastResult) {
      PRINT_NAMED_ERROR("Animation.Update.SendBufferedMessagesFailed", "\n");
      return lastResult;
    }
    
    // Add more stuff to the send buffer. Note that we are not counting individual
    // keyframes here, but instead _audio_ keyframes (with which we will buffer
    // any co-timed keyframes from other tracks).
    while(_sendBuffer.empty() && !AllTracksBuffered())
    {
#     if DEBUG_ANIMATIONS
      //PRINT_NAMED_INFO("Animation.Update", "%d bytes left to send this Update.\n",
      //                 numBytesToSend);
#     endif
      
      // Have to always send an audio frame to keep time, whether that's the next
      // audio sample or a silent frame. This increments "streamingTime"
      // NOTE: Audio frame must be first!
      if(_robotAudioTrack.HasFramesLeft() &&
         _robotAudioTrack.GetCurrentKeyFrame().IsTimeToPlay(_startTime_ms, _streamingTime_ms))
      {
        if(!BufferMessageToSend(_robotAudioTrack.GetCurrentKeyFrame().GetStreamMessage()))
        {
          // No samples left to send for this keyframe. Move to next keyframe,
          // and for now send silence.
          //PRINT_NAMED_INFO("Animation.Update", "Streaming AudioSilenceKeyFrame.\n");
          _robotAudioTrack.MoveToNextKeyFrame();
          BufferMessageToSend(&_silenceMsg);
        }
      } else {
        // No frames left or not time to play next frame yet, so send silence
        //PRINT_NAMED_INFO("Animation.Update", "Streaming AudioSilenceKeyFrame.\n");
        BufferMessageToSend(&_silenceMsg);
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
      
      if(BufferMessageToSend(_headTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming HeadAngleKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
      }
      
      if(BufferMessageToSend(_liftTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming LiftHeightKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
      }
      
      if(BufferMessageToSend(_facePosTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming FacePositionKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
      }
      
      if(BufferMessageToSend(_faceAnimTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming FaceAnimationKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
      }
      
      if(BufferMessageToSend(_proceduralFaceTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming ProceduralFaceKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
      }
    
      if(BufferMessageToSend(_blinkTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming BlinkKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
      }
      
      if(BufferMessageToSend(_backpackLightsTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming BackpackLightsKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
      }
      
      if(BufferMessageToSend(_bodyPosTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
#       if DEBUG_ANIMATIONS
        PRINT_NAMED_INFO("Animation.Update", "Streaming BodyMotionKeyFrame at t=%dms.\n",
                         _streamingTime_ms - _startTime_ms);
#       endif
      }
      
      // Send out as much as we can from the send buffer. If we manage to send
      // the entire buffer out, we will proceed with putting more into the buffer
      // the next time through this while loop. Otherwise, we will exit the while
      // loop and continue trying to empty the buffer on the next Update().
      // Doing this guarantees we don't try to buffer another message pointer from
      // the same frame before sending the last one (which is important because we
      // re-use message structs inside the keyframes and don't want pointers
      // getting reassigned before they get sent out!)
      lastResult = SendBufferedMessages(robot);
      if(RESULT_OK != lastResult) {
        PRINT_NAMED_ERROR("Animation.Update.SendBufferedMessagesFailed", "\n");
        return lastResult;
      }
      
    } // while(buffering frames)
    
#   if PLAY_ROBOT_AUDIO_ON_DEVICE && !defined(ANKI_IOS_BUILD)
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
    
    // Send an end-of-animation keyframe when done
    if(AllTracksBuffered() && _sendBuffer.empty() && !_endOfAnimationSent)
    {
#     if DEBUG_ANIMATIONS
      static TimeStamp_t lastEndOfAnimTime = 0;
      PRINT_NAMED_INFO("Animation.Update", "Streaming EndOfAnimation at t=%dms.",
                       _streamingTime_ms - _startTime_ms);
      
      if(_streamingTime_ms - _startTime_ms == lastEndOfAnimTime) {
        PRINT_NAMED_ERROR("Animation.Update", "Already sent end of animatino at t=%dms.",
                          lastEndOfAnimTime);
      }
      lastEndOfAnimTime = _streamingTime_ms - _startTime_ms;
#     endif
      
      MessageAnimKeyFrame_EndOfAnimation endMsg;
      lastResult = robot.SendMessage(endMsg);
      if(lastResult != RESULT_OK) { return lastResult; }
      _endOfAnimationSent = true;
      
      // Increment running total of bytes streamed
      robot.IncrementNumAnimationBytesStreamed(endMsg.GetSize() + sizeof(RobotMessage::ID));
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
  
  bool Animation::AllTracksBuffered() const
  {
    return !(ALL_TRACKS(HasFramesLeft, ||));
  }
  
  bool Animation::IsFinished() const
  {
    return _endOfAnimationSent && AllTracksBuffered() && _sendBuffer.empty();
  }
  
} // namespace Cozmo
} // namespace Anki
