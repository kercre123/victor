/**
 * File: keyframe.cpp
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


#include "anki/common/basestation/colorRGBA.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/animations/animEventHelpers.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"
#include "anki/cozmo/basestation/keyframe.h"
#include "anki/cozmo/basestation/ledEncoding.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/helpers/quoteMacro.h"
#include "util/logging/logging.h"
#include "cozmo_anim_generated.h"

#include <opencv2/core.hpp>
#include <cassert>


bool has_any_digits(const std::string& s)
{
  return std::any_of(s.begin(), s.end(), ::isdigit);
}


namespace Anki {
  namespace Cozmo {
    
#pragma mark -
#pragma mark IKeyFrame
    
    // Static initialization
    Util::RandomGenerator IKeyFrame::sRNG;
    
    IKeyFrame::IKeyFrame()
    {
      
    }
    
    IKeyFrame::~IKeyFrame()
    {
      
    }
    
    bool IKeyFrame::IsTimeToPlay(TimeStamp_t animationTime_ms) const
    {
      return GetTriggerTime() <= animationTime_ms;
    }
    
    bool IKeyFrame::IsTimeToPlay(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms) const
    {
      return GetTriggerTime() + startTime_ms <= currTime_ms;
    }
    
    Result IKeyFrame::DefineFromJson(const Json::Value &json, const std::string& animNameDebug)
    {
      Result lastResult = RESULT_OK;
      
      // Read the frame time from the json file as well
      if(!json.isMember("triggerTime_ms")) {
        PRINT_NAMED_ERROR("IKeyFrame.ReadFromJson",
                          "%s: Expecting 'triggerTime_ms' field in KeyFrame Json",
                          animNameDebug.c_str());
        lastResult = RESULT_FAIL;
      } else {
        _triggerTime_ms = json["triggerTime_ms"].asUInt();
      }
      
      if(lastResult == RESULT_OK) {
        lastResult = SetMembersFromJson(json, animNameDebug);
      }
      
      return lastResult;
    }
    
    bool IKeyFrame::IsDoneHelper(TimeStamp_t durationTime_ms)
    {
      // Done once enough time has ticked by or if we're not sending a done message
      if(_currentTime_ms >= durationTime_ms) {
        _currentTime_ms = 0; // Reset for next time
        return true;
      } else {
        _currentTime_ms += SAMPLE_LENGTH_MS;
        return false;
      }
    }
    
#pragma mark -
#pragma mark Helpers
    
    // Helper macro used in SetMembersFromJson() overrides below to look for
    // member variable in Json node and fail if it doesn't exist
#define GET_MEMBER_FROM_JSON_AND_STORE_IN(__JSON__, __NAME__, __MEMBER_NAME__) do { \
if(!JsonTools::GetValueOptional(__JSON__, QUOTE(__NAME__), this->_##__MEMBER_NAME__)) { \
PRINT_NAMED_ERROR("IKeyFrame.GetMemberFromJsonMacro", \
"Failed to get '%s' from Json file.", QUOTE(__NAME__)); \
return RESULT_FAIL; \
} } while(0)
    
#define GET_MEMBER_FROM_JSON(__JSON__, __NAME__) GET_MEMBER_FROM_JSON_AND_STORE_IN(__JSON__, __NAME__, __NAME__)

namespace {
  
// Cast the value in fromVal to toVal, clamping to the numerical limits of the target type and printing a debug message if so
template<typename FromType, typename ToType>
void SafeNumericCast(const FromType& fromVal, ToType& toVal, const char* debugName = "InvalidCast")
{
  if (!Util::IsValidNumericCast<ToType>(fromVal)) {
    toVal = Util::numeric_cast_clamped<ToType>(fromVal);
    
    std::stringstream debugStr;
    debugStr << "cast of " << fromVal << " would be invalid, clamping to " << toVal;
    PRINT_NAMED_WARNING("IKeyFrame.SafeNumericCast.InvalidCast",
                        "%s: %s",
                        debugName,
                        debugStr.str().c_str());
  } else {
    toVal = Util::numeric_cast<ToType>(fromVal);
  }
}
  
} // end anonymous namespace
  
#pragma mark -
#pragma mark HeadAngleKeyFrame

    //
    // HeadAngleKeyFrame
    //
    
     HeadAngleKeyFrame::HeadAngleKeyFrame(s8 angle_deg, u8 angle_variability_deg, TimeStamp_t duration_ms)
     : _durationTime_ms(duration_ms)
     , _angle_deg(angle_deg)
     , _angleVariability_deg(angle_variability_deg)
     {
       
     }
    
    RobotInterface::EngineToRobot* HeadAngleKeyFrame::GetStreamMessage()
    {
      _streamHeadMsg.time_ms = (uint16_t)_durationTime_ms;
      
      // Add variability:
      if(_angleVariability_deg > 0) {
        _streamHeadMsg.angle_deg = static_cast<s8>(GetRNG().RandIntInRange(_angle_deg - _angleVariability_deg,
                                                                           _angle_deg + _angleVariability_deg));
      } else {
        _streamHeadMsg.angle_deg = _angle_deg;
      }
      
      return new RobotInterface::EngineToRobot(AnimKeyFrame::HeadAngle(_streamHeadMsg));
    }

    Result HeadAngleKeyFrame::DefineFromFlatBuf(const CozmoAnim::HeadAngle* headAngleKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(headAngleKeyframe != nullptr, "HeadAngleKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(headAngleKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      Result lastResult = SetMembersFromFlatBuf(headAngleKeyframe, animNameDebug);
      return lastResult;
    }

    Result HeadAngleKeyFrame::SetMembersFromFlatBuf(const CozmoAnim::HeadAngle* headAngleKeyframe, const std::string& animNameDebug)
    {
      SafeNumericCast(headAngleKeyframe->durationTime_ms(),      _durationTime_ms,      animNameDebug.c_str());
      SafeNumericCast(headAngleKeyframe->angle_deg(),            _angle_deg,            animNameDebug.c_str());
      SafeNumericCast(headAngleKeyframe->angleVariability_deg(), _angleVariability_deg, animNameDebug.c_str());

      return RESULT_OK;
    }
    
    Result HeadAngleKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, durationTime_ms);
      GET_MEMBER_FROM_JSON(jsonRoot, angle_deg);
      GET_MEMBER_FROM_JSON(jsonRoot, angleVariability_deg);
      
      return RESULT_OK;
    }
    
#pragma mark -
#pragma mark LiftHeightKeyFrame
    //
    // LiftHeightKeyFrame
    //

    LiftHeightKeyFrame::LiftHeightKeyFrame(u8 height_mm, u8 heightVariability_mm, TimeStamp_t duration_ms)
    : _durationTime_ms(duration_ms)
    , _height_mm(height_mm)
    , _heightVariability_mm(heightVariability_mm)
    {
      
    }
    
    RobotInterface::EngineToRobot* LiftHeightKeyFrame::GetStreamMessage()
    {
      _streamLiftMsg.time_ms = (uint16_t)_durationTime_ms;
      
      // Add variability:
      if(_heightVariability_mm > 0) {
        _streamLiftMsg.height_mm = (uint8_t)static_cast<s8>(GetRNG().RandIntInRange(_height_mm - _heightVariability_mm,
                                                                                    _height_mm + _heightVariability_mm));
      } else {
        _streamLiftMsg.height_mm = _height_mm;
      }
      
      return new RobotInterface::EngineToRobot(AnimKeyFrame::LiftHeight(_streamLiftMsg));
    }

    Result LiftHeightKeyFrame::DefineFromFlatBuf(const CozmoAnim::LiftHeight* liftHeightKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(liftHeightKeyframe != nullptr, "LiftHeightKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(liftHeightKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      Result lastResult = SetMembersFromFlatBuf(liftHeightKeyframe, animNameDebug);
      return lastResult;
    }

    Result LiftHeightKeyFrame::SetMembersFromFlatBuf(const CozmoAnim::LiftHeight* liftHeightKeyframe, const std::string& animNameDebug)
    {
      SafeNumericCast(liftHeightKeyframe->durationTime_ms(),      _durationTime_ms,      animNameDebug.c_str());
      SafeNumericCast(liftHeightKeyframe->height_mm(),            _height_mm,            animNameDebug.c_str());
      SafeNumericCast(liftHeightKeyframe->heightVariability_mm(), _heightVariability_mm, animNameDebug.c_str());
               
      return RESULT_OK;
    }
    
    Result LiftHeightKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, durationTime_ms);
      GET_MEMBER_FROM_JSON(jsonRoot, height_mm);
      GET_MEMBER_FROM_JSON(jsonRoot, heightVariability_mm);
      
      return RESULT_OK;
    }
    
#pragma mark -
#pragma mark FaceImageKeyFrame
    //
    // FaceImageKeyFrame
    //
    
    Result FaceImageKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, imageID);
      
      return RESULT_OK;
    }
    
    RobotInterface::EngineToRobot* FaceImageKeyFrame::GetStreamMessage()
    {
      // TODO: Fill the message for streaming using the imageID
      // memcpy(_streamMsg.image, LoadFaceImage(_imageID);
      
      // For now, just put some stuff for display in there, using a few hard-coded
      // patterns depending on ID
      _streamMsg.image.clear();
      if(_imageID == 0) {
        // All black
        _streamMsg.image.push_back(0);
      } else if(_imageID == 1) {
        // All blue
        _streamMsg.image.push_back(64); // Switch to blue
        _streamMsg.image.push_back(63); // Fill lines
        _streamMsg.image.push_back(0);  // Done
      } else {
        // Draw "programmer art" face until we get real assets

        _streamMsg.image = { 24, 64+24,   // Start 24 lines down and 24 pixels right
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          0 };
      }
      
      return new RobotInterface::EngineToRobot(AnimKeyFrame::FaceImage(_streamMsg));
    }
    
#pragma mark -
#pragma mark FaceAnimationKeyFrame
    
    Result FaceAnimationKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, animName);

      return Process(animNameDebug);
    }

    Result FaceAnimationKeyFrame::Process(const std::string& animNameDebug)
    {
      // TODO: Take this out once root path is part of AnimationTool!
      size_t lastSlash = _animName.find_last_of("/");
      if(lastSlash != std::string::npos) {
        PRINT_NAMED_WARNING("FaceAnimationKeyFrame.Process",
                            "%s: Removing path from animation name: %s",
                            animNameDebug.c_str(),
                            _animName.c_str());
        _animName = _animName.substr(lastSlash+1, std::string::npos);
      }

      _curFrame = 0;

      return RESULT_OK;
    }

    Result FaceAnimationKeyFrame::DefineFromFlatBuf(const CozmoAnim::FaceAnimation* faceAnimKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(faceAnimKeyframe != nullptr, "FaceAnimationKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(faceAnimKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      _triggerTime_ms = (uint) faceAnimKeyframe->triggerTime_ms();
      Result lastResult = SetMembersFromFlatBuf(faceAnimKeyframe, animNameDebug);
      return lastResult;
    }

    Result FaceAnimationKeyFrame::SetMembersFromFlatBuf(const CozmoAnim::FaceAnimation* faceAnimKeyframe, const std::string& animNameDebug)
    {
      this->_animName = faceAnimKeyframe->animName()->str();

      return Process(animNameDebug);
    }
    
    bool FaceAnimationKeyFrame::IsDone()
    {
      // Note the dynamic check for num frames, since (in the case of streaming
      // procedural animations) the number of keyframes could be increasing
      // while we're playing and thus isn't known up front.
      return !_isSingleFrame && (_curFrame >= FaceAnimationManager::getInstance()->GetNumFrames(_animName));
    }
    
    RobotInterface::EngineToRobot* FaceAnimationKeyFrame::GetStreamMessage()
    {
      // Populate the message with the next chunk of audio data and send it out
      
      if(!IsDone()) 
      {
        const std::vector<u8>* rleFrame;
        if (_isSingleFrame) {
          rleFrame = &_faceImageMsg.image;
        }
        else {
          rleFrame = FaceAnimationManager::getInstance()->GetFrame(_animName, _curFrame);
        }
        
        if(rleFrame == nullptr) {
          PRINT_NAMED_ERROR("FaceAnimationKeyFrame.GetStreamMessage",
                            "Failed to get frame %d from animation %s",
                            _curFrame, _animName.c_str());
          return nullptr;
        }
        
        if(rleFrame->empty()) {
          // No face in this frame, return nullptr so we don't stream anything to
          // the robot, but don't complain because this is not an error.
          // Still increment the frame counter.
          ++_curFrame;
          return nullptr;
        }
        
        // copy std vector
        _faceImageMsg.image = *rleFrame;
        ++_curFrame;
        
        return new RobotInterface::EngineToRobot(AnimKeyFrame::FaceImage(_faceImageMsg));
      } else {
        _curFrame = 0;
        return nullptr;
      }
    }
    
#pragma mark -
#pragma mark ProceduralFaceKeyFrame
    
    Result ProceduralFaceKeyFrame::DefineFromFlatBuf(const CozmoAnim::ProceduralFace* procFaceKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(procFaceKeyframe != nullptr, "ProceduralFaceKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(procFaceKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      Result lastResult = SetMembersFromFlatBuf(procFaceKeyframe, animNameDebug);
      return lastResult;
    }

    Result ProceduralFaceKeyFrame::SetMembersFromFlatBuf(const CozmoAnim::ProceduralFace* procFaceKeyframe, const std::string& animNameDebug)
    {
      _procFace.SetFromFlatBuf(procFaceKeyframe);
      Reset();
      return RESULT_OK;
    }

    Result ProceduralFaceKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      _procFace.SetFromJson(jsonRoot);
      Reset();
      return RESULT_OK;
    }
    
    void ProceduralFaceKeyFrame::Reset()
    {
      _isDone = false;
    }
    
    bool ProceduralFaceKeyFrame::IsDone()
    {
      bool retVal = _isDone;
      if(_isDone) {
        // This sets _isDone back to false!
        Reset();
      }
      return retVal;
    }
    
    /*
    RobotInterface::EngineToRobot* ProceduralFaceKeyFrame::GetStreamMessageHelper(const ProceduralFace& procFace)
    {
      Result rleResult = FaceAnimationManager::CompressRLE(procFace.GetFace(), _faceImageMsg.image);
      
      if(RESULT_OK != rleResult) {
        PRINT_NAMED_ERROR("ProceduralFaceKeyFrame.GetStreamMessageHelper",
                          "Failed to get RLE frame from procedural face.");
        return nullptr;
      }

      return new RobotInterface::EngineToRobot(AnimKeyFrame::FaceImage(_faceImageMsg));
    }
    
    RobotInterface::EngineToRobot* ProceduralFaceKeyFrame::GetStreamMessage()
    {
      _isDone = true;
      return GetStreamMessageHelper(_procFace);
    }
    
    RobotInterface::EngineToRobot* ProceduralFaceKeyFrame::GetInterpolatedStreamMessage(const ProceduralFaceKeyFrame& nextFrame)
    {
      // The interpolation fraction is how far along in time we are from this frame's
      // trigger time (which currentTime was initialized to) and the next frame's
      // trigger time.
      const f32 fraction = std::min(1.f, static_cast<f32>(_currentTime_ms - GetTriggerTime()) / static_cast<f32>(nextFrame.GetTriggerTime() - GetTriggerTime()));
      
      ProceduralFace interpFace;
      interpFace.GetParams().Interpolate(_procFace.GetParams(), nextFrame._procFace.GetParams(), fraction);
      
      _currentTime_ms += IKeyFrame::SAMPLE_LENGTH_MS;
      if(_currentTime_ms >= nextFrame.GetTriggerTime()) {
        _isDone = true;
      }
      
      return GetStreamMessageHelper(interpFace);
    }
    */
    
    ProceduralFace ProceduralFaceKeyFrame::GetInterpolatedFace(const ProceduralFaceKeyFrame& nextFrame, const TimeStamp_t currentTime_ms)
    {
      // The interpolation fraction is how far along in time we are from this frame's
      // trigger time (which currentTime was initialized to) and the next frame's
      // trigger time.
      const f32 fraction = std::min(1.f, static_cast<f32>(currentTime_ms - GetTriggerTime()) / static_cast<f32>(nextFrame.GetTriggerTime() - GetTriggerTime()));
      
      ProceduralFace interpFace;
      interpFace.Interpolate(_procFace, nextFrame._procFace, fraction);
      
      return interpFace;
    }
    
