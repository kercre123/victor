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

#include "anki/cozmo/basestation/comms/robot/robotMessages.h"

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/shared/ledTypes.h"

#include "anki/cozmo/basestation/soundManager.h"

namespace Anki {
  namespace Cozmo {
    
    // Forward declaration
    class Robot;
    class IRobotMessageHandler;
    
    // IKeyFrame defines an abstract interface for all KeyFrames below.
    class IKeyFrame
    {
    public:
      IKeyFrame();
      //IKeyFrame(const Json::Value& root);
      ~IKeyFrame();
      
      bool IsValid() const { return _isValid; }
      
      bool IsTimeToPlay(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms) const;
      
      // Returns the time to trigger whatever change is implied by the KeyFrame
      TimeStamp_t GetTriggerTime() const { return _triggerTime_ms; }
      
      // Set all members from Json. Calls virtual SetMembersFromJson() method so
      // subclasses can specify how to populate their members.
      Result DefineFromJson(const Json::Value &json);
      
      // Fill some kind of message for streaming and return it. Return nullptr
      // if not available.
      virtual RobotMessage* GetStreamMessage() = 0;
      
    protected:
      
      // Populate members from Json
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) = 0;
      
      //void SetIsValid(bool isValid) { _isValid = isValid; }
      
    private:
      
      TimeStamp_t   _triggerTime_ms;
      bool          _isValid;
      
    }; // class IKeyFrame
    
    
    // A HeadAngleKeyFrame specifies the time to _start_ moving the head towards
    // a given angle (with optional variation), and how long to take to get there.
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
      
    }; // class HeadAngleKeyFrame
    
    
    // A LiftHeightKeyFrame specifies the time to _start_ moving the lift towards
    // a given height (with optional variation), and how long to take to get there.
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
      
      virtual RobotMessage* GetStreamMessage() override;
      
      static const std::string& GetClassName() {
        static const std::string ClassName("DeviceAudioKeyFrame");
        return ClassName;
      }
      
    protected:
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
    private:
      u32 _audioID;
      
    }; // class DeviceAudioKeyFrame
    
    
    // A RobotAudioKeyFrame references a single "sound" which is made of lots
    // of "samples" to be individually streamed to the robot.
    class RobotAudioKeyFrame : public IKeyFrame
    {
    public:
      static const u32 SAMPLE_LENGTH_MS = 33;
      
      RobotAudioKeyFrame() { }
      
      virtual RobotMessage* GetStreamMessage() override;
      
      static const std::string& GetClassName() {
        static const std::string ClassName("RobotAudioKeyFrame");
        return ClassName;
      }
      
      SoundID_t GetAudioID() const { return _audioID; }
      
    protected:
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
    private:
      std::string _audioName;
      SoundID_t   _audioID;
      
      s32 _numSamples;
      s32 _sampleIndex;
      
      MessageAnimKeyFrame_AudioSample  _audioSampleMsg;
      
    }; // class RobotAudioKeyFrame
    
    
    // A FaceImageKeyFrame stores a reference to a particular image / sprite to
    // be displayed on the robot's LED face display. When its GetStreamMessage()
    // is requested, it looks up the actual RLE-compressed image matching the
    // reference in the KeyFrame and fills the streamed message with it.
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
      
    }; // class FaceImageKeyFrame
    
    
    // A FacePositionKeyFrame sets the center of the currently displayed face
    // image/sprite, in LED screen coordinates.
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
      
      MessageAnimKeyFrame_FacePosition _streamMsg;
      
    }; // class FacePositionKeyFrame
    
    
    // A BackpackLightsKeyFrame sets the colors of the robot's five backpack lights
    class BackpackLightsKeyFrame : public IKeyFrame
    {
    public:
      BackpackLightsKeyFrame() { }
      
      virtual RobotMessage* GetStreamMessage() override;
      
      static const std::string& GetClassName() {
        static const std::string ClassName("BackpackLightsKeyFrame");
        return ClassName;
      }
      
    protected:
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
    private:
      
      MessageAnimKeyFrame_BackpackLights _streamMsg;
      
    }; // class BackpackLightsKeyFrame
    
    
    // A BodyPositionKeyFrame
    // TODO: Decide what this actually stores (direct wheel speeds or something more abstracted?)
    class BodyPositionKeyFrame : public IKeyFrame
    {
    public:
      BodyPositionKeyFrame() { }
      
      virtual RobotMessage* GetStreamMessage() override;
      
      static const std::string& GetClassName() {
        static const std::string ClassName("BodyPositionKeyFrame");
        return ClassName;
      }
      
    protected:
      virtual Result SetMembersFromJson(const Json::Value &jsonRoot) override;
      
    private:
      MessageAnimKeyFrame_BodyMotion _streamMsg;
      
    }; // class BodyPositionKeyFrame
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_KEYFRAME_H
