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
#include "anki/cozmo/basestation/proceduralFace.h"

#include <list>
#include <queue>

namespace Anki {
  namespace Cozmo {

    // Forward declaration
    class Robot;

    class Animation
    {
    public:

      Animation(const std::string& name = "");

      // For reading canned animations from files
      Result DefineFromJson(const std::string& name, Json::Value& json);

      // For defining animations at runtime (e.g. procedural faces)
      template<class KeyFrameType>
      Result AddKeyFrame(const KeyFrameType& kf);

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
      template<class FRAME_TYPE>
      class Track {
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
        
        std::queue<typename FrameList::iterator> _liveFramesToClear;
        
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
      //Track<FaceImageKeyFrame>      _faceImageTrack;
      Track<FaceAnimationKeyFrame>  _faceAnimTrack;
      Track<ProceduralFaceKeyFrame> _proceduralFaceTrack;
      Track<FacePositionKeyFrame>   _facePosTrack;
      Track<BlinkKeyFrame>          _blinkTrack;
      Track<BackpackLightsKeyFrame> _backpackLightsTrack;
      Track<BodyMotionKeyFrame>     _bodyPosTrack;
      Track<DeviceAudioKeyFrame>    _deviceAudioTrack;
      Track<RobotAudioKeyFrame>     _robotAudioTrack;

      template<class KeyFrameType>
      Track<KeyFrameType>& GetTrack();
      
      // TODO: Remove this once we aren't playing robot audio on the device
      TimeStamp_t _playedRobotAudio_ms;

      bool _endOfAnimationSent;
      
      ProceduralFace _proceduralFace;
      MessageAnimKeyFrame_FaceImage _proceduralFaceStreamMsg;
      
      bool BufferMessageToSend(RobotMessage* msg);
      Result SendBufferedMessages(Robot& robot);
      
      bool AllTracksBuffered() const;
      std::list<RobotMessage*> _sendBuffer;
      s32 _numBytesToSend;
      
      // Send larger keyframes "hot" for reliable transport (this includes
      // audio samples and face images)
      static const bool SEND_LARGE_KEYFRAMES_HOT = false;

      // "Flow control" for not overrunning reliable transport in a single
      // update tick
      static const s32 MAX_BYTES_FOR_RELIABLE_TRANSPORT;
      
    }; // class Animation

    template<class KeyFrameType>
    Result Animation::AddKeyFrame(const KeyFrameType& kf)
    {
      Result addResult = GetTrack<KeyFrameType>().AddKeyFrame(kf);
      if(RESULT_OK != addResult) {
        PRINT_NAMED_ERROR("Animiation.AddKeyFrame.Failed", "");
      } else {
        // If we add a keyframe after initialization (at which time this animation
        // could have been empty), make sure to mark that we haven't yet sent
        // end of animation.
        _endOfAnimationSent = false;
      }
      
      return addResult;
    }
    
    template<typename FRAME_TYPE>
    Result Animation::Track<FRAME_TYPE>::AddKeyFrame(const FRAME_TYPE& keyFrame)
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
      }
      
      return RESULT_OK;
    }
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_ANIMATION_H
