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


#include "anki/cozmo/basestation/keyframe.h"

#include "anki/cozmo/basestation/robot.h"

#include "robotMessageHandler.h"

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/cozmo/basestation/soundManager.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"

#include "util/logging/logging.h"
#include "anki/common/basestation/colorRGBA.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/common/basestation/jsonTools.h"

#include <opencv2/core/core.hpp>

#include <cassert>

namespace Anki {
  namespace Cozmo {
    
#pragma mark -
#pragma mark IKeyFrame
    
    IKeyFrame::IKeyFrame()
    : _triggerTime_ms(0)
    , _isValid(false)
    , _isLive(false)
    {
      
    }
    
    IKeyFrame::~IKeyFrame()
    {
      
    }
    
    bool IKeyFrame::IsTimeToPlay(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms) const
    {
      return IsLive() || (GetTriggerTime() + startTime_ms <= currTime_ms);
    }
    
    Result IKeyFrame::DefineFromJson(const Json::Value &json)
    {
      Result lastResult = SetMembersFromJson(json);
      
      if(lastResult == RESULT_OK) {
        
        // Read the frame time from the json file as well
        if(!json.isMember("triggerTime_ms")) {
          PRINT_NAMED_ERROR("IKeyFrame.ReadFromJson", "Expecting 'triggerTime_ms' field in KeyFrame Json.\n");
          lastResult = RESULT_FAIL;
        } else {
          _triggerTime_ms = json["triggerTime_ms"].asUInt();
          
          // Only way to set isValid=true is that SetMembersFromJson succeeded and
          // triggerTime was found:
          _isValid = true;
        }
      }
      
      return lastResult;
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

    
#pragma mark -
#pragma mark HeadAngleKeyFrame
    //
    // HeadAngleKeyFrame
    //
    
    /*
     HeadAngleKeyFrame::HeadAngleKeyFrame(s8 angle_deg, u8 angle_variability_deg, TimeStamp_t duration)
     : _durationTime_ms(duration)
     , _angle_deg(angle_deg)
     , _angleVariability_deg(angle_variability_deg)
     {
     IKeyFrame::SetIsValid(true);
     }
     */
    
    RobotMessage* HeadAngleKeyFrame::GetStreamMessage()
    {
      _streamHeadMsg.time_ms = _durationTime_ms;
      
      // Add variability:
      if(_angleVariability_deg > 0) {
        _streamHeadMsg.angle_deg = static_cast<s8>(_rng.RandIntInRange(_angle_deg - _angleVariability_deg,
                                                                       _angle_deg + _angleVariability_deg));
      } else {
        _streamHeadMsg.angle_deg = _angle_deg;
      }
      
      return &_streamHeadMsg;
    }
    
    Result HeadAngleKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
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
    
    RobotMessage* LiftHeightKeyFrame::GetStreamMessage()
    {
      _streamLiftMsg.time_ms = _durationTime_ms;
      
      // Add variability:
      if(_heightVariability_mm > 0) {
        _streamLiftMsg.height_mm = static_cast<s8>(_rng.RandIntInRange(_height_mm - _heightVariability_mm,
                                                                       _height_mm + _heightVariability_mm));
      } else {
        _streamLiftMsg.height_mm = _height_mm;
      }
      
      return &_streamLiftMsg;
    }
    
    Result LiftHeightKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
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
    
    Result FaceImageKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, imageID);
      
      return RESULT_OK;
    }
    
