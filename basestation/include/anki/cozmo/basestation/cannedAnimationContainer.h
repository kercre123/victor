/**
 * File: cannedAnimationContainer.h
 *
 * Authors: Andrew Stein
 * Created: 2014-10-22
 *
 * Description: Container for hard-coded or json-defined "canned" animations
 *              stored on the basestation and send-able to the physical robot.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#ifndef ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H
#define ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H

#include "anki/cozmo/basestation/comms/robot/robotMessages.h"

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  class Robot;
  class IRobotMessageHandler;
  
  class AnimationStreamer {
    
    
  };
  
  class IKeyFrame
  {
  public:
    IKeyFrame();
    IKeyFrame(const Json::Value& root);
    
    ~IKeyFrame();
    
    bool IsValid() const { return _isValid; }
    
    static const u32 SAMPLE_LENGTH_MS = 33;
    
    Result ReadFromJson(const Json::Value &json);
    
    bool IsTimeToPlay(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms) const;
    
    TimeStamp_t GetTriggerTime() const { return _triggerTime_ms; }
    
    virtual RobotMessage* GetMessage() = 0; // { return &_msg; }
    
        //virtual RobotMessage* GetMessage() = 0;
    //virtual Result Send(IRobotMessageHandler* msgHandler) const = 0;
    
  private:
    //RobotMessage* _msg;
    TimeStamp_t   _triggerTime_ms;
    bool          _isValid;
  };
  
  class DeviceAudioKeyFrame : public IKeyFrame
  {
  public:
    // Play sound on device
    void PlayOnDevice();
    
    virtual RobotMessage* GetMessage() override { return nullptr; }
  };
  
  class RobotAudioKeyFrame : public IKeyFrame
  {
  public:

    // Returns nullptr when no more samples are available
    MessageAnimKeyFrame_AudioSample* GetAudioSampleMessage();
    
  private:
    MessageAnimKeyFrame_PlayAudioID* _idMsg;
    MessageAnimKeyFrame_AudioSample  _audioSampleMsg;
    s32 _numSamples;
    s32 _sampleIndex;
  };
  
  class FaceImageKeyFrame : public IKeyFrame
  {
  public:
    
    // Override ReadFromJson() to turn stored image into a FaceImage keyframe
    // instead of a FaceID message
    Result ReadFromJson(const Json::Value &json);
    
  };
  
  class Animation
  {
  public:
    Animation();
    
    Result DefineFromJson(Json::Value& json);

    Result Init(Robot& robot);
    Result Update(Robot& robot);
    
  private:
    
    Result SendSilence(IRobotMessageHandler* msgHandler);
    
    template<typename FRAME_TYPE>
    class Track {
    public:
      
      void Init();
      Result AddKeyFrame(const Json::Value& jsonRoot);
      
      RobotMessage* GetNextMessage(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms);
      
      FRAME_TYPE& GetNextFrame() { return *_frameIter; }
      
      void Increment() { ++_frameIter; }
      bool HasFramesLeft() const { _frameIter != _frames.end(); }
      
    private:
      using FrameList = std::vector<IKeyFrame>;
      FrameList                    _frames;
      typename FrameList::iterator _frameIter;
    };
    
    // Name of this animation
    std::string _name;
    
    // When this animation started playing (was initialized) in milliseconds
    TimeStamp_t _startTime_ms;
    
    // A reusable "Silence" message for when we have no audio to send robot
    MessageAnimKeyFrame_AudioSilence _silenceMsg;
    
    // All the animation tracks, storing different kinds of KeyFrames
    Track<IKeyFrame>         _headTrack;
    Track<IKeyFrame>         _liftTrack;
    Track<FaceImageKeyFrame> _faceImageTrack;
    Track<IKeyFrame>         _facePosTrack;
    
    Track<DeviceAudioKeyFrame>  _deviceAudioTrack;
    Track<RobotAudioKeyFrame>   _robotAudioTrack;
    
  }; // class Animation
  
  
  
  
  class CannedAnimationContainer
  {
  public:
    
    class KeyFrameList
    {
    public:
      ~KeyFrameList();
      
      void AddKeyFrame(RobotMessage* msg);
      
      bool IsEmpty() const {
        return _keyFrameMessages.empty();
      }
      
      const std::vector<RobotMessage*>& GetMessages() const {
        return _keyFrameMessages;
      }
      
    private:
      
      // TODO: Store some kind of KeyFrame wrapper that holds Message* instead of Message* directly
      std::vector<RobotMessage*> _keyFrameMessages;
    }; // class KeyFrameList
    
    
    CannedAnimationContainer();
    
    Result DefineHardCoded(); // called at construction
    
    Result DefineFromJson(Json::Value& jsonRoot);
    
    Result AddAnimation(const std::string& name);
    
    KeyFrameList* GetKeyFrameList(const std::string& name);
    
    s32 GetID(const std::string& name) const;
    
    // TODO: Add a way to ask how long an animation is
    //u16 GetLengthInMilliSeconds(const std::string& name) const;
    
    // Is there a better way to do this?
    Result Send(RobotID_t robotID, IRobotMessageHandler* msgHandler);
    
    void Clear();
    
  private:
    
    std::map<std::string, std::pair<s32, KeyFrameList> > _animations;
    
  }; // class Animation

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H
