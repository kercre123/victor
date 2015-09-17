/**
 * File: animationTrack.h
 *
 * Authors: Andrew Stein
 * Created: 2015-09-17
 *
 * Description:
 *    Templated calss for storing different animation "tracks", which each
 *    hold a different type of keyframe.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Anki_Cozmo_Animation_Track_H__
#define __Anki_Cozmo_Animation_Track_H__

#include <list>

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  class RobotMessage;
    
  template<class FRAME_TYPE>
  class AnimationTrack {
  public:
    static const size_t MAX_FRAMES_PER_TRACK = 100;
    
    void Init();
    
    Result AddKeyFrame(const FRAME_TYPE& keyFrame);
    Result AddKeyFrame(const Json::Value& jsonRoot);
    
    // Return the Streaming message for the current KeyFrame if it is time,
    // nullptr otherwise. Also returns nullptr if there are no KeyFrames
    // left in the track.
    RobotMessage* GetCurrentStreamingMessage(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms);
    
    // Get a reference to the current KeyFrame in the track
    FRAME_TYPE& GetCurrentKeyFrame() { return *_frameIter; }
    
    void MoveToNextKeyFrame();
    
    bool HasFramesLeft() const { return _frameIter != _frames.end(); }
    
    bool IsEmpty() const { return _frames.empty(); }
    
    void Clear() { _frames.clear(); _frameIter = _frames.end(); }
    
    void ClearPlayedLiveFrames();
    
  private:
    using FrameList = std::list<FRAME_TYPE>;
    FrameList                    _frames;
    typename FrameList::iterator _frameIter;
    typename FrameList::iterator _lastClearedLiveFrame;
    
  }; // class AnimationTrack

} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Animation_Track_H__