    RobotMessage* FaceImageKeyFrame::GetStreamMessage()
    {
      // TODO: Fill the message for streaming using the imageID
      // memcpy(_streamMsg.image, LoadFaceImage(_imageID);
      
      // For now, just put some stuff for display in there, using a few hard-coded
      // patterns depending on ID
      _streamMsg.image.fill(0);
      if(_imageID == 0) {
        // All black
        _streamMsg.image[0] = 0;
      } else if(_imageID == 1) {
        // All blue
        _streamMsg.image[0] = 64; // Switch to blue
        _streamMsg.image[1] = 63; // Fill lines
        _streamMsg.image[2] = 0;  // Done
      } else {
        // Draw "programmer art" face until we get real assets
        
        static const u8 simpleFace[] = { 24, 64+24,   // Start 24 lines down and 24 pixels right
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          64+16, 64+48, 64+16, 64+48+128,  // One line of eyes
          0 };
        
        for(s32 i=0; i<sizeof(simpleFace); ++i) {
          _streamMsg.image[i] = simpleFace[i];
        }
      }
      
      return &_streamMsg;
    }
    
#pragma mark -
#pragma mark FaceAnimationKeyFrame
    
    Result FaceAnimationKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, animName);
      
      // TODO: Take this out once root path is part of AnimationTool!
      size_t lastSlash = _animName.find_last_of("/");
      if(lastSlash != std::string::npos) {
        PRINT_NAMED_WARNING("FaceAnimationKeyFrame.SetMembersFromJson",
                            "Removing path from animation name: %s\n",
                            _animName.c_str());
        _animName = _animName.substr(lastSlash+1, std::string::npos);
      }
      
      _curFrame = 0;
      
      return RESULT_OK;
    }
    
    bool FaceAnimationKeyFrame::IsDone()
    {
      // Note the dynamic check for num frames, since (in the case of streaming
      // procedural animations) the number of keyframes could be increasing
      // while we're playing and thus isn't known up front.
      return _curFrame >= FaceAnimationManager::getInstance()->GetNumFrames(_animName);
    }
    
    RobotMessage* FaceAnimationKeyFrame::GetStreamMessage()
    {
      // Populate the message with the next chunk of audio data and send it out
      
      if(!IsDone()) 
      {
        const std::vector<u8>* rleFrame = FaceAnimationManager::getInstance()->GetFrame(_animName, _curFrame);
        
        if(rleFrame == nullptr) {
          PRINT_NAMED_ERROR("FaceAnimationKeyFrame.GetStreamMesssage",
                            "Failed to get frame %d from animation %s.\n",
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
        
        if(rleFrame->size() >= sizeof(_faceImageMsg.image)) {
          PRINT_NAMED_ERROR("FaceAnimationKeyFrame.GetStreamMessage",
                            "RLE frame %d for animation %s too large to fit in message (%lu>=%lu).\n",
                            _curFrame, _animName.c_str(), rleFrame->size(), sizeof(_faceImageMsg.image));
          return nullptr;
        }
        
        _faceImageMsg.image.fill(0); // Necessary??
        std::copy(rleFrame->begin(), rleFrame->end(), _faceImageMsg.image.begin());
        ++_curFrame;
        
        return &_faceImageMsg;
      } else {
        _curFrame = 0;
        return nullptr;
      }
    }
    
#pragma mark -
#pragma mark ProceduralFaceKeyFrame
    
    static Result SetEyeArrayHelper(ProceduralFace::WhichEye whichEye,
                                  const Json::Value& jsonRoot,
                                    ProceduralFace& face)
    {
      std::array<ProceduralFace::Value,static_cast<size_t>(ProceduralEyeParameter::NumParameters)> eyeArray;
      const char* eyeStr = (whichEye == ProceduralFace::WhichEye::Left ?
                            "leftEye" : "rightEye");
      
      if(JsonTools::GetArrayOptional(jsonRoot, eyeStr, eyeArray)) {
        for(s32 i=0; i<eyeArray.size(); ++i)
        {
          face.SetParameter(whichEye,
                            static_cast<ProceduralFace::Parameter>(i),
                            eyeArray[i]);
        }
      }
      
      return RESULT_OK;
    }
    
    Result ProceduralFaceKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
    {
      
      ProceduralFace::Value angle;
      if(!JsonTools::GetValueOptional(jsonRoot, "faceAngle", angle)) {
        if(JsonTools::GetValueOptional(jsonRoot, "faceAngle_deg", angle)) {
          PRINT_NAMED_WARNING("ProceduralFaceKeyFrame.SetMembersFromJson.OldAngleName",
                              "Animation JSON memeber 'faceAngle_deg' is deprecated. "
                              "Please update to 'faceAngle'.");
        } else {
          PRINT_NAMED_WARNING("ProceduralFaceKeyFrame.SetMembersFromJson.MissingParam",
                              "Missing 'faceAngle' parameter - will use 0.");
          angle = 0;
        }
      }
      
      _procFace.SetFaceAngle(angle);
      
      //
      // Individual members for each parameter
      // TODO: Remove support for these old-style Json files?
      //
      for(int iParam=0; iParam < static_cast<int>(ProceduralFace::Parameter::NumParameters); ++iParam)
      {
        ProceduralEyeParameter param = static_cast<ProceduralEyeParameter>(iParam);
        
        ProceduralFace::Value value;
        std::string paramName = ProceduralEyeParameterToString(param);
        
        // Left
        if(JsonTools::GetValueOptional(jsonRoot, "left" + paramName, value)) {
          _procFace.SetParameter(ProceduralFace::Left, param, value);
        }
        
        // Right
        if(JsonTools::GetValueOptional(jsonRoot, "right" + paramName, value)) {
          _procFace.SetParameter(ProceduralFace::Right, param, value);
        }
      }
      
      //
      // Single array for left and right eye, indexed by enum value
      //  Note: these will overwrite any individual values found above.
      Result lastResult = SetEyeArrayHelper(ProceduralFace::WhichEye::Left,
                                            jsonRoot, _procFace);
      if(RESULT_OK == lastResult) {
        lastResult = SetEyeArrayHelper(ProceduralFace::WhichEye::Right,
                                     jsonRoot, _procFace);
      }

      Reset();
      
      return lastResult;
    }
    
    void ProceduralFaceKeyFrame::Reset()
    {
      _currentTime_ms = GetTriggerTime();
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
    
    RobotMessage* ProceduralFaceKeyFrame::GetStreamMessageHelper(const ProceduralFace& procFace)
    {
      std::vector<u8> rleData;
      Result rleResult = FaceAnimationManager::CompressRLE(procFace.GetFace(), rleData);
      
      if(RESULT_OK != rleResult) {
        PRINT_NAMED_ERROR("ProceduralFaceKeyFrame.GetStreamMesssageHelper",
                          "Failed to get RLE frame from procedural face.");
        return nullptr;
      }
      
      if(rleData.size() >= sizeof(_faceImageMsg.image)) {
        PRINT_NAMED_ERROR("ProceduralFaceKeyFrame.GetStreamMessageHelper",
                          "RLE frame for procedural face too large to fit in message (%lu>=%lu).",
                          rleData.size(), sizeof(_faceImageMsg.image));
        return nullptr;
      }
      
      _faceImageMsg.image.fill(0);
      std::copy(rleData.begin(), rleData.end(), _faceImageMsg.image.begin());
      
      return &_faceImageMsg;
    }
    
    RobotMessage* ProceduralFaceKeyFrame::GetStreamMessage()
    {
      _isDone = true;
      return GetStreamMessageHelper(_procFace);
    }
    
    RobotMessage* ProceduralFaceKeyFrame::GetInterpolatedStreamMessage(const ProceduralFaceKeyFrame& nextFrame)
    {
      // The interpolation fraction is how far along in time we are from this frame's
      // trigger time (which currentTime was initialized to) and the next frame's
      // trigger time.
      const f32 fraction = std::min(1.f, static_cast<f32>(_currentTime_ms - GetTriggerTime()) / static_cast<f32>(nextFrame.GetTriggerTime() - GetTriggerTime()));
      
      ProceduralFace interpFace;
      interpFace.Interpolate(_procFace, nextFrame._procFace, fraction);
      
      _currentTime_ms += IKeyFrame::SAMPLE_LENGTH_MS;
      if(_currentTime_ms >= nextFrame.GetTriggerTime()) {
        _isDone = true;
      }
      
      return GetStreamMessageHelper(interpFace);
    }
    
#pragma mark -
#pragma mark RobotAudioKeyFrame
    
    //
    // RobotAudioKeyFrame
    //
    
    const std::string& RobotAudioKeyFrame::GetSoundName() const
    {
      static const std::string unknownName("UNKNOWN");
      if(_selectedAudioIndex<0 || _audioReferences.empty()) {
        PRINT_NAMED_ERROR("RobotAudioKeyFrame.GetSoundName.InvalidState",
                          "KeyFrame not initialzed with selected sound name yet.\n");
        return unknownName;
      }
      
      return _audioReferences[_selectedAudioIndex].name;
    }
    
    Result RobotAudioKeyFrame::AddAudioRef(const std::string& name, const f32 volume)
    {
      // TODO: Compute number of samples for this audio ID
      // TODO: Catch failure if ID is invalid
      // _numSamples = wwise::GetNumSamples(_idMsg.audioID, SAMPLE_SIZE);
      
      AudioRef audioRef;
      const u32 duration_ms = SoundManager::getInstance()->GetSoundDurationInMilliseconds(name);
      if(duration_ms == 0) {
        PRINT_NAMED_ERROR("RobotAudioKeyFrame.AddAudioRef.InvalidName",
                          "SoundManager could not find the sound associated with name '%s'.\n",
                          name.c_str());
        return RESULT_FAIL;
      }
      
      audioRef.name = name;
      audioRef.numSamples = duration_ms / SAMPLE_LENGTH_MS;
      audioRef.volume = volume;
      _audioReferences.push_back(audioRef);
      
      return RESULT_OK;
    }
    
    Result RobotAudioKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
    {
      // Get volume
      f32 volume = 1.0;
      JsonTools::GetValueOptional(jsonRoot, "volume", volume);
      
      
      if(!jsonRoot.isMember("audioName")) {
        PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromJson.MissingAudioName",
                          "No 'audioName' field in Json frame.\n");
        return RESULT_FAIL;
      }
      
      const Json::Value& jsonAudioNames = jsonRoot["audioName"];
      if(jsonAudioNames.isArray()) {
        for(s32 i=0; i<jsonAudioNames.size(); ++i) {
          Result addResult = AddAudioRef(jsonAudioNames[i].asString(), volume);
          if(addResult != RESULT_OK) {
            return addResult;
          }
        }
      } else {
        Result addResult = AddAudioRef(jsonAudioNames.asString(), volume);
        if(addResult != RESULT_OK) {
          return addResult;
        }
      }
      
      _sampleIndex = 0;
      
      return RESULT_OK;
    }
    
    RobotMessage* RobotAudioKeyFrame::GetStreamMessage()
    {
      if(_sampleIndex == 0) {
        // Select one of the audio names to play
        if(_audioReferences.size()==1) {
          // Special case: there's only one audio option 
          _selectedAudioIndex = 0;
        } else {
          _selectedAudioIndex = _rng.RandIntInRange(0, static_cast<s32>(_audioReferences.size()-1));
        }
      }
      
      // Populate the message with the next chunk of audio data and send it out
      if(_sampleIndex < _audioReferences[_selectedAudioIndex].numSamples)
      {
        // TODO: Get next chunk of audio from wwise or something?
        //wwise::GetNextSample(_audioSampleMsg.sample, 800);
        
        if (!SoundManager::getInstance()->GetSoundSample(_audioReferences[_selectedAudioIndex].name,
                                                         _sampleIndex,
                                                         _audioReferences[_selectedAudioIndex].volume,
                                                         _audioSampleMsg)) {
          PRINT_NAMED_WARNING("RobotAudioKeyFrame.GetStreamMessage.MissingSample", "Index %d", _sampleIndex);
        }
        
        ++_sampleIndex;
        
        return &_audioSampleMsg;
      } else {
        _sampleIndex = 0;
        return nullptr;
      }
    }
    
#pragma mark -
#pragma mark DeviceAudioKeyFrame
    //
    // DeviceAudioKeyFrame
    //
    
    Result DeviceAudioKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, audioName);
      
      return RESULT_OK;
    }
    
    RobotMessage* DeviceAudioKeyFrame::GetStreamMessage()
    {
      // Device audio is not streamed to the robot, by definition, so it always
      // returns nullptr
      return nullptr;
    }
    
    void DeviceAudioKeyFrame::PlayOnDevice()
    {
      // TODO: Replace with real call to wwise or something
      SoundManager::getInstance()->Play(_audioName);
    }
    