#pragma mark -
#pragma mark RobotAudioKeyFrame
    
    //
    // RobotAudioKeyFrame
    //
    RobotAudioKeyFrame::RobotAudioKeyFrame(AudioRef&& audioRef, TimeStamp_t triggerTime_ms)
    {
      SetTriggerTime(triggerTime_ms);
      AddAudioRef( std::move( audioRef ) );
    }
    
    Result RobotAudioKeyFrame::AddAudioRef(AudioRef&& audioRef)
    {
      // TODO: Need a way to verify the event is valid while loading animation metadata - JMR
      _audioReferences.emplace_back( std::move( audioRef ) );
      return RESULT_OK;
    }
    
    const RobotAudioKeyFrame::AudioRef& RobotAudioKeyFrame::GetAudioRef() const
    {
      if(_audioReferences.empty()) {
        PRINT_NAMED_ERROR("RobotAudioKeyFrame.GetStreamMessage.EmptyAudioReferences",
                          "Check to make sure animation loaded successfully - sound file(s) probably not found.");
        static const AudioRef InvalidRef( AudioMetaData::GameEvent::GenericEvent::Invalid );
        return InvalidRef;
      }
      
      // Select one of the audio names to play
      size_t selectedAudioIndex = 0;
      if(_audioReferences.size()>1) {
        // If there are more than one audio references
        selectedAudioIndex = GetRNG().RandIntInRange(0, static_cast<s32>(_audioReferences.size()-1));
      }
      
      return _audioReferences[selectedAudioIndex];
    }

    Result RobotAudioKeyFrame::DefineFromFlatBuf(const CozmoAnim::RobotAudio* audioKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(audioKeyframe != nullptr, "RobotAudioKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(audioKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      Result lastResult = SetMembersFromFlatBuf(audioKeyframe, animNameDebug);
      return lastResult;
    }
    
    Result RobotAudioKeyFrame::SetMembersFromFlatBuf(const CozmoAnim::RobotAudio* audioKeyframe, const std::string& animNameDebug)
    {
      f32 volume = audioKeyframe->volume();
      f32 probability = audioKeyframe->probability();
      bool hasAlts = audioKeyframe->hasAlts();;

      if(audioKeyframe->audioName()->size() < 1) {
        PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromFlatBuf.MissingAudioName",
                          "%s: No 'audioName' field in FlatBuffers frame.",
                          animNameDebug.c_str());
        return RESULT_FAIL;
      }

      auto audioEventData = audioKeyframe->audioEventId();
      for (int aeIdx=0; aeIdx < audioEventData->size(); aeIdx++) {
        auto audioEventVal = audioEventData->Get(aeIdx);

        // The casting to 64-bit was borrowed from RobotAudioKeyFrame::SetMembersFromJson(), where the corresponding
        // comment is "We intentionally cast json data to 64 bit so we can guaranty that the value is 32 bit"
        const auto eventId = static_cast<AudioMetaData::GameEvent::GenericEvent>( (uint64_t) audioEventVal );

        Result addResult = AddAudioRef( AudioRef( eventId, volume, probability, hasAlts ) );
        if(addResult != RESULT_OK) {
          return addResult;
        }
      }

      return RESULT_OK;
    }
    
    Result RobotAudioKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      // Get volume
      f32 volume = 1.0f;
      JsonTools::GetValueOptional(jsonRoot, "volume", volume);
      // Get Random probability
      f32 probability = 1.0f;
      JsonTools::GetValueOptional(jsonRoot, "probability", probability);
      // Get Has Alternate audio flag
      bool hasAlts = false;
      JsonTools::GetValueOptional(jsonRoot, "hasAlts", hasAlts);
      
      if(!jsonRoot.isMember("audioName")) {
        PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromJson.MissingAudioName",
                          "%s: No 'audioName' field in Json frame.",
                          animNameDebug.c_str());
        return RESULT_FAIL;
      }
      
      const Json::Value& jsonAudioNames = jsonRoot["audioEventId"];
      if(jsonAudioNames.isArray()) {
        for(s32 i=0; i<jsonAudioNames.size(); ++i) {
          // We intentionally cast json data to 64 bit so we can guaranty that the value is 32 bit
          const auto eventId = static_cast<AudioMetaData::GameEvent::GenericEvent>( jsonAudioNames[i].asUInt64() );
          Result addResult = AddAudioRef( AudioRef( eventId, volume, probability, hasAlts ) );
          if(addResult != RESULT_OK) {
            return addResult;
          }
        }
      } else {
        // We intentionally cast json data to 64 bit so we can guaranty that the value is 32 bit
        const auto eventId = static_cast<AudioMetaData::GameEvent::GenericEvent>( jsonAudioNames.asUInt64() );
        Result addResult = AddAudioRef( AudioRef( eventId, volume, probability, hasAlts ) );
        if(addResult != RESULT_OK) {
          return addResult;
        }
      }
      
      return RESULT_OK;
    }
    
