/**
 * File: cannedAnimationContainer.cpp
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

#include "anki/cozmo/basestation/cannedAnimationContainer.h"

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

#define DEBUG_ANIMATION_STREAMING 1

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
    return (GetTriggerTime() + startTime_ms >= currTime_ms);
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
if(!JsonTools::GetValueOptional(__JSON__, QUOTE(__NAME__), this->__NAME__)) { \
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
    GET_MEMBER_FROM_JSON(jsonRoot, _durationTime_ms);
    GET_MEMBER_FROM_JSON(jsonRoot, _angle_deg);
    GET_MEMBER_FROM_JSON(jsonRoot, _angleVariability_deg);
    
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
    GET_MEMBER_FROM_JSON(jsonRoot, _durationTime_ms);
    GET_MEMBER_FROM_JSON(jsonRoot, _height_mm);
    GET_MEMBER_FROM_JSON(jsonRoot, _heightVariability_mm);
    
    return RESULT_OK;
  }
  
  //
  // FaceImageKeyFrame
  //
  
  Result FaceImageKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot)
  {
    GET_MEMBER_FROM_JSON(jsonRoot, _imageID);
    
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
    GET_MEMBER_FROM_JSON(jsonRoot, _audioID);
    
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
    GET_MEMBER_FROM_JSON(jsonRoot, _audioID);
    
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
    GET_MEMBER_FROM_JSON(jsonRoot, _xcen);
    GET_MEMBER_FROM_JSON(jsonRoot, _ycen);
    
    return RESULT_OK;
  }
  
  
  template<typename FRAME_TYPE>
  void Animation::Track<FRAME_TYPE>::Init()
  {
    _frameIter = _frames.begin();
  }
  
  template<typename FRAME_TYPE>
  RobotMessage* Animation::Track<FRAME_TYPE>::GetNextMessage(TimeStamp_t startTime_ms,
                                                             TimeStamp_t currTime_ms)
  {
    RobotMessage* msg = nullptr;
    
    if(HasFramesLeft() && GetNextFrame().IsTimeToPlay(startTime_ms, currTime_ms))
    {
      msg = GetNextFrame().GetStreamMessage();
      Increment();
    }
    
    return msg;
  }
  
  template<typename FRAME_TYPE>
  Result Animation::Track<FRAME_TYPE>::AddKeyFrame(const Json::Value &jsonRoot)
  {
    _frames.emplace_back();
    return _frames.back().DefineFromJson(jsonRoot);
  }

  
  Result Animation::DefineFromJson(Json::Value &jsonRoot)
  {
    if(!jsonRoot.isMember("Name")) {
      PRINT_NAMED_ERROR("Animation.DefineFromJson.NoName",
                        "Missing 'Name' field for animation.\n");
      return RESULT_FAIL;
    }
    
    _name = jsonRoot["Name"].asString();
   
    // Clear whatever is in the existing animation
    Clear();
    
    const s32 numFrames = jsonRoot.size();
    for(s32 iFrame = 0; iFrame < numFrames; ++iFrame)
    {
      Json::Value& jsonFrame = jsonRoot[iFrame];
      
      if(!jsonFrame.isMember("Name")) {
        PRINT_NAMED_ERROR("Animation.DefineFromJson.NoFrameName",
                          "Missing 'Name' field for frame %d of '%s' animation.\n",
                          iFrame, _name.c_str());
        return RESULT_FAIL;
      }
      
      const std::string& frameName = jsonFrame["Name"].asString();
      
      Result addResult = RESULT_FAIL;
      
      // Map from string name of frame to which track we want to store it in:
      if(frameName == HeadAngleKeyFrame::GetClassName()) {
        addResult = _headTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == LiftHeightKeyFrame::GetClassName()) {
        addResult = _liftTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == FaceImageKeyFrame::GetClassName()) {
        addResult = _faceImageTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == FacePositionKeyFrame::GetClassName()) {
        addResult = _facePosTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == DeviceAudioKeyFrame::GetClassName()) {
        addResult = _deviceAudioTrack.AddKeyFrame(jsonFrame);
      } else if(frameName == RobotAudioKeyFrame::GetClassName()) {
        addResult = _robotAudioTrack.AddKeyFrame(jsonFrame);
      } else {
        PRINT_NAMED_ERROR("Animation.DefineFromJson.UnrecognizedFrameName",
                          "Frame %d in '%s' animation has unrecognized name '%s'.\n",
                          iFrame, _name.c_str(), frameName.c_str());
        return RESULT_FAIL;
      }
      
      if(addResult != RESULT_OK) {
        PRINT_NAMED_ERROR("Animation.DefineFromJson.AddKeyFrameFailure",
                          "Adding %s frame %d failed.\n",
                          frameName.c_str(), iFrame);
        return addResult;
      }
      
    } // for each frame
    
    return RESULT_OK;
  }
  
  Result Animation::AddKeyFrame(const HeadAngleKeyFrame& kf)
  {
    return _headTrack.AddKeyFrame(kf);
  }
  
  // Helper macro for running a given method of all tracks and combining the result
  // in the specified way. To just call a method, use ";" for COMBINE_WITH, or
  // use "&&" or "||" to combine into a single result.
# define ALL_TRACKS(__METHOD__, __COMBINE_WITH__) \
_headTrack.__METHOD__() __COMBINE_WITH__ \
_liftTrack.__METHOD__() __COMBINE_WITH__ \
_faceImageTrack.__METHOD__() __COMBINE_WITH__ \
_facePosTrack.__METHOD__() __COMBINE_WITH__ \
_deviceAudioTrack.__METHOD__() __COMBINE_WITH__ \
_robotAudioTrack.__METHOD__()
  
  Result Animation::Init(Robot& robot)
  {
    _startTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    
    // Initialize "fake" streaming time to the same start time so we can compare
    // to it for determining when its time to stream out a keyframe
    _streamingTime_ms = _startTime_ms;
    
    ALL_TRACKS(Init, ;);
    
    return RESULT_OK;
  } // Animation::Init()
  
  
  Result Animation::Update(Robot& robot)
  {
    const TimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    
    // Is it time to play device audio? (using actual basestation time)
    if(_deviceAudioTrack.GetNextFrame().IsTimeToPlay(_startTime_ms, currTime_ms)) {
      _deviceAudioTrack.GetNextFrame().PlayOnDevice();
      _deviceAudioTrack.Increment();
    }

    // Don't send frames if robot has no space for them
    //const bool isRobotReadyForFrames = !robot.IsAnimationBufferFull();
    s32 numFramesToSend = robot.GetNumAnimationFramesFree();
    
    while(numFramesToSend-- > 0 && !IsEmpty())
    {
      RobotMessage* msg = nullptr;
      
      // Have to always send an audio frame to keep time, whether that's the next
      // audio sample or a silent frame. This increments "streamingTime"
      // NOTE: Audio frame must be first!
      if(_robotAudioTrack.HasFramesLeft())
      {
        msg = _robotAudioTrack.GetNextFrame().GetStreamMessage();
        if(msg != nullptr) {
          // Still have samples to send, don't increment to the next frame in the track
          robot.SendMessage(*msg);
          
        } else {
          // No samples left to send for this keyframe. Move to next, and for now
          // send silence.
          _robotAudioTrack.Increment();
          robot.SendMessage(_silenceMsg);
        }
      } else {
        // No frames left or not time to play next frame yet, so send silence
        robot.SendMessage(_silenceMsg);
      }
      
      // Increment fake "streaming" time, so we can evaluate below whether
      // it's time to stream out any of the other tracks. Note that it is still
      // relative to the same start time.
      _streamingTime_ms += RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
      
      //
      // We are guaranteed to have sent some kind of audio frame at this point.
      // Now send any other frames that are ready, so they will be timed with
      // that audio frame (silent or not).
      //
      // Note that these frames don't actually use up additional slots in the
      // robot's keyframe buffer, so we don't have to decrement numFramesToSend
      // for each one, just once for each audio/silence frame.
      //
      
      Result sendResult = RESULT_OK;
      
      msg = _headTrack.GetNextMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
        sendResult = robot.SendMessage(*msg);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      msg = _liftTrack.GetNextMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
        sendResult = robot.SendMessage(*msg);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      msg = _facePosTrack.GetNextMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
        sendResult = robot.SendMessage(*msg);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      msg = _faceImageTrack.GetNextMessage(_startTime_ms, _streamingTime_ms);
      if(msg != nullptr) {
        sendResult = robot.SendMessage(*msg);
        if(sendResult != RESULT_OK) { return sendResult; }
      }
      
      
    } // while(numFramesToSend > 0)
    
    
    return RESULT_OK;
    
  } // Animation::Update()
  
  void Animation::Clear()
  {
    ALL_TRACKS(Clear, ;);
  }
  
  bool Animation::IsEmpty() const
  {
    return ALL_TRACKS(IsEmpty, &&);
  }
  
  bool Animation::IsFinished() const
  {
    return !(ALL_TRACKS(HasFramesLeft, ||));
  }
  
  //////////////////////////////////////////////
  
  CannedAnimationContainer::CannedAnimationContainer()
  {
    DefineHardCoded();
  }
  
  Result CannedAnimationContainer::AddAnimation(const std::string& name)
  {
    Result lastResult = RESULT_OK;
    
    auto retVal = _animations.find(name);
    if(retVal == _animations.end()) {
      const s32 animID = static_cast<s32>(_animations.size());
      _animations[name].first = animID;
      
      PRINT_NAMED_INFO("CannedAnimationContainer.AddAnimation",
                       "Adding new animation named '%s' with ID=%d\n",
                       name.c_str(), animID);
      
    } else {
      PRINT_NAMED_ERROR("CannedAnimationContainer.AddAnimation.DuplicateName",
                        "Animation named '%s' already exists. Not adding.\n",
                        name.c_str());
      lastResult = RESULT_FAIL;
    }
    
    return lastResult;
  }

  Animation* CannedAnimationContainer::GetAnimation(const std::string& name)
  {
    Animation* animPtr = nullptr;
    
    auto retVal = _animations.find(name);
    if(retVal == _animations.end()) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.GetKeyFrameList.InvalidName",
                        "Animation requested for unknown animation '%s'.\n",
                        name.c_str());
    } else {
      animPtr = &retVal->second.second;
    }
    
    return animPtr;
  }

  AnimationID_t CannedAnimationContainer::GetID(const std::string& name) const
  {
    AnimationID_t animID = INVALID_ANIMATION_ID;
    
    auto retVal = _animations.find(name);
    if(retVal == _animations.end()) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.GetID.InvalidName",
                        "ID requested for unknown animation '%s'.\n",
                        name.c_str());
    } else {
      animID = retVal->second.first;
    }
    
    return animID;
  }
  

  /*
  u16 CannedAnimationContainer::GetLengthInMilliSeconds(const std::string& name) const
  {
    auto result = _animations.find(name);
    if(result == _animations.end()) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.GetLengthInMilliSeconds.InvalidName",
                        "Length requested for unknown animation '%s'.\n",
                        name.c_str());
      return 0;
    } else {
      u16 length = 0;
      
      const KeyFrameList& anim = result->second.second;
      
    }
  } // GetLengthInMilliSeconds()
   */
  
  /*
  Result CannedAnimationContainer::Send(RobotID_t robotID, IRobotMessageHandler* msgHandler)
  {
    Result lastResult = RESULT_OK;
    
    PRINT_STREAM_INFO("CannedAnimationContainer.Send",
                      "Sending " << _animations.size() << " animations to robot " << robotID << ".");
    
    for(auto & cannedAnimationByName : _animations)
    {
      const std::string&  name         = cannedAnimationByName.first;
      const s32           animID       = cannedAnimationByName.second.first;
      const Animation&    keyFrameList = cannedAnimationByName.second.second;
      
      if(keyFrameList.IsEmpty()) {
        PRINT_NAMED_WARNING("CannedAnimationContainer.Send.EmptyAnimation",
                            "Refusing to send empty '%s' animation with ID=%d.\n",
                            name.c_str(), animID);
      } else if (animID < 0) {
        PRINT_NAMED_WARNING("CannedAnimationContainer.Send.ZeroID",
                            "Refusing to send '%s' animation with invalid ID=-1.\n",
                            name.c_str());
      } else {
        
        // First send a clear command for this animation ID, before populating
        // it with keyframes
        MessageClearCannedAnimation clearMsg;
        clearMsg.animationID = static_cast<AnimationID_t>(animID);
        if(msgHandler != nullptr) { // could be in a test environment
          msgHandler->SendMessage(robotID, clearMsg);
        }
        
        PRINT_NAMED_INFO("CannedAnimationContainer.Send",
                         "Sending '%s' animation definition with ID=%d.\n",
                         name.c_str(), animID);

        // Now send all the keyframe definition messages for this animation ID
        for(auto message : keyFrameList.GetMessages())
        {
          if(message != nullptr) {
            if(msgHandler != nullptr) { // could be in a test environment
              lastResult = msgHandler->SendMessage(robotID, *message);
            }
            if(lastResult != RESULT_OK) {
              PRINT_NAMED_ERROR("CannedAnimationContainer.Send.SendMessageFail",
                                "Failed to send a keyframe message.\n");
              return lastResult;
            }
          } else {
            PRINT_NAMED_WARNING("CannedAnimationContainer.Send.NullMessagePtr",
                                "Robot %d encountered NULL message sending canned animations.\n", robotID);
          }
        } // for each message
        
      } // if/else ladder for keyFrameList validity
      
    } // for each canned animation
    
    return lastResult;
    
  } // SendCannedAnimations()
  */
  
  /*
  static EyeShape GetEyeShape(const Json::Value& json)
  {
    static const std::map<std::string, EyeShape> LUT = {
      {"CURRENT_SHAPE",   EYE_CURRENT_SHAPE},
      {"OPEN",            EYE_OPEN},
      {"HALF",            EYE_HALF},
      {"SLIT",            EYE_SLIT},
      {"CLOSED",          EYE_CLOSED},
      {"OFF_PUPIL_UP",    EYE_OFF_PUPIL_UP},
      {"OFF_PUPIL_DOWN",  EYE_OFF_PUPIL_DOWN},
      {"OFF_PUPIL_LEFT",  EYE_OFF_PUPIL_LEFT},
      {"OFF_PUPIL_RIGHT", EYE_OFF_PUPIL_RIGHT},
      {"ON_PUPIL_UP",     EYE_ON_PUPIL_UP},
      {"ON_PUPIL_DOWN",   EYE_ON_PUPIL_DOWN},
      {"ON_PUPIL_LEFT",   EYE_ON_PUPIL_LEFT},
      {"ON_PUPIL_RIGHT",  EYE_ON_PUPIL_RIGHT}
    };
    
    const EyeShape DEFAULT = EYE_CURRENT_SHAPE;
    
    if(json.isString()) {
      auto result = LUT.find(json.asString());
      if(result == LUT.end()) {
        PRINT_NAMED_WARNING("GetEyeShape", "Unknown eye shape '%s', returning default.\n",
                            json.asString().c_str());
        return DEFAULT;
      } else {
        return result->second;
      }
    } else if(json.isIntegral()) {
      return static_cast<EyeShape>(json.asInt());
    } else {
      PRINT_NAMED_WARNING("GetEyeShape",
                          "Not sure how to convert stored eye shape, returning default.\n");
      return DEFAULT;
    }
  } // GetEyeShape()
  
  
  static WhichEye GetWhichEye(const Json::Value& json)
  {
    static const std::map<std::string, WhichEye> LUT = {
      {"BOTH",   EYE_BOTH},
      {"LEFT",   EYE_LEFT},
      {"RIGHT",  EYE_RIGHT}
    };
    
    const WhichEye DEFAULT = EYE_BOTH;
    
    if(json.isString()) {
      auto result = LUT.find(json.asString());
      if(result == LUT.end()) {
        PRINT_NAMED_WARNING("GetWhichEye", "Unknown eye '%s', returning default.\n",
                            json.asString().c_str());
        return DEFAULT;
      } else {
        return result->second;
      }
    } else if(json.isIntegral()) {
      return static_cast<WhichEye>(json.asInt());
    } else {
      PRINT_NAMED_WARNING("GetWhichEye",
                          "Not sure how to convert stored eye, returning default.\n");
      return DEFAULT;
    }
  } // GetEyeShape()
  
  static f32 GetHeight(const Json::Value& json)
  {
    static const std::map<std::string, f32> LUT = {
      {"LOW_DOCK",  LIFT_HEIGHT_LOWDOCK},
      {"HIGH_DOCK", LIFT_HEIGHT_HIGHDOCK},
      {"CARRY",     LIFT_HEIGHT_CARRY},
    };
    
    const f32 DEFAULT = LIFT_HEIGHT_LOWDOCK;
    
    if(json.isString()) {
      auto result = LUT.find(json.asString());
      if(result == LUT.end()) {
        PRINT_STREAM_WARNING("GetHeight", "Unknown height " << json.toStyledString() << ", returning default.\n");
        return DEFAULT;
      } else {
        return result->second;
      }
    } else if(json.isNumeric()) {
      return json.asFloat();
    } else {
      PRINT_NAMED_WARNING("GetHeight",
                          "Encountered unknown height, returning default.\n");
      return DEFAULT;
    }
  } // GetHeight()

  static void GetColor(Json::Value& json)
  {
    if(json.isString()) {
      Json::Value temp; // can't seem to turn input json into array directly
      const ColorRGBA& color( NamedColors::GetByString(json.asString()));
      temp[0] = color.r();
      temp[1] = color.g();
      temp[2] = color.b();
      json = temp;
    } else if(!json.isArray()) {
      PRINT_NAMED_WARNING("GetColor", "Not sure how to translate color, not changing.\n");
    }
    
  } // GetColor()
  */
  
  Result CannedAnimationContainer::DefineFromJson(Json::Value& jsonRoot)
  {
    
    Json::Value::Members animationNames = jsonRoot.getMemberNames();
    
    /*
    // Add _all_ the animations first to register the IDs, so Trigger keyframes
    // which specify another animation's name will still work (because that
    // name should already exist, no matter the order the Json is parsed)
    for(auto const& animationName : animationNames) {
      AddAnimation(animationName);
    }
     */
    
    for(auto const& animationName : animationNames)
    {
      if(RESULT_OK != AddAnimation(animationName)) {
        PRINT_NAMED_INFO("CannedAnimationContainer.DefineFromJson.ReplaceName",
                          "Replacing existing animation named '%s'.\n",
                          animationName.c_str());
      }
      
      Animation* animation = GetAnimation(animationName);
      if(animation == nullptr) {
        PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson",
                          "Could not GetAnimation named '%s'.",
                          animationName.c_str());
        return RESULT_FAIL;
      }
      
      Result lastResult = animation->DefineFromJson(jsonRoot[animationName]);
      
      // Sanity check
      if(animation->GetName() != animationName) {
        PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson",
                          "Animation's internal name ('%s') doesn't match container's name for it ('%s').\n",
                          animation->GetName().c_str(),
                          animationName.c_str());
        return RESULT_FAIL;
      }
      
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson",
                          "Failed to define animation '%s' from Json.\n",
                          animationName.c_str());
        return lastResult;
      }
      
      /*
      const s32 animID = GetID(animationName);
      if(animID < 0) {
        return RESULT_FAIL;
      }
      
      const s32 numFrames = jsonRoot[animationName].size();
      for(s32 iFrame = 0; iFrame < numFrames; ++iFrame)
      {
        Json::Value& jsonFrame = jsonRoot[animationName][iFrame];
        
        // Semi-hack to make sure each message has the animatino ID in it.
        // We do this here since the CreateFromJson() call below returns a
        // pointer to a base-class message (without accessible "animationID"
        // field).
        jsonFrame["animationID"] = animID;
        
        if(!jsonFrame.isMember("transitionIn")) {
          PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson.NoTransitionIn",
                            "Missing 'transitionIn' field for '%s' frame of '%s' animation.\n",
                            jsonFrame["Name"].asString().c_str(),
                            animationName.c_str());
          return RESULT_FAIL;
        }
        
        if(!jsonFrame.isMember("transitionOut")) {
          PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson.NoTransitionOut",
                            "Missing 'transitionOut' field for '%s' frame of '%s' animation.\n",
                            jsonFrame["Name"].asString().c_str(),
                            animationName.c_str());
          return RESULT_FAIL;
        }
        
        if(jsonFrame.isMember("animToPlay")) {
          if(!jsonFrame["animToPlay"].isString()) {
            PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson.animToPlayString",
                              "Expecting 'animToPlay' field for '%s' frame of '%s' animation to be a string.\n",
                              jsonFrame["Name"].asString().c_str(),
                              animationName.c_str());
          } else {
            jsonFrame["animToPlay"] = GetID(jsonFrame["animToPlay"].asString());
          }
        }
        
        // Convert sound name (if specified) to SoundID (robot doesn't use strings):
        if(jsonFrame.isMember("soundID")) {
          jsonFrame["soundID"] = SoundManager::GetID(jsonFrame["soundID"].asString());
        }
        
        // Convert eye shape/whichEye to enums:
        if(jsonFrame.isMember("whichEye")) {
          jsonFrame["whichEye"] = GetWhichEye(jsonFrame["whichEye"]);
        }
        if(jsonFrame.isMember("shape")) {
          jsonFrame["shape"] = GetEyeShape(jsonFrame["shape"]);
        }
        
        // Convert named lift heights to values
        if(jsonFrame.isMember("height_mm")) {
          jsonFrame["height_mm"]     = GetHeight(jsonFrame["height_mm"]);
        } else if(jsonFrame.isMember("lowHeight_mm")) {
          jsonFrame["lowHeight_mm"]  = GetHeight(jsonFrame["lowHeight_mm"]);
        } else if(jsonFrame.isMember("highHeight_mm")) {
          jsonFrame["highHeight_mm"] = GetHeight(jsonFrame["highHeight_mm"]);
        }
        
        // Convert color strings if necessary (this modifies the json value directly)
        if(jsonFrame.isMember("color")) {
          GetColor(jsonFrame["color"]);
        }
        
        RobotMessage* kfMessage = RobotMessage::CreateFromJson(jsonRoot[animationName][iFrame]); 
        if(kfMessage != nullptr) {
          KeyFrameList* keyFrames = GetKeyFrameList(animationName);
          if(keyFrames == nullptr) {
            return RESULT_FAIL;
          }
          keyFrames->AddKeyFrame(kfMessage);
        }
        
      } // for each frame
       */
    } // for each animation
    
    return RESULT_OK;
  } // CannedAnimationContainer::DefineFromJson()
  
  Result CannedAnimationContainer::DefineHardCoded()
  {
    /*
    //
    // SLOW HEAD NOD - 2 slow nods
    //
    {
      AddAnimation("ANIM_HEAD_NOD_SLOW");
      
      const s32 animID = GetID("ANIM_HEAD_NOD_SLOW");
      assert(animID >= 0);
      
      KeyFrameList* keyFrames = GetKeyFrameList("ANIM_HEAD_NOD_SLOW");
      assert(keyFrames != nullptr);
      
      // Start the nod
      MessageAddAnimKeyFrame_StartHeadNod* startNodMsg = new MessageAddAnimKeyFrame_StartHeadNod();
      startNodMsg->animationID = animID;
      startNodMsg->relTime_ms = 0;
      startNodMsg->lowAngle_deg  = -25;
      startNodMsg->highAngle_deg =  25;
      startNodMsg->period_ms = 1200;
      keyFrames->AddKeyFrame(startNodMsg);
      
      // Stop the nod
      MessageAddAnimKeyFrame_StopHeadNod* stopNodMsg = new MessageAddAnimKeyFrame_StopHeadNod();
      stopNodMsg->animationID = animID;
      stopNodMsg->relTime_ms = 2400;
      stopNodMsg->finalAngle_deg = 0;
      keyFrames->AddKeyFrame(stopNodMsg);
    } // SLOW HEAD NOD
    */
    return RESULT_OK;
    
  } // DefineHardCodedAnimations()
  
  void CannedAnimationContainer::Clear()
  {
    _animations.clear();
  } // Clear()

} // namespace Cozmo
} // namespace Anki