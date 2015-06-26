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

#include "anki/util/logging/logging.h"
#include "anki/common/basestation/colorRGBA.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/common/basestation/jsonTools.h"

#include <cassert>


namespace Anki {
  namespace Cozmo {
    
    
    
    IKeyFrame::IKeyFrame()
    : _triggerTime_ms(0)
    , _isValid(false)
    {
      
    }
    
    IKeyFrame::~IKeyFrame()
    {
      
    }
    
    bool IKeyFrame::IsTimeToPlay(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms) const
    {
      return (GetTriggerTime() + startTime_ms <= currTime_ms);
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
    
    static s32 RandHelper(const s32 minLimit, const s32 maxLimit)
    {
      const s32 num = (std::rand() % (maxLimit - minLimit + 1)) + minLimit;
      
      assert(num >= minLimit && num <= maxLimit);
      
      return num;
    }
    
    
    // Helper macro used in SetMembersFromJson() overrides below to look for
    // member variable in Json node and fail if it doesn't exist
# define GET_MEMBER_FROM_JSON(__JSON__, __NAME__) do { \
if(!JsonTools::GetValueOptional(__JSON__, QUOTE(__NAME__), this->_##__NAME__)) { \
PRINT_NAMED_ERROR("IKeyFrame.GetMemberFromJsonMacro", \
"Failed to get '%s' from Json file.", QUOTE(__NAME__)); \
return RESULT_FAIL; \
} } while(0)
    
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
        _streamHeadMsg.angle_deg = static_cast<s8>(RandHelper(_angle_deg - _angleVariability_deg,
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
    
    //
    // LiftHeightKeyFrame
    //
    
    RobotMessage* LiftHeightKeyFrame::GetStreamMessage()
    {
      _streamLiftMsg.time_ms = _durationTime_ms;
      
      // Add variability:
      if(_heightVariability_mm > 0) {
        _streamLiftMsg.height_mm = static_cast<s8>(RandHelper(_height_mm - _heightVariability_mm,
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
      // Fill the message for streaming using the imageID
      // memcpy(_streamMsg.image, LoadFaceImage(_imageID);
      
      return &_streamMsg;
    }
    
    
    
    //
    // RobotAudioKeyFrame
    //
    
    Result RobotAudioKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, audioID);
      
      // TODO: Compute number of samples for this audio ID
      // TODO: Catch failure if ID is invalid
      // _numSamples = wwise::GetNumSamples(_idMsg.audioID, SAMPLE_SIZE);
      
      _sampleIndex = 0;
      
      return RESULT_OK;
    }
    
    RobotMessage* RobotAudioKeyFrame::GetStreamMessage()
    {
      // Populate the message with the next chunk of audio data and send it out
      if(_sampleIndex < _numSamples) {
        
        // TODO: Get next chunk of audio from wwise or something?
        //wwise::GetNextSample(_audioSampleMsg.sample, 480);
        
        ++_sampleIndex;
        
        return &_audioSampleMsg;
      } else {
        return nullptr;
      }
    }
    
    //
    // DeviceAudioKeyFrame
    //
    
    Result DeviceAudioKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, audioID);
      
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
      SoundManager::getInstance()->Play(static_cast<SoundID_t>(_audioID));
    }
    
    //
    // FacePositionKeyFrame
    //
    
    RobotMessage* FacePositionKeyFrame::GetStreamMessage()
    {
      _streamMsg.xCen = _xcen;
      _streamMsg.yCen = _ycen;
      
      return &_streamMsg;
    }
    
    Result FacePositionKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
    {
      GET_MEMBER_FROM_JSON(jsonRoot, xcen);
      GET_MEMBER_FROM_JSON(jsonRoot, ycen);
      
      return RESULT_OK;
    }
    
  
  } // namespace Cozmo
} // namespace Anki