#pragma mark -
#pragma mark FacePositionKeyFrame
    //
    // FacePositionKeyFrame
    //
    
    RobotMessage* FacePositionKeyFrame::GetStreamMessage()
    {
      //_streamMsg.xCen = _xcen;
      //_streamMsg.yCen = _ycen;
      
      return &_streamMsg;
    }
    
    Result FacePositionKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
    {
      // Just store the center point directly in the message.
      // No need to duplicate since we don't do anything extra to the stored
      // values before streaming.
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, xcen, streamMsg.xCen);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, ycen, streamMsg.yCen);
      
      return RESULT_OK;
    }
    
#pragma mark -
#pragma mark BlinkKeyFrame
    
    BlinkKeyFrame::BlinkKeyFrame()
    : _curTime_ms(0)
    {
      
    }
    
    bool BlinkKeyFrame::IsDone()
    {
      if(_streamMsg.blinkNow) {
        return true;
      } else if(_curTime_ms >= _duration_ms) {
        _curTime_ms = 0; // Reset for next time
        return true;
      } else {
        _curTime_ms += SAMPLE_LENGTH_MS;
        return false;
      }
    }
    
    RobotMessage* BlinkKeyFrame::GetStreamMessage()
    {
      RobotMessage* returnMessage = nullptr;
      if(_streamMsg.blinkNow) {
        _streamMsg.enable = true;
        returnMessage = &_streamMsg;
      } else {
        // If not a blink now message, then must be a "disable blink for
        // some duration" keyframe.
        if(_curTime_ms == 0) {
          // Start of the keyframe period: disable blinking
          _streamMsg.enable = false;
          returnMessage = &_streamMsg;
        } else if(_curTime_ms >= _duration_ms) {
          // Done with disable period: re-enable.
          _streamMsg.enable = true;
          returnMessage = &_streamMsg;
        } else {
          // Don't do anything in the middle (and return nullptr)
          // Note that we will not advance to next keyframe during this period
          // because IsDone() will be false.
        }
      }
      
      return returnMessage;
    }
    
    Result BlinkKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
    {
      if(!jsonRoot.isMember("command")) {
        PRINT_NAMED_ERROR("BlinkKeyFrame.SetMembersFromJson.MissingCommand",
                          "Missing 'command' field.\n");
        return RESULT_FAIL;
      } else if(!jsonRoot["command"].isString()) {
        PRINT_NAMED_ERROR("BlinkKeyFrame.SetMembersFromJson.BadCommand",
                          "Expecting 'command' field to be a string.\n");
        return RESULT_FAIL;
      }
      
      const std::string& commandStr = jsonRoot["command"].asString();
      if(commandStr == "BLINK") {
        // Blink now, duration and "enable" don't matter
        _streamMsg.blinkNow = true;
      } else if(commandStr == "DISABLE") {
        // Disable blinking for the given duration
        _streamMsg.blinkNow = false;
        GET_MEMBER_FROM_JSON(jsonRoot, duration_ms);
      } else {
        PRINT_NAMED_ERROR("BlinkKeyFrame.SetMembersFromJson.BadCommandString",
                          "Unrecognized string for 'command' field: %s.\n",
                          commandStr.c_str());
        return RESULT_FAIL;
      }
      
      return RESULT_OK;
    }
    
