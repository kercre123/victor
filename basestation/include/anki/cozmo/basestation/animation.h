/**
 * File: animation.h
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


#ifndef ANKI_COZMO_CANNED_ANIMATION_H
#define ANKI_COZMO_CANNED_ANIMATION_H

#include "anki/cozmo/shared/cozmoTypes.h"

#include "anki/common/basestation/jsonTools.h"

#include "anki/cozmo/basestation/keyframe.h"

namespace Anki {
  namespace Cozmo {
    
    // Forward declaration
    class Robot;
    
    class Animation
    {
    public:
      Animation();
      
      Result DefineFromJson(const std::string& name, Json::Value& json);
      
      //Result AddKeyFrame(const HeadAngleKeyFrame& kf);
      
      Result Init();
      Result Update(Robot& robot);
      
      // An animation is Empty if *all* its tracks are empty
      bool IsEmpty() const;
      
      // An animation is finished when none of its track have frames left to play
      bool IsFinished() const;
      
      void Clear();
      
      const std::string& GetName() const { return _name; }
      
    private:
    
      // Internal templated class for storing/accessing various "tracks", which
      // hold different types of KeyFrames.
      template<typename FRAME_TYPE>
      class Track {
      public:
        void Init();
        
        Result AddKeyFrame(const FRAME_TYPE& keyFrame) { _frames.emplace_back(keyFrame); return RESULT_OK; }
        Result AddKeyFrame(const Json::Value& jsonRoot);
        
        // Return the Streaming message for the current KeyFrame if it is time,
        // nullptr otherwise. Also returns nullptr if there are no KeyFrames
        // left in the track.
        RobotMessage* GetCurrentStreamingMessage(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms);
        
        // Get a reference to the current KeyFrame in the track
        FRAME_TYPE& GetCurrentKeyFrame() { return *_frameIter; }
        
        void MoveToNextKeyFrame() { ++_frameIter; }
        
        bool HasFramesLeft() const { return _frameIter != _frames.end(); }
        
        bool IsEmpty() const { return _frames.empty(); }
        
        void Clear() { _frames.clear(); _frameIter = _frames.end(); }
        
      private:
        using FrameList = std::vector<FRAME_TYPE>;
        FrameList                    _frames;
        typename FrameList::iterator _frameIter;
      }; // class Animation::Track
      
      // Name of this animation
      std::string _name;
      
      bool _isInitialized;
      
      // When this animation started playing (was initialized) in milliseconds, in
      // "real" basestation time
      TimeStamp_t _startTime_ms;
      
      // Where we are in the animation in terms of what has been streamed out, since
      // we don't stream in real time. Each time we send an audio frame to the
      // robot (silence or actual audio), this increments by one audio sample
      // length, since that's what keeps time for streaming animations (not a
      // clock)
      TimeStamp_t _streamingTime_ms;
      
      // A reusable "Silence" message for when we have no audio to send robot
      MessageAnimKeyFrame_AudioSilence _silenceMsg;
      
      // All the animation tracks, storing different kinds of KeyFrames
      Track<HeadAngleKeyFrame>      _headTrack;
      Track<LiftHeightKeyFrame>     _liftTrack;
      Track<FaceImageKeyFrame>      _faceImageTrack;
      Track<FacePositionKeyFrame>   _facePosTrack;
      Track<BackpackLightsKeyFrame> _backpackLightsTrack;
      Track<BodyPositionKeyFrame>   _bodyPosTrack;
      
      Track<DeviceAudioKeyFrame>  _deviceAudioTrack;
      Track<RobotAudioKeyFrame>   _robotAudioTrack;
      
    }; // class Animation

  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_ANIMATION_H