#pragma mark -
#pragma mark DeviceAudioKeyFrame
    //
    // DeviceAudioKeyFrame
    //
    
    Result DeviceAudioKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, audioName);
      
      return RESULT_OK;
    }
    
    RobotInterface::EngineToRobot* DeviceAudioKeyFrame::GetStreamMessage()
    {
      // Device audio is not streamed to the robot, by definition, so it always
      // returns nullptr
      return nullptr;
    }
    
    void DeviceAudioKeyFrame::PlayOnDevice()
    {
      // TODO: Replace with real call to wwise or something
    }
    
#pragma mark -
#pragma mark EventKeyFrame
    //
    // EventKeyFrame
    //
    
    RobotInterface::EngineToRobot* EventKeyFrame::GetStreamMessage()
    {
      return new RobotInterface::EngineToRobot(AnimKeyFrame::Event(_streamMsg));
    }

    Result EventKeyFrame::DefineFromFlatBuf(const CozmoAnim::Event* eventKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(eventKeyframe != nullptr, "EventKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(eventKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      Result lastResult = SetMembersFromFlatBuf(eventKeyframe, animNameDebug);
      return lastResult;
    }
    
    Result EventKeyFrame::SetMembersFromFlatBuf(const CozmoAnim::Event* eventKeyframe, const std::string& animNameDebug)
    {
      // Convert event_id string to AnimEvent enum
      const std::string& eventStr = eventKeyframe->event_id()->str();
      AnimEvent e = AnimEventFromString(eventStr.c_str());
      if (e == AnimEvent::Count) {
        PRINT_NAMED_WARNING("EventKeyFrame.UnrecognizedEventName", "%s", eventStr.c_str());
        return RESULT_FAIL;
      }
      _streamMsg.event_id = e;

      return RESULT_OK;
    }

    Result EventKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      // Convert event_id string to AnimEvent enum
      if (!jsonRoot.isMember("event_id")) {
        PRINT_NAMED_WARNING("EventKeyFrame.NoEventIDFound", "");
        return RESULT_FAIL;
      } else {
        if(jsonRoot["event_id"].isString()) {
          const std::string& eventStr = jsonRoot["event_id"].asString();
          AnimEvent e = AnimEventFromString(eventStr.c_str());
          if (e == AnimEvent::Count) {
            PRINT_NAMED_WARNING("EventKeyFrame.UnrecognizedEventName", "%s", eventStr.c_str());
            return RESULT_FAIL;
          }
          _streamMsg.event_id = e;
        } else {
          PRINT_NAMED_WARNING("EventKeyFrame.EventIDNotString", "");
          return RESULT_FAIL;
        }
      }
      
      return RESULT_OK;
    }
    