#pragma mark -
#pragma mark BackpackLightsKeyFrame
    
    Result BackpackLightsKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
    {
      ColorRGBA color;
     
      // Special helper macro for getting the LED colors out of the Json and
      // store them directly in the streamMsg
#define GET_COLOR_FROM_JSON(__NAME__, __LED_NAME__) do { \
if(!JsonTools::GetColorOptional(jsonRoot, QUOTE(__NAME__), color)) { \
PRINT_NAMED_ERROR("BackpackLightsKeyFrame.SetMembersFromJson", \
"Failed to get '%s' LED color from Json file.\n", QUOTE(__NAME__)); \
return RESULT_FAIL; \
} \
_streamMsg.colors[__LED_NAME__] = u32(color) >> 8; } while(0) // Note we shift the Alpha out, since it's unused
    
      GET_COLOR_FROM_JSON(Back,   LED_BACKPACK_BACK);
      GET_COLOR_FROM_JSON(Front,  LED_BACKPACK_FRONT);
      GET_COLOR_FROM_JSON(Middle, LED_BACKPACK_MIDDLE);
      GET_COLOR_FROM_JSON(Left,   LED_BACKPACK_LEFT);
      GET_COLOR_FROM_JSON(Right,  LED_BACKPACK_RIGHT);
      
      return RESULT_OK;
    }
    
    
    RobotMessage* BackpackLightsKeyFrame::GetStreamMessage()
    {
      return &_streamMsg;
    }
  
    
