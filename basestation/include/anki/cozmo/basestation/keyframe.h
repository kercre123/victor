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
      static const u32 SAMPLE_LENGTH_MS = 33;
      
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
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_KEYFRAME_H