#pragma mark -
#pragma mark BackpackLightsKeyFrame
    
    Result BackpackLightsKeyFrame::DefineFromFlatBuf(CozmoAnim::BackpackLights* backpackKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(backpackKeyframe != nullptr, "BackpackLightsKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(backpackKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      Result lastResult = SetMembersFromFlatBuf(backpackKeyframe, animNameDebug);
      return lastResult;
    }

    Result BackpackLightsKeyFrame::SetMembersFromFlatBuf(CozmoAnim::BackpackLights* backpackKeyframe, const std::string& animNameDebug)
    {
      // TODO: IMPLEMENT THIS METHOD

      PRINT_NAMED_ERROR("BackpackLightsKeyFrame::SetMembersFromFlatBuf",
                        "The BackpackLightsKeyFrame::SetMembersFromFlatBuf() method still needs to be implemented");

      return RESULT_OK;
    }

    Result BackpackLightsKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      ColorRGBA color;
     
      // Special helper macro for getting the LED colors out of the Json and
      // store them directly in the streamMsg
#define GET_COLOR_FROM_JSON(__NAME__, __LED_NAME__) do {             \
if(!JsonTools::GetColorOptional(jsonRoot, QUOTE(__NAME__), color)) { \
  PRINT_NAMED_ERROR("BackpackLightsKeyFrame.SetMembersFromJson",        \
                    "%s: Failed to get '%s' LED color from Json file", \
                    animNameDebug.c_str(), QUOTE(__NAME__));            \
  return RESULT_FAIL;                                                   \
}                                                                       \
_streamMsg.colors[__LED_NAME__] = ENCODED_COLOR(color); } while(0)

      GET_COLOR_FROM_JSON(Back, (int)LEDId::LED_BACKPACK_BACK);
      GET_COLOR_FROM_JSON(Front, (int)LEDId::LED_BACKPACK_FRONT);
      GET_COLOR_FROM_JSON(Middle, (int)LEDId::LED_BACKPACK_MIDDLE);
      GET_COLOR_FROM_JSON(Left, (int)LEDId::LED_BACKPACK_LEFT);
      GET_COLOR_FROM_JSON(Right, (int)LEDId::LED_BACKPACK_RIGHT);
      
      GET_MEMBER_FROM_JSON(jsonRoot, durationTime_ms);
      
      return RESULT_OK;
    }
    
    RobotInterface::EngineToRobot* BackpackLightsKeyFrame::GetStreamMessage()
    {
      return new RobotInterface::EngineToRobot(AnimKeyFrame::BackpackLights(_streamMsg));
    }
  
  
    bool BackpackLightsKeyFrame::IsDone()
    {
      return IsDoneHelper(_durationTime_ms);
    }
    
