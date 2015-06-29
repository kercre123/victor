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
    
#pragma mark -
#pragma mark IKeyFrame
    
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
    
    
#pragma mark -
#pragma mark Helpers
    
    static s32 RandHelper(const s32 minLimit, const s32 maxLimit)
    {
      const s32 num = (std::rand() % (maxLimit - minLimit + 1)) + minLimit;
      
      assert(num >= minLimit && num <= maxLimit);
      
      return num;
    }
    
    
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
#pragma mark RobotAudioKeyFrame
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
    
#pragma mark -
#pragma mark DeviceAudioKeyFrame
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
#pragma mark BackpackLightsKeyFrame
    
    static u32 GetColorAsU32(Json::Value& json)
    {
      u32 retVal = 0;
      
      if(json.isString()) {
        Json::Value temp; // can't seem to turn input json into array directly
        const ColorRGBA color( NamedColors::GetByString(json.asString()));
        retVal = u32(color);
        
      } else if(json.isArray() && json.size() == 3) {
        const ColorRGBA color(static_cast<u8>(json[0].asUInt()),
                              static_cast<u8>(json[1].asUInt()),
                              static_cast<u8>(json[2].asUInt()));
        retVal = u32(color);
      } else {
        PRINT_NAMED_WARNING("GetColor", "Expecting color in Json to be a string or 3-element array, not changing.\n");
      }
      
      return retVal;
    } // GetColor()
    
    
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
#pragma mark BodyPositionKeyFrame
    
    Result BodyPositionKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
    {
      // For now just store the wheel speeds directly in the message.
      // Once we decide what the Json will actually contain, we may need
      // to read into some private members and fill the streaming message
      // dynamically in GetStreamMessage()
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, wheelSpeedL, streamMsg.wheelSpeedL_mmps);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, wheelSpeedR, streamMsg.wheelSpeedR_mmps);
      
      return RESULT_OK;
    }
    
    RobotMessage* BodyPositionKeyFrame::GetStreamMessage()
    {
      return &_streamMsg;
    }
    
  } // namespace Cozmo
} // namespace Anki