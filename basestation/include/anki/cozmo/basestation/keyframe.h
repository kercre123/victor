/**
 * File: keyframe.h
 *
 * Authors: Andrew Stein
 * Created: 2015-06-25
 *
 * Description: 
 *   Defines the various KeyFrames used to store an animation on the
 *   the robot, all of which inherit from a common interface, 
 *   IKeyFrame.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef ANKI_COZMO_CANNED_KEYFRAME_H
#define ANKI_COZMO_CANNED_KEYFRAME_H

#include "anki/common/basestation/colorRGBA.h"
#include "anki/cozmo/basestation/proceduralFace.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/types/ledTypes.h"
#include "clad/audio/audioEventTypes.h"
#include "util/random/randomGenerator.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  namespace RobotInterface {
  class EngineToRobot;
  enum class EngineToRobotTag : uint8_t;
  }
  
  class Robot;
  class IRobotMessageHandler;
    
  // IKeyFrame defines an abstract interface for all KeyFrames below.
  class IKeyFrame
  {
  public:
    static const u32 SAMPLE_LENGTH_MS = 33;
    
    IKeyFrame();
    //IKeyFrame(const Json::Value& root);
    ~IKeyFrame();
    
    // Returns true if the animation's time has reached frame's "trigger" time
    bool IsTimeToPlay(TimeStamp_t animationTime_ms) const;
    // Returns true if current time has reached frame's "trigger" time, relative
    // to the given start time
    bool IsTimeToPlay(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms) const;
    
    // Returns the time to trigger whatever change is implied by the KeyFrame
    TimeStamp_t GetTriggerTime() const { return _triggerTime_ms; }
    
    // Set the triggert time, relative to the start time of track the animation
    // is playing in
    void SetTriggerTime(TimeStamp_t triggerTime_ms) { _triggerTime_ms = triggerTime_ms; }
    
    // Set all members from Json. Calls virtual SetMembersFromJson() method so subclasses can specify how to
    // populate their members. Second argument is used to print nicer debug strings if something goes wrong
    Result DefineFromJson(const Json::Value &json, const std::string& animNameDebug = "");
    
    // Fill some kind of message for streaming and return it. Return nullptr
    // if not available.
    virtual RobotInterface::EngineToRobot* GetStreamMessage() = 0;
    
    // Whether or not this KeyFrame is "done" after calling GetStreamMessage().
    // Override for special keyframes that need to keep parceling out data into
    // multiple returned messages.
    virtual bool IsDone() { return true; }
    
  protected:
    
    // Populate members from Json
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug = "") = 0;
    
    TimeStamp_t GetCurrentTime() const { return _currentTime_ms; }
    
    // Increments member currentTime_ms by SAMPLE_LENGTH_MS and checks it against durationTime_ms.
    // Once currentTime_ms >= durationTime, it gets reset to 0 to be ready to call again.
    bool IsDoneHelper(TimeStamp_t durationTime_ms);
    
    //void SetIsValid(bool isValid) { _isValid = isValid; }
    
    Util::RandomGenerator& GetRNG() const;
    
  private:
    
    // A random number generator for all keyframes to share (for adding variability)
    static Util::RandomGenerator sRNG;

    TimeStamp_t   _triggerTime_ms = 0;
    TimeStamp_t   _currentTime_ms = 0;
    
  }; // class IKeyFrame
  
  inline Util::RandomGenerator& IKeyFrame::GetRNG() const {
    return sRNG;
  }
  
  
  // A HeadAngleKeyFrame specifies the time to _start_ moving the head towards
  // a given angle (with optional variation), and how long to take to get there.
  class HeadAngleKeyFrame : public IKeyFrame
  {
  public:
    HeadAngleKeyFrame() { }
    
    HeadAngleKeyFrame(s8 angle_deg, u8 angle_variability_deg, TimeStamp_t duration_ms);
    
    virtual RobotInterface::EngineToRobot* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("HeadAngleKeyFrame");
      return ClassName;
    }
    
    virtual bool IsDone() override;
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug = "") override;
    
  private:
    TimeStamp_t _durationTime_ms;
    s8          _angle_deg;
    u8          _angleVariability_deg;
    
    AnimKeyFrame::HeadAngle _streamHeadMsg;
    
  }; // class HeadAngleKeyFrame
  
  
  // A LiftHeightKeyFrame specifies the time to _start_ moving the lift towards
  // a given height (with optional variation), and how long to take to get there.
  class LiftHeightKeyFrame : public IKeyFrame
  {
  public:
    LiftHeightKeyFrame() { }
    LiftHeightKeyFrame(u8 height_mm, u8 heightVariability_mm, TimeStamp_t duration_ms);
    
    virtual RobotInterface::EngineToRobot* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("LiftHeightKeyFrame");
      return ClassName;
    }
    
    virtual bool IsDone() override;
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug = "") override;
    
  private:
    TimeStamp_t _durationTime_ms;
    u8          _height_mm;
    u8          _heightVariability_mm;
    
    AnimKeyFrame::LiftHeight _streamLiftMsg;
    
  }; // class LiftHeightKeyFrame
    
    
  // A DeviceAudioKeyFrame references a single "sound" to be played on the
  // device directly. It is not streamed at all, and thus its GetStreamMessage()
  // method always returns nullptr.
  class DeviceAudioKeyFrame : public IKeyFrame
  {
  public:
    DeviceAudioKeyFrame() { }
    
    // Play sound on device
    void PlayOnDevice();
    
    virtual RobotInterface::EngineToRobot* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("DeviceAudioKeyFrame");
      return ClassName;
    }
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug = "") override;
    
  private:
    std::string _audioName;
    
  }; // class DeviceAudioKeyFrame
    
  // A RobotAudioKeyFrame references a single "sound" which is made of lots
  // of "samples" to be individually streamed to the robot.
  class RobotAudioKeyFrame : public IKeyFrame
  {
  public:
    
    struct AudioRef {
      Audio::GameEvent::GenericEvent audioEvent;
      float volume;
      float probability;   // random play weight
      bool audioAlts; // The audio event has altrnate or random audio track playback, avoid replaying event
      
      AudioRef( Audio::GameEvent::GenericEvent audioEvent = Audio::GameEvent::GenericEvent::Invalid,
                float volume      = 1.0f,
                float probability = 1.0f,
                bool audioAlts    = false )
      : audioEvent( audioEvent )
      , volume( volume )
      , probability( probability )
      , audioAlts( audioAlts ) {};
    };
    
    RobotAudioKeyFrame() { }
    RobotAudioKeyFrame( AudioRef&& audioRef, TimeStamp_t triggerTime_ms);
    
    // NOTE: Always returns nullptr for RobotAudioKeyframe!
    virtual RobotInterface::EngineToRobot* GetStreamMessage() override { return nullptr; };
    
    static const std::string& GetClassName() {
      static const std::string ClassName("RobotAudioKeyFrame");
      return ClassName;
    }
    
    const AudioRef& GetAudioRef() const;
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug = "") override;
    
  private:
    
    Result AddAudioRef(AudioRef&& audioRef);

    std::vector<AudioRef> _audioReferences;
    
  }; // class RobotAudioKeyFrame
    
    
  // A FaceImageKeyFrame stores a reference to a particular image / sprite to
  // be displayed on the robot's LED face display. When its GetStreamMessage()
  // is requested, it looks up the actual RLE-compressed image matching the
  // reference in the KeyFrame and fills the streamed message with it.
  
  // Depercated
  class FaceImageKeyFrame : public IKeyFrame
  {
  public:
    FaceImageKeyFrame() { }
    
    virtual RobotInterface::EngineToRobot* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("FaceImageKeyFrame");
      return ClassName;
    }
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug = "") override;
    
  private:
    u32 _imageID;
    
    AnimKeyFrame::FaceImage _streamMsg;
    
  }; // class FaceImageKeyFrame
  
  
  // A FaceAnimationKeyFrame is for streaming a set of images to display on the
  // robot's face. It is a cross between an AudioKeyFrame and an ImageKeyFrame.
  // Like an ImageKeyFrame, it populates single messages with RLE-compressed
  // data for display on the face display. Like an AudioKeyFrame, it will
  // return a non-NULL message each time GetStreamMessage() is called until there
  // are no more frames left in the animation.
  class FaceAnimationKeyFrame : public IKeyFrame
  {
  public:
    FaceAnimationKeyFrame(const std::string& faceAnimName = "") : _animName(faceAnimName) { }
    FaceAnimationKeyFrame(const AnimKeyFrame::FaceImage& faceImageMsg, const std::string& faceAnimName = "")
    : _animName(faceAnimName)
    , _faceImageMsg(faceImageMsg)
    { }
    
    virtual RobotInterface::EngineToRobot* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("FaceAnimationKeyFrame");
      return ClassName;
    }

    virtual bool IsDone() override;
    
    const std::string& GetName() const { return _animName; }
    const AnimKeyFrame::FaceImage& GetFaceImage() const { return _faceImageMsg; }
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug = "") override;
    
  private:
    std::string  _animName;
    
    s32 _curFrame;
    
    AnimKeyFrame::FaceImage _faceImageMsg;
    
  }; // class FaceAnimationKeyFrame
  
  
  class ProceduralFaceKeyFrame : public IKeyFrame
  {
  public:
    ProceduralFaceKeyFrame() { }
    ProceduralFaceKeyFrame(const ProceduralFace& face, TimeStamp_t triggerTime_ms = 0);
    
    // Always returns nullptr. Use GetInterpolatedFace() to get the face stored in this
    // keyframe.
    virtual RobotInterface::EngineToRobot* GetStreamMessage() override { return nullptr; }
    
    // Returns message for the face interpolated between the stored face in this
    // keyframe and the one in the next keyframe.
    //RobotInterface::EngineToRobot* GetInterpolatedStreamMessage(const ProceduralFaceKeyFrame& nextFrame);
    
    // Returns the interpolated face between the current keyframe and the next.
    // If the nextFrame is nullptr, then this frame's procedural face are returned.
    ProceduralFace GetInterpolatedFace(const ProceduralFaceKeyFrame& nextFrame, const TimeStamp_t currentTime_ms);
    
    static const std::string& GetClassName() {
      static const std::string ClassName("ProceduralFaceKeyFrame");
      return ClassName;
    }
    
    virtual bool IsDone() override;
    
    const ProceduralFace& GetFace() const { return _procFace; }

  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug = "") override;
    
  private:
    ProceduralFace  _procFace;
    bool            _isDone = false;
  
    //AnimKeyFrame::FaceImage _faceImageMsg;
    
    // This is what actually populates the message to stream, and is used
    // by GetStreamMessage() and GetInterpolatedStreamMessage().
    //RobotInterface::EngineToRobot* GetStreamMessageHelper(const ProceduralFace& procFace);
    
    void Reset();
    
  }; // class ProceduralFaceKeyFrame
  
  inline ProceduralFaceKeyFrame::ProceduralFaceKeyFrame(const ProceduralFace& face,
                                                        TimeStamp_t triggerTime)
  : _procFace(face)
  {
    SetTriggerTime(triggerTime);
    Reset();
  }
  
  // An EventKeyFrame simply returns an AnimEvent message from the robot
  // for higher precision event timing... like in Speed Tap.
  class EventKeyFrame : public IKeyFrame
  {
  public:
    EventKeyFrame() { }
    
    virtual RobotInterface::EngineToRobot* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("EventKeyFrame");
      return ClassName;
    }
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug = "") override;
    
  private:
    
    AnimKeyFrame::Event _streamMsg;
    
  }; // class EventKeyFrame
  
  
  // A BackpackLightsKeyFrame sets the colors of the robot's five backpack lights
  class BackpackLightsKeyFrame : public IKeyFrame
  {
  public:
    BackpackLightsKeyFrame() { }
    
    virtual RobotInterface::EngineToRobot* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("BackpackLightsKeyFrame");
      return ClassName;
    }
    
    virtual bool IsDone() override;
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug = "") override;
    
  private:
    
    s32 _durationTime_ms;
    AnimKeyFrame::BackpackLights _streamMsg;
    
  }; // class BackpackLightsKeyFrame
  
  
  // A BodyMotionKeyFrame controls the wheels to drive straight, turn in place, or
  // drive arcs. They specify the speed and duration of the motion.
  class BodyMotionKeyFrame : public IKeyFrame
  {
  public:
    BodyMotionKeyFrame();
    BodyMotionKeyFrame(s16 speed, s16 curvatureRadius_mm, s32 duration_ms);
    
    virtual RobotInterface::EngineToRobot* GetStreamMessage() override;
    
    static const std::string& GetClassName() {
      static const std::string ClassName("BodyMotionKeyFrame");
      return ClassName;
    }
    
    virtual bool IsDone() override;
    
    s32 GetDurationTime() const { return _durationTime_ms; }
    void EnableStopMessage(bool enable) { _enableStopMessage = enable; }
    
  protected:
    virtual Result SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug = "") override;
    
  private:
    
    s32 _durationTime_ms;
    bool _enableStopMessage = true;
    
    AnimKeyFrame::BodyMotion _streamMsg;
    AnimKeyFrame::BodyMotion _stopMsg;
    
  }; // class BodyMotionKeyFrame
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_KEYFRAME_H