#pragma mark -
#pragma mark BodyMotionKeyFrame
    
    BodyMotionKeyFrame::BodyMotionKeyFrame()
    : _currentTime_ms(0)
    {
      _stopMsg.speed = 0;
    }
    
    Result BodyMotionKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, durationTime_ms);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, speed, streamMsg.speed);
      
      if(!jsonRoot.isMember("radius_mm")) {
        PRINT_NAMED_ERROR("BodyMotionKeyFrame.SetMembersFromJson.MissingRadius",
                          "Missing 'radius_mm' field.\n");
        return RESULT_FAIL;
      } else if(jsonRoot["radius_mm"].isString()) {
        const std::string& radiusStr = jsonRoot["radius_mm"].asString();
        if(radiusStr == "TURN_IN_PLACE" || radiusStr == "POINT_TURN") {
          _streamMsg.curvatureRadius_mm = 0;
        } else if(radiusStr == "STRAIGHT") {
          _streamMsg.curvatureRadius_mm = s16_MAX;
        } else {
          PRINT_NAMED_ERROR("BodyMotionKeyFrame.BadRadiusString",
                            "Unrecognized string for 'radius_mm' field: %s.\n",
                            radiusStr.c_str());
          return RESULT_FAIL;
        }
      } else {
        _streamMsg.curvatureRadius_mm = jsonRoot["radius_mm"].asInt();
      }
      
      return RESULT_OK;
    }
    
    
    RobotMessage* BodyMotionKeyFrame::GetStreamMessage()
    {
      //PRINT_NAMED_INFO("BodyMotionKeyFrame.GetStreamMessage",
      //                 "currentTime=%d, duration=%d\n", _currentTime_ms, _duration_ms);
      
      if(_currentTime_ms == 0) {
        // Send the motion command at the beginning
        return &_streamMsg;
      } else if(_currentTime_ms >= _durationTime_ms) {
        // Send a stop command when the duration has passed
        return &_stopMsg;
      } else {
        // Do nothing in the middle. (Note that IsDone() will return false during
        // this period so the animation track won't advance.)
        return nullptr;
      }
    }
    
    bool BodyMotionKeyFrame::IsDone()
    {
      // Done once enough time has ticked by
      if(_currentTime_ms >= _durationTime_ms) {
        _currentTime_ms = 0; // Reset for next time
        return true;
      } else {
        _currentTime_ms += SAMPLE_LENGTH_MS;
        return false;
      }
    }
    
  } // namespace Cozmo
} // namespace Anki