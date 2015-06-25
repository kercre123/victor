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

#include "anki/cozmo/shared/cozmoTypes.h"

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  class Robot;
  class IRobotMessageHandler;
  
  class IKeyFrame
  {
  public:
    IKeyFrame();
    //IKeyFrame(const Json::Value& root);
    
    ~IKeyFrame();
    
    bool IsValid() const { return _isValid; }
    
    bool IsTimeToPlay(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms) const;
    
    TimeStamp_t GetTriggerTime() const { return _triggerTime_ms; }

    Result DefineFromJson(const Json::Value &json);
    
    // Fill some kind of message for streaming and return it. Return nullptr
    // if not available.
    virtual RobotMessage* GetStreamMessage() = 0;
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot) = 0;
    
    //void SetIsValid(bool isValid) { _isValid = isValid; }
    
  private:
    //RobotMessage* _msg;
    TimeStamp_t   _triggerTime_ms;
    bool          _isValid;
  };
  
  class HeadAngleKeyFrame : public IKeyFrame
  {
  public:
    HeadAngleKeyFrame() { }
    
    //HeadAngleKeyFrame(s8 angle_deg, u8 angle_variability_deg, TimeStamp_t duration);
    
    virtual RobotMessage* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("HeadAngleKeyFrame");
      return ClassName;
    }
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
    
  private:
    TimeStamp_t _durationTime_ms;
    s8          _angle_deg;
    u8          _angleVariability_deg;
    
    MessageAnimKeyFrame_HeadAngle _streamHeadMsg;
    
  };
  
  class LiftHeightKeyFrame : public IKeyFrame
  {
  public:
    LiftHeightKeyFrame() { }
    
    virtual RobotMessage* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("LiftHeightKeyFrame");
      return ClassName;
    }
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
    
  private:
    TimeStamp_t _durationTime_ms;
    u8          _height_mm;
    u8          _heightVariability_mm;
    
    MessageAnimKeyFrame_LiftHeight _streamLiftMsg;
    
  };
  
  class DeviceAudioKeyFrame : public IKeyFrame
  {
  public:
    DeviceAudioKeyFrame() { }
    
    // Play sound on device
    void PlayOnDevice();
    
    virtual RobotMessage* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("DeviceAudioKeyFrame");
      return ClassName;
    }
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
    
  private:
    u32 _audioID;
  };
  
  class RobotAudioKeyFrame : public IKeyFrame
  {
  public:
    RobotAudioKeyFrame() { }
    
    virtual RobotMessage* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("RobotAudioKeyFrame");
      return ClassName;
    }
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
  
  private:
    u32 _audioID;
    
    s32 _numSamples;
    s32 _sampleIndex;
    
    MessageAnimKeyFrame_AudioSample  _audioSampleMsg;
  };
  
  class FaceImageKeyFrame : public IKeyFrame
  {
  public:
    FaceImageKeyFrame() { }
    
    virtual RobotMessage* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("FaceImageKeyFrame");
      return ClassName;
    }
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
    
  private:
    u32 _imageID;
    
    MessageAnimKeyFrame_FaceImage _streamMsg;
    
  };
  
  class FacePositionKeyFrame : public IKeyFrame
  {
  public:
    FacePositionKeyFrame() { }
    
    virtual RobotMessage* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("FacePositionKeyFrame");
      return ClassName;
    }
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
    
  private:
    s8 _xcen, _ycen;
    
    MessageAnimKeyFrame_FacePosition _streamMsg;
  };
  
  
  class Animation
  {
  public:
    Animation();
    
    Result DefineFromJson(Json::Value& json);
    
    Result AddKeyFrame(const HeadAngleKeyFrame& kf);

    Result Init(Robot& robot);
    Result Update(Robot& robot);
    
    // An animation is Empty if *all* its tracks are empty
    bool IsEmpty() const;
    
    // An animation is finished when none of its track have frames left to play
    bool IsFinished() const;
    
    void Clear();
    
    const std::string& GetName() const { return _name; }
    
  private:
    
    Result SendSilence(IRobotMessageHandler* msgHandler);
    
    template<typename FRAME_TYPE>
    class Track {
    public:
      enum class Type : u8 {
        HEAD,
        LIFT,
        FACE_IMAGE,
        FACE_POSITION,
        DEVICE_AUDIO,
        ROBOT_AUDIO
      };
      
      void Init();
      
      Result AddKeyFrame(const FRAME_TYPE& keyFrame) { _frames.emplace_back(keyFrame); return RESULT_OK; }
      
      Result AddKeyFrame(const Json::Value& jsonRoot);
      
      RobotMessage* GetNextMessage(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms);
      
      FRAME_TYPE& GetNextFrame() { return *_frameIter; }
      
      void Increment() { ++_frameIter; }
      
      bool HasFramesLeft() const { return _frameIter != _frames.end(); }
      
      bool IsEmpty() const { return _frames.empty(); }
      
      void Clear() { _frames.clear(); _frameIter = _frames.end(); }
      
    private:
      using FrameList = std::vector<FRAME_TYPE>;
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
    Track<HeadAngleKeyFrame>    _headTrack;
    Track<LiftHeightKeyFrame>   _liftTrack;
    Track<FaceImageKeyFrame>    _faceImageTrack;
    Track<FacePositionKeyFrame> _facePosTrack;
    
    Track<DeviceAudioKeyFrame>  _deviceAudioTrack;
    Track<RobotAudioKeyFrame>   _robotAudioTrack;
    
  }; // class Animation
  
  
  
  
  class CannedAnimationContainer
  {
  public:
    
    
    CannedAnimationContainer();
    
    Result DefineHardCoded(); // called at construction
    
    Result DefineFromJson(Json::Value& jsonRoot);
    
    Result AddAnimation(const std::string& name);
    
    Animation* GetAnimation(const std::string& name);
    
    AnimationID_t GetID(const std::string& name) const;
    
    // Sets an animation to be streamed and how many times to stream it.
    // Use zero to play the animation indefinitely.
    // Actual streaming occurs on calls to Update().
    Result SetStreamingAnimation(const std::string& name, u32 numLoops = 1);
    
    // If any animation is set for streaming and isn't done yet, stream it.
    Result Update(Robot& robot);
    
    // TODO: Add a way to ask how long an animation is
    //u16 GetLengthInMilliSeconds(const std::string& name) const;
    
    void Clear();
    
  private:
    
    std::map<std::string, std::pair<AnimationID_t, Animation> > _animations;
    
    Animation* _streamingAnimation;
    
    u32 _numLoops;
    u32 _loopCtr;
    
  }; // class Animation

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H
