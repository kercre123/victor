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

#include "anki/util/logging/logging.h"

#include "anki/common/basestation/utils/timer.h"

//#include <cassert>

#define DEBUG_ANIMATIONS 1

namespace Anki {
namespace Cozmo {
  
#pragma mark -
#pragma mark Animation::Track
  
  template<typename FRAME_TYPE>
  void Animation::Track<FRAME_TYPE>::Init()
  {
#   if DEBUG_ANIMATIONS
    PRINT_NAMED_INFO("Animation.Track.Init", "Initializing %s Track.\n",
                     FRAME_TYPE::GetClassName().c_str());
#   endif
    
    _frameIter = _frames.begin();
  }
  
  template<typename FRAME_TYPE>
  RobotMessage* Animation::Track<FRAME_TYPE>::GetNextMessage(TimeStamp_t startTime_ms,
                                                             TimeStamp_t currTime_ms)
  {
    RobotMessage* msg = nullptr;
    
    if(HasFramesLeft() && GetNextFrame().IsTimeToPlay(startTime_ms, currTime_ms))
    {
      msg = GetNextFrame().GetStreamMessage();
      Increment();
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
        auto nextToLastFrame = _frames.rend();
        --nextToLastFrame;
        
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
      } else if(frameName == FaceImageKeyFrame::GetClassName()) {
        addResult = _faceImageTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == FacePositionKeyFrame::GetClassName()) {
        addResult = _facePosTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == DeviceAudioKeyFrame::GetClassName()) {
        addResult = _deviceAudioTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == RobotAudioKeyFrame::GetClassName()) {
        addResult = _robotAudioTrack.AddKeyFrame(jsonFrame);
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
_faceImageTrack.__METHOD__() __COMBINE_WITH__ \
_facePosTrack.__METHOD__() __COMBINE_WITH__ \
_deviceAudioTrack.__METHOD__() __COMBINE_WITH__ \
_robotAudioTrack.__METHOD__()
  
  Result Animation::Init()
  {
#   if DEBUG_ANIMATIONS
    PRINT_NAMED_INFO("Animation.Init", "Initializing animation '%s'\n", GetName().c_str());
#   endif
    
    _startTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    
    // Initialize "fake" streaming time to the same start time so we can compare
    // to it for determining when its time to stream out a keyframe
    _streamingTime_ms = _startTime_ms;
    
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
       _deviceAudioTrack.GetNextFrame().IsTimeToPlay(_startTime_ms, currTime_ms)) {
      _deviceAudioTrack.GetNextFrame().PlayOnDevice();
      _deviceAudioTrack.Increment();
    }
    
    // Don't send frames if robot has no space for them
    //const bool isRobotReadyForFrames = !robot.IsAnimationBufferFull();
    s32 numFramesToSend = robot.GetNumAnimationFramesFree();
    
    while(numFramesToSend-- > 0 && !IsEmpty())
    {
      RobotMessage* msg = nullptr;
      
      // Have to always send an audio frame to keep time, whether that's the next
      // audio sample or a silent frame. This increments "streamingTime"
      // NOTE: Audio frame must be first!
      if(_robotAudioTrack.HasFramesLeft())
      {
        msg = _robotAudioTrack.GetNextFrame().GetStreamMessage();
        if(msg != nullptr) {
          // Still have samples to send, don't increment to the next frame in the track
          robot.SendMessage(*msg);
          
        } else {
          // No samples left to send for this keyframe. Move to next, and for now
          // send silence.
          _robotAudioTrack.Increment();
          robot.SendMessage(_silenceMsg);
        }
      } else {
        // No frames left or not time to play next frame yet, so send silence
        robot.SendMessage(_silenceMsg);
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
      
      Result sendResult = RESULT_OK;
      
      msg = _headTrack.GetNextMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
        sendResult = robot.SendMessage(*msg);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      msg = _liftTrack.GetNextMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
        sendResult = robot.SendMessage(*msg);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      msg = _facePosTrack.GetNextMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
        sendResult = robot.SendMessage(*msg);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      msg = _faceImageTrack.GetNextMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
        sendResult = robot.SendMessage(*msg);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      
    } // while(numFramesToSend > 0)
    
    
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