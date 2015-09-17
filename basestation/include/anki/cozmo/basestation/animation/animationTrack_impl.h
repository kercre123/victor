#include "animationTrack.h"

#include "anki/cozmo/basestation/comms/robot/robotMessages.h"

namespace Anki {
namespace Cozmo {

  template<typename FRAME_TYPE>
  void AnimationTrack<FRAME_TYPE>::Init()
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
  void AnimationTrack<FRAME_TYPE>::MoveToNextKeyFrame()
  {
    ++_frameIter;
  }

  template<typename FRAME_TYPE>
  void AnimationTrack<FRAME_TYPE>::ClearPlayedLiveFrames()
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
  RobotMessage* AnimationTrack<FRAME_TYPE>::GetCurrentStreamingMessage(TimeStamp_t startTime_ms,
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
  RobotMessage* AnimationTrack<ProceduralFaceKeyFrame>::GetCurrentStreamingMessage(TimeStamp_t startTime_ms,
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
  Result AnimationTrack<FRAME_TYPE>::AddKeyFrame(const Json::Value &jsonRoot)
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
  
  template<typename FRAME_TYPE>
  Result AnimationTrack<FRAME_TYPE>::AddKeyFrame(const FRAME_TYPE& keyFrame)
  {
    if(_frames.size() > MAX_FRAMES_PER_TRACK) {
      PRINT_NAMED_ERROR("Animation.Track.AddKeyFrame.TooManyFrames",
                        "There are already %lu frames in %s track. Refusing to add more.",
                        _frames.size(), keyFrame.GetClassName().c_str());
      return RESULT_FAIL;
    }
    
    _frames.emplace_back(keyFrame);
    
    // If we just added the first keyframe (e.g. after deleting the last remaining
    // keyframe in a "Live" track), we need to reset the frameIter to point
    // back to the beginning.
    if(_frames.size() == 1) {
      _frameIter = _frames.begin();
      _lastClearedLiveFrame = _frameIter;
    }
    
    return RESULT_OK;
  }


} // namespace Cozmo
} // namespace Anki