#pragma mark -
#pragma mark BodyMotionKeyFrame
    
    BodyMotionKeyFrame::BodyMotionKeyFrame()
    {
      _stopMsg.speed = 0;
    }
    
    BodyMotionKeyFrame::BodyMotionKeyFrame(s16 speed, s16 curvatureRadius_mm, s32 duration_ms)
    : BodyMotionKeyFrame()
    {
      _durationTime_ms = duration_ms;
      _streamMsg.speed = speed;
      _streamMsg.curvatureRadius_mm = curvatureRadius_mm;
    }
    
    void BodyMotionKeyFrame::CheckRotationSpeed(const std::string& animNameDebug)
    {
      // Check that speed is valid
      if (std::abs(_streamMsg.speed) > MAX_BODY_ROTATION_SPEED_DEG_PER_SEC) {
        PRINT_NAMED_INFO("BodyMotionKeyFrame.CheckRotationSpeed.PointTurnSpeedExceedsLimit",
                         "%s: PointTurn speed %d deg/s exceeds limit of %f deg/s. Clamping",
                         animNameDebug.c_str(),
                         std::abs(_streamMsg.speed),
                         MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
        _streamMsg.speed = CLIP((f32)_streamMsg.speed,
                                -MAX_BODY_ROTATION_SPEED_DEG_PER_SEC,
                                MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
      }
    }

    void BodyMotionKeyFrame::CheckStraightSpeed(const std::string& animNameDebug)
    {
      // Check that speed is valid
      if (std::abs(_streamMsg.speed) > MAX_WHEEL_SPEED_MMPS) {
        PRINT_NAMED_INFO("BodyMotionKeyFrame.CheckStraightSpeed.StraightSpeedExceedsLimit",
                         "%s: Speed %d mm/s exceeds limit of %f mm/s. Clamping",
                         animNameDebug.c_str(),
                         std::abs(_streamMsg.speed), MAX_WHEEL_SPEED_MMPS);
        _streamMsg.speed = CLIP((f32)_streamMsg.speed, -MAX_WHEEL_SPEED_MMPS, MAX_WHEEL_SPEED_MMPS);
      }
    }

    void BodyMotionKeyFrame::CheckTurnSpeed(const std::string& animNameDebug)
    {
      // Check that speed is valid
      // NOTE: This should actually be checking the speed of the outer wheel
      //       when driving at the given curvature, but not exactly sure what
      //       speed limit should look like between straight and point turns so
      //       just using straight limit for now as a sanity check.
      if (std::abs(_streamMsg.speed) > MAX_WHEEL_SPEED_MMPS) {
        PRINT_NAMED_INFO("BodyMotionKeyFrame.CheckTurnSpeed.ArcSpeedExceedsLimit",
                         "%s: Speed %d mm/s exceeds limit of %f mm/s. Clamping",
                         animNameDebug.c_str(),
                         std::abs(_streamMsg.speed), MAX_WHEEL_SPEED_MMPS);
        _streamMsg.speed = CLIP((f32)_streamMsg.speed, -MAX_WHEEL_SPEED_MMPS, MAX_WHEEL_SPEED_MMPS);
      }
    }

    Result BodyMotionKeyFrame::DefineFromFlatBuf(const CozmoAnim::BodyMotion* bodyKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(bodyKeyframe != nullptr, "BodyMotionKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(bodyKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      Result lastResult = SetMembersFromFlatBuf(bodyKeyframe, animNameDebug);
      return lastResult;
    }

    Result BodyMotionKeyFrame::SetMembersFromFlatBuf(const CozmoAnim::BodyMotion* bodyKeyframe, const std::string& animNameDebug)
    {
      SafeNumericCast(bodyKeyframe->durationTime_ms(), _durationTime_ms, animNameDebug.c_str());
      SafeNumericCast(bodyKeyframe->speed(),           _streamMsg.speed, animNameDebug.c_str());

      const std::string& radiusStr = bodyKeyframe->radius_mm()->str();
      if (has_any_digits(radiusStr)) {
        SafeNumericCast(std::atoi(radiusStr.c_str()), _streamMsg.curvatureRadius_mm, animNameDebug.c_str());
        CheckTurnSpeed(animNameDebug);
      } else {
        Result lastResult = ProcessRadiusString(radiusStr, animNameDebug);
        if(lastResult != RESULT_OK) {
          return lastResult;
        }
      }

      return RESULT_OK;
    }

    Result BodyMotionKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, durationTime_ms);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, speed, streamMsg.speed);
      
      if(!jsonRoot.isMember("radius_mm")) {
        PRINT_NAMED_ERROR("BodyMotionKeyFrame.SetMembersFromJson.MissingRadius",
                          "%s: Missing 'radius_mm' field.",
                          animNameDebug.c_str());
        return RESULT_FAIL;
      } else if(jsonRoot["radius_mm"].isString()) {
        const std::string& radiusStr = jsonRoot["radius_mm"].asString();
        Result lastResult = ProcessRadiusString(radiusStr, animNameDebug);
        if(lastResult != RESULT_OK) {
          return lastResult;
        }
      } else {
        GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, radius_mm, streamMsg.curvatureRadius_mm);
        CheckTurnSpeed(animNameDebug);
      }
      
      return RESULT_OK;
    }
    
    Result BodyMotionKeyFrame::ProcessRadiusString(const std::string& radiusStr, const std::string& animNameDebug)
    {
      if(radiusStr == "TURN_IN_PLACE" || radiusStr == "POINT_TURN") {
        _streamMsg.curvatureRadius_mm = 0;
        CheckRotationSpeed(animNameDebug);
      } else if(radiusStr == "STRAIGHT") {
        _streamMsg.curvatureRadius_mm = std::numeric_limits<s16>::max();
        CheckStraightSpeed(animNameDebug);
      } else {
        PRINT_NAMED_ERROR("BodyMotionKeyFrame.BadRadiusString",
                          "%s: Unrecognized string for 'radius_mm' field: %s",
                          animNameDebug.c_str(),
                          radiusStr.c_str());
        return RESULT_FAIL;
      }
      return RESULT_OK;
    }

    RobotInterface::EngineToRobot* BodyMotionKeyFrame::GetStreamMessage()
    {
      //PRINT_NAMED_INFO("BodyMotionKeyFrame.GetStreamMessage",
      //                 "currentTime=%d, duration=%d\n", _currentTime_ms, _duration_ms);
      
      if(GetCurrentTime() == 0) {
        // Send the motion command at the beginning
        return new RobotInterface::EngineToRobot(AnimKeyFrame::BodyMotion(_streamMsg));
      } else if(_enableStopMessage && GetCurrentTime() >= _durationTime_ms) {
        // Send a stop command when the duration has passed
        return new RobotInterface::EngineToRobot(AnimKeyFrame::BodyMotion(_stopMsg));
      } else {
        // Do nothing in the middle or if no done message is required.
        // (Note that IsDone() will return false during
        // this period so the animation track won't advance.)
        return nullptr;
      }
    }
    
    bool BodyMotionKeyFrame::IsDone()
    {
      // Done once enough time has ticked by or if we're not sending a done message
      if(!_enableStopMessage)
      {
        return true;
      }
      
      return IsDoneHelper(_durationTime_ms);
    }

