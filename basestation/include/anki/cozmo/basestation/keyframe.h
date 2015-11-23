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
#include "anki/cozmo/basestation/soundManager.h"
#include "util/random/randomGenerator.h"
#include "json/json-forwards.h"

namespace Anki {
  namespace Cozmo {
  // Forward declaration
  namespace RobotInterface {
  class EngineToRobot;
  enum class EngineToRobotTag : uint8_t;
  }


  // Forward declaration
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
      
      bool IsValid() const { return _isValid; }
      
      // Returns true if current time has reached frame's "trigger" time, relative
      // to the given start time, or if this has been set as a "Live" keyframe
      bool IsTimeToPlay(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms) const;
      
      // Returns the time to trigger whatever change is implied by the KeyFrame
      TimeStamp_t GetTriggerTime() const { return _triggerTime_ms; }
      
      // Set all members from Json. Calls virtual SetMembersFromJson() method so
      // subclasses can specify how to populate their members.
      Result DefineFromJson(const Json::Value &json);
      
      // The trigger time for "live" keyframes is irrelevant; they are always
      // ready to play.
      bool IsLive() const { return _isLive; }
      void SetIsLive(bool tf) { _isLive = tf; }
      
      // Fill some kind of message for streaming and return it. Return nullptr
      // if not available.
      virtual RobotInterface::EngineToRobot* GetStreamMessage() = 0;
      
      // Whether or not this KeyFrame is "done" after calling GetStreamMessage().
      // Override for special keyframes that need to keep parceling out data into
      // multiple returned messages.
      virtual bool IsDone() { return true; }
      
    protected:
      
      // Populate members from Json
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) = 0;
      
      //void SetIsValid(bool isValid) { _isValid = isValid; }
      
      Util::RandomGenerator& GetRNG();
      
    private:
      
      // A random number generator for all keyframes to share (for adding variability)
      static Util::RandomGenerator sRNG;

      TimeStamp_t   _triggerTime_ms;
      bool          _isValid;
      bool          _isLive;
      
    }; // class IKeyFrame
    
    inline Util::RandomGenerator& IKeyFrame::GetRNG() {
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
      
    protected:
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
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
      
    protected:
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
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
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
    private:
      std::string _audioName;
      
    }; // class DeviceAudioKeyFrame
    
    
    // A RobotAudioKeyFrame references a single "sound" which is made of lots
    // of "samples" to be individually streamed to the robot.
    class RobotAudioKeyFrame : public IKeyFrame
    {
    public:
      RobotAudioKeyFrame() : _selectedAudioIndex(0), _sampleIndex(0) { }
      
      virtual RobotInterface::EngineToRobot* GetStreamMessage() override;
      
      static const std::string& GetClassName() {
        static const std::string ClassName("RobotAudioKeyFrame");
        return ClassName;
      }
      
      const std::string& GetSoundName() const;
      
    protected:
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
    private:
      
      Result AddAudioRef(const std::string& name, const f32 volume = 1.f);
      
      struct AudioRef {
        std::string name;
        s32 numSamples;
        f32 volume;
      };
      std::vector<AudioRef> _audioReferences;
      s32 _selectedAudioIndex;
      
      s32 _sampleIndex;
      
      AnimKeyFrame::AudioSample  _audioSampleMsg;
      
    }; // class RobotAudioKeyFrame
    
    
    // A FaceImageKeyFrame stores a reference to a particular image / sprite to
    // be displayed on the robot's LED face display. When its GetStreamMessage()
    // is requested, it looks up the actual RLE-compressed image matching the
    // reference in the KeyFrame and fills the streamed message with it.
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
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
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
      
      virtual RobotInterface::EngineToRobot* GetStreamMessage() override;
      
      static const std::string& GetClassName() {
        static const std::string ClassName("FaceAnimationKeyFrame");
        return ClassName;
      }

      virtual bool IsDone() override;
      
    protected:
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
    private:
      std::string  _animName;
      
      s32 _curFrame;
      
      AnimKeyFrame::FaceImage _faceImageMsg;
      
    }; // class FaceAnimationKeyFrame
    
    
    class ProceduralFaceKeyFrame : public IKeyFrame
    {
    public:
      ProceduralFaceKeyFrame() { }
      ProceduralFaceKeyFrame(const ProceduralFace& face) : _procFace(face) { }
      
      // Returns message for the face stored in this message
      virtual RobotInterface::EngineToRobot* GetStreamMessage() override;
      
      // Returns message for the face interpolated between the stored face in this
      // keyframe and the one in the next keyframe.
      RobotInterface::EngineToRobot* GetInterpolatedStreamMessage(const ProceduralFaceKeyFrame& nextFrame);
      
      static const std::string& GetClassName() {
        static const std::string ClassName("ProceduralFaceKeyFrame");
        return ClassName;
      }
      
      virtual bool IsDone() override;
      
      const ProceduralFace& GetFace() const { return _procFace; }

    protected:
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
    private:
      ProceduralFace  _procFace;
      TimeStamp_t     _currentTime_ms;
      bool            _isDone;
    
      AnimKeyFrame::FaceImage _faceImageMsg;
      
      // This is what actually populates the message to stream, and is used
      // by GetStreamMessage() and GetInterpolatedStreamMessage().
      RobotInterface::EngineToRobot* GetStreamMessageHelper(const ProceduralFace& procFace);
      
      void Reset();
      
    }; // class ProceduralFaceKeyFrame
    
    // A FacePositionKeyFrame sets the center of the currently displayed face
    // image/sprite, in LED screen coordinates.
    class FacePositionKeyFrame : public IKeyFrame
    {
    public:
      FacePositionKeyFrame() { }
      
      virtual RobotInterface::EngineToRobot* GetStreamMessage() override;
      
      static const std::string& GetClassName() {
        static const std::string ClassName("FacePositionKeyFrame");
        return ClassName;
      }
      
    protected:
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
    private:
      
      AnimKeyFrame::FacePosition _streamMsg;
      
    }; // class FacePositionKeyFrame
    
    // BlinkKeyFrame either disables automatic blinking for a period fo time or
    // commands a single blink to happen "now".
    class BlinkKeyFrame : public IKeyFrame
    {
    public:
      BlinkKeyFrame();
      
      virtual RobotInterface::EngineToRobot* GetStreamMessage() override;
      
      static const std::string& GetClassName() {
        static const std::string ClassName("BlinkKeyFrame");
        return ClassName;
      }
      
      virtual bool IsDone() override;
      
    protected:
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
    private:
      
      s32 _duration_ms;
      s32 _curTime_ms;
      
      AnimKeyFrame::Blink _streamMsg;

    }; // class BlinkKeyFrame
    
    
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
      
    protected:
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
    private:
      
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
      
    protected:
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
    private:
      
      s32 _durationTime_ms;
      s32 _currentTime_ms;
      
      AnimKeyFrame::BodyMotion _streamMsg;
      AnimKeyFrame::BodyMotion _stopMsg;
      
    }; // class BodyMotionKeyFrame
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_KEYFRAME_H