#pragma mark -
#pragma mark RecordHeadingKeyFrame
    
    RecordHeadingKeyFrame::RecordHeadingKeyFrame()
    {
    }
    
    Result RecordHeadingKeyFrame::DefineFromFlatBuf(const CozmoAnim::RecordHeading* recordHeadingKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(recordHeadingKeyframe != nullptr, "RecordedHeadingKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(recordHeadingKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      Result lastResult = SetMembersFromFlatBuf(recordHeadingKeyframe, animNameDebug);
      return lastResult;
    }
    
    Result RecordHeadingKeyFrame::SetMembersFromFlatBuf(const CozmoAnim::RecordHeading* recordHeadingKeyframe, const std::string& animNameDebug)
    {
      return RESULT_OK;
    }
    
    Result RecordHeadingKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      return RESULT_OK;
    }
    
    RobotInterface::EngineToRobot* RecordHeadingKeyFrame::GetStreamMessage()
    {
      return new RobotInterface::EngineToRobot(AnimKeyFrame::RecordHeading(_streamMsg));
    }
    
    bool RecordHeadingKeyFrame::IsDone()
    {
      return true;
    }
    
    
#pragma mark -
#pragma mark TurnToRecordedHeadingKeyFrame
    
    TurnToRecordedHeadingKeyFrame::TurnToRecordedHeadingKeyFrame()
    {
    }
    
    TurnToRecordedHeadingKeyFrame::TurnToRecordedHeadingKeyFrame(s16 offset_deg,
                                                                 s16 speed_degPerSec,
                                                                 s16 accel_degPerSec2,
                                                                 s16 decel_degPerSec2,
                                                                 u16 tolerance_deg,
                                                                 u16 numHalfRevs,
                                                                 bool useShortestDir,
                                                                 s32 duration_ms)
    : TurnToRecordedHeadingKeyFrame()
    {
      _durationTime_ms = duration_ms;
      _streamMsg.offset_deg = offset_deg;
      _streamMsg.speed_degPerSec = speed_degPerSec;
      _streamMsg.accel_degPerSec2 = accel_degPerSec2;
      _streamMsg.decel_degPerSec2 = decel_degPerSec2;
      _streamMsg.tolerance_deg = tolerance_deg;
      _streamMsg.numHalfRevs = numHalfRevs;
      _streamMsg.useShortestDir = useShortestDir;
    }
    
    void TurnToRecordedHeadingKeyFrame::CheckRotationSpeed(const std::string& animNameDebug)
    {
      // Check that speed is valid
      if (std::abs(_streamMsg.speed_degPerSec) > MAX_BODY_ROTATION_SPEED_DEG_PER_SEC) {
        PRINT_NAMED_INFO("TurnToRecordedHeadingKeyFrame.CheckRotationSpeed.PointTurnSpeedExceedsLimit",
                         "%s: PointTurn speed %d deg/s exceeds limit of %f deg/s. Clamping",
                         animNameDebug.c_str(),
                         std::abs(_streamMsg.speed_degPerSec),
                         MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
        _streamMsg.speed_degPerSec = CLIP((f32)_streamMsg.speed_degPerSec,
                                          -MAX_BODY_ROTATION_SPEED_DEG_PER_SEC,
                                          MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
      }
      
      // Check that accel/decel are within range
      if (std::abs(_streamMsg.accel_degPerSec2) > MAX_BODY_ROTATION_ACCEL_DEG_PER_SEC2) {
        PRINT_NAMED_INFO("TurnToRecordedHeadingKeyFrame.CheckRotationAccel.PointTurnAccelExceedsLimit",
                         "%s: PointTurn accel %d deg/s^2 exceeds limit of %f deg/s^2. Clamping",
                         animNameDebug.c_str(),
                         std::abs(_streamMsg.accel_degPerSec2),
                         MAX_BODY_ROTATION_ACCEL_DEG_PER_SEC2);
        _streamMsg.accel_degPerSec2 = CLIP((f32)_streamMsg.accel_degPerSec2,
                                          -MAX_BODY_ROTATION_ACCEL_DEG_PER_SEC2,
                                          MAX_BODY_ROTATION_ACCEL_DEG_PER_SEC2);
      }
      if (std::abs(_streamMsg.decel_degPerSec2) > MAX_BODY_ROTATION_ACCEL_DEG_PER_SEC2) {
        PRINT_NAMED_INFO("TurnToRecordedHeadingKeyFrame.CheckRotationAccel.PointTurnDecelExceedsLimit",
                         "%s: PointTurn decel %d deg/s^2 exceeds limit of %f deg/s^2. Clamping",
                         animNameDebug.c_str(),
                         std::abs(_streamMsg.decel_degPerSec2),
                         MAX_BODY_ROTATION_ACCEL_DEG_PER_SEC2);
        _streamMsg.decel_degPerSec2 = CLIP((f32)_streamMsg.decel_degPerSec2,
                                           -MAX_BODY_ROTATION_ACCEL_DEG_PER_SEC2,
                                           MAX_BODY_ROTATION_ACCEL_DEG_PER_SEC2);
      }
    }
    
    Result TurnToRecordedHeadingKeyFrame::DefineFromFlatBuf(const CozmoAnim::TurnToRecordedHeading* turnToRecordedHeadingKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(turnToRecordedHeadingKeyframe != nullptr, "TurnToRecordedHeadingKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(turnToRecordedHeadingKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      Result lastResult = SetMembersFromFlatBuf(turnToRecordedHeadingKeyframe, animNameDebug);
      return lastResult;
    }
    
    Result TurnToRecordedHeadingKeyFrame::SetMembersFromFlatBuf(const CozmoAnim::TurnToRecordedHeading* turnToRecordedHeadingKeyframe, const std::string& animNameDebug)
    {
      const char* const dbgName = animNameDebug.c_str();
      SafeNumericCast(turnToRecordedHeadingKeyframe->durationTime_ms(),  _durationTime_ms,            dbgName);
      SafeNumericCast(turnToRecordedHeadingKeyframe->offset_deg(),       _streamMsg.offset_deg,       dbgName);
      SafeNumericCast(turnToRecordedHeadingKeyframe->speed_degPerSec(),  _streamMsg.speed_degPerSec,  dbgName);
      SafeNumericCast(turnToRecordedHeadingKeyframe->accel_degPerSec2(), _streamMsg.accel_degPerSec2, dbgName);
      SafeNumericCast(turnToRecordedHeadingKeyframe->decel_degPerSec2(), _streamMsg.decel_degPerSec2, dbgName);
      SafeNumericCast(turnToRecordedHeadingKeyframe->tolerance_deg(),    _streamMsg.tolerance_deg,    dbgName);
      SafeNumericCast(turnToRecordedHeadingKeyframe->numHalfRevs(),      _streamMsg.numHalfRevs,      dbgName);
      _streamMsg.useShortestDir = turnToRecordedHeadingKeyframe->useShortestDir();
      
      CheckRotationSpeed(animNameDebug);
      
      return RESULT_OK;
    }
    
    Result TurnToRecordedHeadingKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, durationTime_ms);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, offset_deg,       streamMsg.offset_deg);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, speed_degPerSec,  streamMsg.speed_degPerSec);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, accel_degPerSec2, streamMsg.accel_degPerSec2);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, decel_degPerSec2, streamMsg.decel_degPerSec2);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, tolerance_deg,    streamMsg.tolerance_deg);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, numHalfRevs,      streamMsg.numHalfRevs);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, useShortestDir,   streamMsg.useShortestDir);
      
      CheckRotationSpeed(animNameDebug);

      return RESULT_OK;
    }
    
    
    RobotInterface::EngineToRobot* TurnToRecordedHeadingKeyFrame::GetStreamMessage()
    {
      return new RobotInterface::EngineToRobot(AnimKeyFrame::TurnToRecordedHeading(_streamMsg));
    }
    
    bool TurnToRecordedHeadingKeyFrame::IsDone()
    {
      return IsDoneHelper(_durationTime_ms);
    }
    
  } // namespace Cozmo
} // namespace Anki
