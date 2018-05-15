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


#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/colorRGBA.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/shared/compositeImage/compositeImage.h"
#include "cannedAnimLib/baseTypes/cozmo_anim_generated.h"
#include "cannedAnimLib/baseTypes/keyframe.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/helpers/boundedWhile.h"
#include "util/helpers/quoteMacro.h"
#include "util/logging/logging.h"

#include <opencv2/core.hpp>
#include <cassert>


bool has_any_digits(const std::string& s)
{
  return std::any_of(s.begin(), s.end(), ::isdigit);
}


#define CREATE_STREAM_MSG(msgName, sourceMsg)

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
    
    bool IKeyFrame::IsTimeToPlay(TimeStamp_t timeSinceAnimStart_ms) const
    {
      return GetTriggerTime_ms() <= timeSinceAnimStart_ms;
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
    
    bool IKeyFrame::IsDoneHelper(const TimeStamp_t timeSinceAnimStart_ms, TimeStamp_t duration_ms) const
    {
      return GetTimeSinceTrigger(timeSinceAnimStart_ms) >= duration_ms;
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
#if ANKI_DEV_CHEATS
    std::stringstream debugStr;
    debugStr << "cast of " << fromVal << " would be invalid, clamping to " << toVal;
    PRINT_NAMED_WARNING("IKeyFrame.SafeNumericCast.InvalidCast",
                        "%s: %s",
                        debugName,
                        debugStr.str().c_str());
#endif
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
     : _angle_deg(angle_deg)
     , _angleVariability_deg(angle_variability_deg)
     {
       _keyframeDuration_ms = duration_ms;
     }
    
    #if CAN_STREAM
      RobotInterface::EngineToRobot* HeadAngleKeyFrame::GetStreamMessage(const TimeStamp_t timeSinceAnimStart_ms) const
      {
        if (!IsFirstKeyframeTick(timeSinceAnimStart_ms)) {
          return nullptr;
        }
        
        RobotInterface::SetHeadAngle streamHeadMsg;
        
        streamHeadMsg.actionID = 0;
        streamHeadMsg.duration_sec = 0.001 * _motionDuration_ms;
        
        // Add variability:
        if(_angleVariability_deg > 0) {
          streamHeadMsg.angle_rad = DEG_TO_RAD(static_cast<s8>(GetRNG().RandIntInRange(_angle_deg - _angleVariability_deg,
                                                                                        _angle_deg + _angleVariability_deg)));
        } else {
          streamHeadMsg.angle_rad = DEG_TO_RAD(_angle_deg);
        }
        
        return new RobotInterface::EngineToRobot(streamHeadMsg);
      }

    #endif

    Result HeadAngleKeyFrame::DefineFromFlatBuf(const CozmoAnim::HeadAngle* headAngleKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(headAngleKeyframe != nullptr, "HeadAngleKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(headAngleKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      Result lastResult = SetMembersFromFlatBuf(headAngleKeyframe, animNameDebug);
      return lastResult;
    }

    Result HeadAngleKeyFrame::SetMembersFromFlatBuf(const CozmoAnim::HeadAngle* headAngleKeyframe, const std::string& animNameDebug)
    {
      SafeNumericCast(headAngleKeyframe->durationTime_ms(),      _motionDuration_ms,    animNameDebug.c_str());
      SafeNumericCast(headAngleKeyframe->angle_deg(),            _angle_deg,            animNameDebug.c_str());
      SafeNumericCast(headAngleKeyframe->angleVariability_deg(), _angleVariability_deg, animNameDebug.c_str());
      _keyframeDuration_ms = _motionDuration_ms;
      return RESULT_OK;
    }
    
    Result HeadAngleKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, durationTime_ms, motionDuration_ms);
      GET_MEMBER_FROM_JSON(jsonRoot, angle_deg);
      GET_MEMBER_FROM_JSON(jsonRoot, angleVariability_deg);
      _keyframeDuration_ms = _motionDuration_ms;
      return RESULT_OK;
    }
    
#pragma mark -
#pragma mark LiftHeightKeyFrame
    //
    // LiftHeightKeyFrame
    //

    LiftHeightKeyFrame::LiftHeightKeyFrame(u8 height_mm, u8 heightVariability_mm, TimeStamp_t duration_ms)
    : _height_mm(height_mm)
    , _heightVariability_mm(heightVariability_mm)
    {
      _keyframeDuration_ms = duration_ms;
    }
    
    #if CAN_STREAM
      RobotInterface::EngineToRobot* LiftHeightKeyFrame::GetStreamMessage(const TimeStamp_t timeSinceAnimStart_ms) const
      {
        if (!IsFirstKeyframeTick(timeSinceAnimStart_ms)) {
          return nullptr;
        }
        RobotInterface::SetLiftHeight streamLiftMsg;
        
        streamLiftMsg.actionID = 0;
        streamLiftMsg.duration_sec = 0.001 * _motionDuration_ms;
        
        // Add variability:
        if(_heightVariability_mm > 0) {
          streamLiftMsg.height_mm = (uint8_t)static_cast<s8>(GetRNG().RandIntInRange(_height_mm - _heightVariability_mm,
                                                                                     _height_mm + _heightVariability_mm));
        } else {
          streamLiftMsg.height_mm = _height_mm;
        }

        return new RobotInterface::EngineToRobot(streamLiftMsg);
      }
    #endif

    Result LiftHeightKeyFrame::DefineFromFlatBuf(const CozmoAnim::LiftHeight* liftHeightKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(liftHeightKeyframe != nullptr, "LiftHeightKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(liftHeightKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      Result lastResult = SetMembersFromFlatBuf(liftHeightKeyframe, animNameDebug);
      return lastResult;
    }

    Result LiftHeightKeyFrame::SetMembersFromFlatBuf(const CozmoAnim::LiftHeight* liftHeightKeyframe, const std::string& animNameDebug)
    {
      SafeNumericCast(liftHeightKeyframe->durationTime_ms(),      _motionDuration_ms,    animNameDebug.c_str());
      SafeNumericCast(liftHeightKeyframe->height_mm(),            _height_mm,            animNameDebug.c_str());
      SafeNumericCast(liftHeightKeyframe->heightVariability_mm(), _heightVariability_mm, animNameDebug.c_str());
      _keyframeDuration_ms = _motionDuration_ms;
         
      return RESULT_OK;
    }
    
    Result LiftHeightKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, durationTime_ms, motionDuration_ms);
      GET_MEMBER_FROM_JSON(jsonRoot, height_mm);
      GET_MEMBER_FROM_JSON(jsonRoot, heightVariability_mm);
      _keyframeDuration_ms = _motionDuration_ms;
      return RESULT_OK;
    }
    
#pragma mark -
#pragma mark SpriteSequenceKeyFrame

    SpriteSequenceKeyFrame::SpriteSequenceKeyFrame(bool shouldRenderInEyeHue,
                                                   Vision::SpriteName sequenceName,
                                                   bool containsRuntimeSpriteSeq)
    : _shouldRenderInEyeHue(shouldRenderInEyeHue)
    , _spriteSequenceName(sequenceName) 
    {
      _keyframeDuration_ms = ANIM_TIME_STEP_MS;
      if(containsRuntimeSpriteSeq){
        _runtimeSpriteSequence = std::make_unique<Vision::SpriteSequence>();
      }
    }

    SpriteSequenceKeyFrame::SpriteSequenceKeyFrame(const SpriteSequenceKeyFrame& other)
    {
      _triggerTime_ms            = other._triggerTime_ms;
      _keyframeDuration_ms       = other._keyframeDuration_ms;
      _shouldRenderInEyeHue      = other._shouldRenderInEyeHue;
      _cannedSpriteSequence      = other._cannedSpriteSequence;
      _rawSeqName                = other._rawSeqName;
      _spriteSequenceName        = other._spriteSequenceName;
      _scanlineOpacity           = other._scanlineOpacity;
      _internalUpdateInterval_ms = other._internalUpdateInterval_ms;
      
      if(other._runtimeSpriteSequence != nullptr){
        _runtimeSpriteSequence.reset(new Vision::SpriteSequence(*other._runtimeSpriteSequence));
      }

      if(other._compositeImage != nullptr){
        _compositeImage.reset(new Vision::CompositeImage(*other._compositeImage));
      }
      
    }

    SpriteSequenceKeyFrame::~SpriteSequenceKeyFrame()
    {

    }
    
    Result SpriteSequenceKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      {
        std::string strSeqName = JsonTools::ParseString(jsonRoot, "animName", "SpriteSequenceKeyframe.MissingName");
        // TODO: Take this out once root path is part of AnimationTool!
        size_t lastSlash = strSeqName.find_last_of("/");
        if(lastSlash != std::string::npos) {
          PRINT_NAMED_WARNING("SpriteSequenceKeyFrame.Process",
                              "%s: Removing path from animation name: %s",
                              animNameDebug.c_str(),
                              strSeqName.c_str());
          strSeqName = strSeqName.substr(lastSlash+1, std::string::npos);
        }
        _rawSeqName = strSeqName;
      }

      JsonTools::GetValueOptional(jsonRoot, "scanlineOpacity", _scanlineOpacity);
      JsonTools::GetValueOptional(jsonRoot, "frameDuration_ms", _internalUpdateInterval_ms);

      return Process(animNameDebug);
    }

    Result SpriteSequenceKeyFrame::Process(const std::string& animNameDebug)
    {
      // Verify that the scanline opacity is between 0 and 1
      DEV_ASSERT_MSG(Util::InRange(_scanlineOpacity, 0.f, 1.f),
                     "SpriteSequenceKeyFrame.Process.InvalidScanlineOpacity",
                     "%s: Invalid scanline opacity of %f",
                     animNameDebug.c_str(),
                     _scanlineOpacity);
      _scanlineOpacity = Util::Clamp(_scanlineOpacity, 0.f, 1.f);

      return RESULT_OK;
    }

    
    void SpriteSequenceKeyFrame::GiveKeyframeImageAccess(const Vision::SpritePathMap* spriteMap,
                                                         Vision::SpriteSequenceContainer* spriteSequenceContainer) 
    {
      DEV_ASSERT((spriteMap != nullptr) && (_runtimeSpriteSequence == nullptr), 
                 "SpriteSequenceKeyFram.GiveKeyframeImageAccess.InvalidSpriteMap");

      if(ParseSequenceNameFromString(spriteMap)){
        _cannedSpriteSequence = spriteSequenceContainer->GetSequenceByName(_spriteSequenceName);
      }else{
        PRINT_NAMED_WARNING("SpriteSequenceKeyFram.GiveKeyframeImageAccess.NoMatchFoundForSequenceName",
                            "Sequence name %s does not appear in spriteMap, attempting to grab from unnamed map",
                            _rawSeqName.c_str());
        _cannedSpriteSequence = spriteSequenceContainer->GetUnmappedSequenceByFileName(_rawSeqName);
        ANKI_VERIFY(_cannedSpriteSequence != nullptr,
                    "SpriteSequenceKeyFram.GiveKeyframeImageAccess.Failed to fild unmaped sequence",
                    "Missing sequence for %s entirely",
                    _rawSeqName.c_str());
      }
    }

    bool SpriteSequenceKeyFrame::VerifyOnlyOneImplementationSet() const
    {
      const bool oneSequenceNull = (_cannedSpriteSequence == nullptr) || (_runtimeSpriteSequence == nullptr);
      const bool oneRuntimeNull  = (_runtimeSpriteSequence == nullptr) || (_compositeImage == nullptr);
      const bool oneOtherNull    = (_cannedSpriteSequence == nullptr) || (_compositeImage == nullptr);
      return oneSequenceNull && oneRuntimeNull && oneOtherNull;
    }
    
    TimeStamp_t SpriteSequenceKeyFrame::CalculateInternalEndTime_ms() const
    {
      TimeStamp_t outTime = 0;
      if(_cannedSpriteSequence != nullptr){
        outTime = _cannedSpriteSequence->GetNumFrames() * _internalUpdateInterval_ms;
      }else if(_runtimeSpriteSequence != nullptr){
        outTime = _runtimeSpriteSequence->GetNumFrames() * _internalUpdateInterval_ms;
      }else if(_compositeImage != nullptr){
        outTime = _compositeImage->GetFullLoopLength() * _internalUpdateInterval_ms;
      }
      return outTime += _triggerTime_ms;
    }
    
    bool SpriteSequenceKeyFrame::HaveKeyframeForTimeStamp(const TimeStamp_t timeSinceAnimStart_ms) const
    {
      return (timeSinceAnimStart_ms < CalculateInternalEndTime_ms()) &&
             ((timeSinceAnimStart_ms % _internalUpdateInterval_ms) <= ANIM_TIME_STEP_MS);
    }

    
    bool SpriteSequenceKeyFrame::ParseSequenceNameFromString(const Vision::SpritePathMap* spriteMap)
    {
      // _rawSeqName is only the folder name - manually check all entries in sprite map
      // so that just the folder name is pulled out of the full path to try and find a match
      bool foundMatch = false;
      for(const auto& key : spriteMap->GetAllKeys()){
        const auto& fullPath = spriteMap->GetValue(key);
        const auto& fileName = Util::FileUtils::GetFileName(fullPath);
        if(fileName == _rawSeqName){
          foundMatch = true;
          _spriteSequenceName = key;
          break;
        }
      }

      if(foundMatch){
        const bool isValidSequence = Vision::IsSpriteSequence(_spriteSequenceName, false);
        ANKI_VERIFY(isValidSequence,
                    "SpriteSequenceKeyFrame.SetMembersFromJson.InvalidSequence",
                    "Sprite %s is not marked as a sprite sequence",
                    SpriteNameToString(_spriteSequenceName));
      }

      return foundMatch;
    }

    void SpriteSequenceKeyFrame::ApplyCompositeImageUpdate(CompositeImageUpdateSpec&& updateSpec)
    {
      auto& compImg = GetCompositeImage();
      auto* layer = compImg.GetLayerByName(updateSpec.layerName);
      if(layer != nullptr){
        // clear the whole layer if no sprite box name specified
        if(updateSpec.spriteBox.spriteBoxName == Vision::SpriteBoxName::Count){
          compImg.ClearLayerByName(updateSpec.layerName);
          PRINT_NAMED_INFO("AnimationStreamer.UpdateCompositeImage.ClearingLayer", 
                           "Layer %s cleared from image because spriteBox with count value received",
                           LayerNameToString(updateSpec.layerName));
        }else{
          layer->AddToLayout(updateSpec.spriteBox.spriteBoxName, updateSpec.spriteBox);
          layer->AddToImageMap(updateSpec.spriteCache, updateSpec.seqContainer,
                               updateSpec.spriteBox.spriteBoxName, updateSpec.spriteName);
        }
      }else{
        Vision::CompositeImageLayer layer(updateSpec.layerName);
        layer.AddToLayout(updateSpec.spriteBox.spriteBoxName, updateSpec.spriteBox);
        Vision::CompositeImageLayer::SpriteEntry entry(updateSpec.spriteCache, updateSpec.seqContainer, updateSpec.spriteName);
        layer.AddToImageMap(updateSpec.spriteBox.spriteBoxName, entry);
        compImg.AddLayer(std::move(layer));
        PRINT_NAMED_INFO("AnimationStreamer.UpdateCompositeImage.AddingLayer",
                         "Layer %s added to composite image",
                         LayerNameToString(updateSpec.layerName));
      }
    }


    Result SpriteSequenceKeyFrame::DefineFromFlatBuf(const CozmoAnim::FaceAnimation* faceAnimKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(faceAnimKeyframe != nullptr, "SpriteSequenceKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(faceAnimKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      Result lastResult = SetMembersFromFlatBuf(faceAnimKeyframe, animNameDebug);
      return lastResult;
    }

    Result SpriteSequenceKeyFrame::SetMembersFromFlatBuf(const CozmoAnim::FaceAnimation* faceAnimKeyframe, const std::string& animNameDebug)
    {
      _rawSeqName = faceAnimKeyframe->animName()->str();
      SafeNumericCast(faceAnimKeyframe->scanlineOpacity(), _scanlineOpacity, animNameDebug.c_str());

      return Process(animNameDebug);
    }
    
    bool SpriteSequenceKeyFrame::IsDone(const TimeStamp_t timeSinceAnimStart_ms) const
    {
      if(!SequenceShouldAdvance() || (timeSinceAnimStart_ms < CalculateInternalEndTime_ms())){
        return false;
      }

      // For canned animations we check if GetFaceImage() has been called as many
      // times as there are frames.
      // For procedural animations, frames are deleted (with PopFront) after they are
      // played in order to avoid unbounded growth (since a procedural animation can
      // be played for an arbitrary period of time) so we check for whether or not
      // there are any frames left.
      if(ANKI_DEV_CHEATS){
        ANKI_VERIFY(VerifyOnlyOneImplementationSet(), 
                    "SpriteSequenceKeyFrame.IsDone.MultilpeImplementationsSet", 
                    "");
      }

      if(_runtimeSpriteSequence != nullptr){
        return _runtimeSpriteSequence->GetNumFrames() == 0;
      }else if(_cannedSpriteSequence != nullptr){
        return _cannedSpriteSequence->GetNumFrames() <= GetFrameNumberForTime(timeSinceAnimStart_ms);
      }else if(_compositeImage != nullptr){
        return _compositeImage->GetFullLoopLength() <= GetFrameNumberForTime(timeSinceAnimStart_ms);
      }
      return true;
    }
    
    bool SpriteSequenceKeyFrame::ShouldRenderInEyeHue() const
    {
      // Composite images handle their own eye coloring
      return (_compositeImage == nullptr) && _shouldRenderInEyeHue;
    }


    void SpriteSequenceKeyFrame::AddFrameToRuntimeSequence(Vision::SpriteHandle spriteHandle)
    {
      if(_runtimeSpriteSequence != nullptr){
        _runtimeSpriteSequence->AddFrame(spriteHandle);
      }
    }

    void SpriteSequenceKeyFrame::SetCompositeImage(Vision::SpriteCache* spriteCache, 
                                                   Vision::CompositeImage* compImg, 
                                                   u32 getFrameInterval_ms)
    {
      Vision::HSImageHandle faceHueAndSaturation = ProceduralFace::GetHueSatWrapper();
      _compositeImage = std::make_unique<Vision::CompositeImage>(spriteCache, faceHueAndSaturation);
      _compositeImage.reset(compImg);
      _internalUpdateInterval_ms = getFrameInterval_ms;
      ANKI_VERIFY((_internalUpdateInterval_ms != 0) &&
                  ((_internalUpdateInterval_ms % ANIM_TIME_STEP_MS) == 0),
                  "SpriteSequenceKeyFrame.SetCompositeImage.InvalidTimeStep",
                  "Update interval %d is not a multiple of anim time step %d",
                  _internalUpdateInterval_ms, ANIM_TIME_STEP_MS);
    }

    void SpriteSequenceKeyFrame::QueueCompositeImageUpdate(CompositeImageUpdateSpec&& updateSpec,
                                                           u32 applyAt_ms)
    {
      _compositeImageUpdateMap.emplace(applyAt_ms, std::move(updateSpec));
      _compositeImageUpdated = true;
    }


    bool SpriteSequenceKeyFrame::NewImageContentAvailable(const TimeStamp_t timeSinceAnimStart_ms) const
    {
      if(IsFirstKeyframeTick(timeSinceAnimStart_ms)){
        return true;
      }
      
      if(_cannedSpriteSequence != nullptr){
        return HaveKeyframeForTimeStamp(timeSinceAnimStart_ms);
      }else if(_runtimeSpriteSequence != nullptr){
        return _runtimeSpriteSequence->GetNumFrames() > 1;
      }else if(_compositeImage != nullptr){
        const bool timeToAdvanceFrame = (_compositeImage->GetFullLoopLength() > 1) && 
                                        ((timeSinceAnimStart_ms % _internalUpdateInterval_ms) == 0);
        const bool updatesForCurrentFrame = !_compositeImageUpdateMap.empty() &&
                                            (_compositeImageUpdateMap.begin()->first <= timeSinceAnimStart_ms);
        return _compositeImageUpdated ||timeToAdvanceFrame || updatesForCurrentFrame;
      }
      
      return false;
    }

    
    bool SpriteSequenceKeyFrame::GetFaceImageHandle(const TimeStamp_t timeSinceAnimStart_ms, Vision::SpriteHandle& handle)
    {
      if(IsDone(timeSinceAnimStart_ms)) {
        return false;
      }

      if(ANKI_DEV_CHEATS){
        ANKI_VERIFY(VerifyOnlyOneImplementationSet(), 
                    "SpriteSequenceKeyFrame.GetFaceImageHandle.MultilpeImplementationsSet", 
                    "");
      }

      if(_compositeImage != nullptr){
        // Apply any composite image updates queued
        auto iter = _compositeImageUpdateMap.begin();
        const auto mapBound = _compositeImageUpdateMap.size() + 1;
        BOUNDED_WHILE(mapBound, iter != _compositeImageUpdateMap.end()){
          if(iter->first <= timeSinceAnimStart_ms){
            auto updateSpec = iter->second;
            ApplyCompositeImageUpdate(std::move(updateSpec));
            // erase element/move iterator
            _compositeImageUpdateMap.erase(iter);
            iter = _compositeImageUpdateMap.begin();
            _compositeImageUpdated = true;
          }else{
            break;
          }
        }
      }

      
      u32 curFrame = GetFrameNumberForTime(timeSinceAnimStart_ms);

      if((HaveKeyframeForTimeStamp(timeSinceAnimStart_ms)) ||
         _compositeImageUpdated){
        if(_runtimeSpriteSequence != nullptr){
          _runtimeSpriteSequence->GetFrame(0, handle);
          if(SequenceShouldAdvance()){
            _runtimeSpriteSequence->PopFront();
          }
        }else if(_cannedSpriteSequence != nullptr){
          _cannedSpriteSequence->GetFrame(curFrame, handle);
        }else if(_compositeImage != nullptr){
          auto* img = new Vision::ImageRGBA(_compositeImage->GetHeight(),
                                            _compositeImage->GetWidth());
          img->FillWith(Vision::PixelRGBA());
          _compositeImage->OverlayImageWithFrame(*img, curFrame);
          handle = std::make_shared<Vision::SpriteWrapper>(img);
        }

        _compositeImageUpdated = false;
        return true;
      }else{
        return false;
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
      return RESULT_OK;
    }

    Result ProceduralFaceKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      _procFace.SetFromJson(jsonRoot);
      return RESULT_OK;
    }


    ProceduralFace ProceduralFaceKeyFrame::GetInterpolatedFace(const ProceduralFaceKeyFrame& nextFrame, const TimeStamp_t currentTime_ms)
    {
      // The interpolation fraction is how far along in time we are from this frame's
      // trigger time (which currentTime was initialized to) and the next frame's
      // trigger time.
      const f32 fraction = std::min(1.f, static_cast<f32>(currentTime_ms - GetTriggerTime_ms()) / static_cast<f32>(nextFrame.GetTriggerTime_ms() - GetTriggerTime_ms()));
      
      ProceduralFace interpFace;
      interpFace.Interpolate(_procFace, nextFrame._procFace, fraction);
      
      return interpFace;
    }
    
#pragma mark -
#pragma mark RobotAudioKeyFrame
    
    //
    // RobotAudioKeyFrame
    //

    Result RobotAudioKeyFrame::AddAudioRef(AudioKeyFrameType::AudioRef&& audioRef)
    {
      _audioReferences.push_back( std::move(audioRef) );
      return RESULT_OK;
    }

    Result RobotAudioKeyFrame::DefineFromFlatBuf(const CozmoAnim::RobotAudio* audioKeyframe, const std::string& animNameDebug)
    {
      DEV_ASSERT(audioKeyframe != nullptr, "RobotAudioKeyFrame.DefineFromFlatBuf.NullAnim");
      SafeNumericCast(audioKeyframe->triggerTime_ms(), _triggerTime_ms, animNameDebug.c_str());
      Result lastResult = SetMembersFromFlatBuf(audioKeyframe, animNameDebug);
      return lastResult;
    }
    
#define JSON_KEY( __KEY__ ) static const char* kKey_##__KEY__ = QUOTE(__KEY__)
    
    Result RobotAudioKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      using namespace AudioEngine;
      using namespace AudioKeyFrameType;
      using namespace AudioMetaData;
      
      // Check for deprecated format
      if ( jsonRoot.isMember("audioEventId") ) {
        // Deprecated format
        return SetMembersFromDeprecatedJson(jsonRoot, animNameDebug);
      }
      
      // Frame type list keys
      JSON_KEY(eventGroups);
      JSON_KEY(states);
      JSON_KEY(switches);
      JSON_KEY(parameters);
      
      const auto& states = jsonRoot[kKey_states];
      if (states.isArray()) {
        JSON_KEY(stateGroupId);
        JSON_KEY(stateId);
        for (auto stateIt = states.begin(); stateIt != states.end(); ++stateIt) {
          auto groupId = static_cast<u32>(GameState::StateGroupType::Invalid);
          auto stateId = static_cast<u32>(GameState::GenericState::Invalid);
          JsonTools::GetValueOptional(*stateIt, kKey_stateGroupId, groupId);
          JsonTools::GetValueOptional(*stateIt, kKey_stateId, stateId);
          if (((u32)GameState::StateGroupType::Invalid == groupId) || ((u32)GameState::GenericState::Invalid == stateId)) {
            PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromJson.InvalidGameState",
                              "'%s' @ %i ms : Has an invalid stateGroupId (%i) or stateId (%i)",
                              animNameDebug.c_str(), _triggerTime_ms, groupId, stateId);
            // Move to next state
            continue;
          }
          Result addResult = AddAudioRef(AudioRef(AudioStateRef(static_cast<GameState::StateGroupType>(groupId),
                                                                static_cast<GameState::GenericState>(stateId))));
          if(addResult != RESULT_OK) {
            return addResult;
          }
        }
      } // States
      
      const auto& switches = jsonRoot[kKey_switches];
      if (switches.isArray()) {
        JSON_KEY(switchGroupId);
        JSON_KEY(stateId);
        for (auto switchIt = switches.begin(); switchIt != switches.end(); ++switchIt) {
          auto groupId = static_cast<u32>(SwitchState::SwitchGroupType::Invalid);
          auto stateId = static_cast<u32>(SwitchState::GenericSwitch::Invalid);
          JsonTools::GetValueOptional(*switchIt, kKey_switchGroupId, groupId);
          JsonTools::GetValueOptional(*switchIt, kKey_stateId, stateId);
          if (((u32)SwitchState::SwitchGroupType::Invalid == groupId) ||
              ((u32)SwitchState::GenericSwitch::Invalid == stateId)) {
            PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromJson.InvalidSwitchState",
                              "'%s' @ %i ms : Has an invalid switchGroupId (%i) or stateId (%i)",
                              animNameDebug.c_str(), _triggerTime_ms, groupId, stateId);
            // Move to next switch
            continue;
          }
          Result addResult = AddAudioRef(AudioRef(AudioSwitchRef(static_cast<SwitchState::SwitchGroupType>(groupId),
                                                                 static_cast<SwitchState::GenericSwitch>(stateId))));
          if(addResult != RESULT_OK) {
            return addResult;
          }
        }
      } // Switches
      
      const auto& parameters = jsonRoot[kKey_parameters];
      if (parameters.isArray()) {
        JSON_KEY(parameterId);
        JSON_KEY(value);
        JSON_KEY(time_ms);
        JSON_KEY(curve);
        for (auto parameterIt = parameters.begin(); parameterIt != parameters.end(); ++parameterIt) {
          auto parameterId = static_cast<u32>(GameParameter::ParameterType::Invalid);
          float value = 0.0f;
          u32   time_ms = 0;
          u8    curve = static_cast<u8>(AudioEngine::Multiplexer::CurveType::Linear);
          JsonTools::GetValueOptional(*parameterIt, kKey_parameterId, parameterId);
          if ((u32)GameParameter::ParameterType::Invalid == parameterId) {
            PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromJson.InvalidParameter",
                              "'%s' @ %i ms : Has an invalid parameterId", animNameDebug.c_str(), _triggerTime_ms);
            // Move to next parameter
            continue;
          }
          JsonTools::GetValueOptional(*parameterIt, kKey_value, value);
          JsonTools::GetValueOptional(*parameterIt, kKey_time_ms, time_ms);
          JsonTools::GetValueOptional(*parameterIt, kKey_curve, curve);
          Result addResult = AddAudioRef(AudioRef(AudioParameterRef(static_cast<GameParameter::ParameterType>(parameterId),
                                                                    value,
                                                                    time_ms,
                                                                    static_cast<AudioEngine::Multiplexer::CurveType>(curve))));
          if(addResult != RESULT_OK) {
            return addResult;
          }
        }
      } // Parameters
      
      // Events need to be added last to the AudioRef list, they need to be posted last when performing a key frame
      const auto& eventGroups = jsonRoot[kKey_eventGroups];
      if (eventGroups.isArray()) {
        JSON_KEY(eventIds);
        JSON_KEY(volumes);
        JSON_KEY(probabilities);
        
        for (auto eventGroupIt = eventGroups.begin(); eventGroupIt != eventGroups.end(); ++eventGroupIt) {
          const auto& eventIds = (*eventGroupIt)[kKey_eventIds];
          const auto& volumes = (*eventGroupIt)[kKey_volumes];
          const auto& probabilities = (*eventGroupIt)[kKey_probabilities];
          if ((eventIds.size() != volumes.size()) || (eventIds.size() != probabilities.size())) {
            PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromJson.InvlaidEventGroup",
                              "'%s' @ %i ms : EventIds, Volumes & Probabilities don't have the same count",
                              animNameDebug.c_str(), _triggerTime_ms);
            // Move to next Event Group
            continue;
          }
          // Check sum of all probabilities to ensure it is <= 1.0
          f32 totalProb = 0.0f;
          for (auto probIt = probabilities.begin(); probIt != probabilities.end(); ++probIt) {
            totalProb += probIt->asFloat();
            if (totalProb > 1.0f) {
              PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromJson.TotalProbabilitiesTooHigh",
                                "'%s' @ %i ms : The total probability of all audio events combined exceeds 1.0",
                                animNameDebug.c_str(), _triggerTime_ms);
              return RESULT_FAIL;
            }
          }
          
          // Add events to group
          AudioEventGroupRef eventGroup;
          GameEvent::GenericEvent eventId;
          for (int idx = 0; idx < eventIds.size(); ++idx) {
            eventId = static_cast<GameEvent::GenericEvent>( eventIds[idx].asUInt() );
            if (GameEvent::GenericEvent::Invalid == eventId) {
              PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromJson.InvalidGameEvent",
                                "'%s' @ %i ms : Has an invalid audio event", animNameDebug.c_str(), _triggerTime_ms);
              // Move on to next event
              continue;
            }
            eventGroup.AddEvent(eventId, volumes[idx].asFloat(), probabilities[idx].asFloat());
          }
          
          if (eventGroup.Events.empty()) {
            PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromJson.InvalidGameEventGroup",
                              "'%s' @ %i ms : Has an empty event group", animNameDebug.c_str(), _triggerTime_ms);
            return  RESULT_FAIL;
          }
          Result addResult = AddAudioRef(AudioRef(std::move(eventGroup)));
          if (addResult != RESULT_OK) {
            return addResult;
          }
        }
      } // EventGroups
      return RESULT_OK;
    }
    
    Result RobotAudioKeyFrame::SetMembersFromDeprecatedJson(const Json::Value &jsonRoot,
                                                            const std::string& animNameDebug)
    {
      using namespace AudioKeyFrameType;
      using namespace AudioMetaData;
      
      JSON_KEY(audioEventId);
      JSON_KEY(volume);
      JSON_KEY(probability);
      
      // Get volume settings
      f32 volume = 1.0f;
      JsonTools::GetValueOptional(jsonRoot, kKey_volume, volume);
      f32 probability = 1.0f;
      
      const Json::Value& eventIds = jsonRoot[kKey_audioEventId];
      if (eventIds.isArray()) {
        std::vector<f32> probabilities;
        const bool probabilitiesSet = JsonTools::GetVectorOptional(jsonRoot, kKey_probability, probabilities);
        if (!probabilitiesSet) {
          const bool probabilitySet = JsonTools::GetValueOptional(jsonRoot, kKey_probability, probability);
          if (probabilitySet) {
            probabilities.push_back(probability);
          }
        }
        
        if (probabilities.empty() && !eventIds.empty()) {
          // If no probability is set, use equal probability for all audio events
          const f32 eachProbability = 1.0f / eventIds.size();
          for (int idx = 0; idx < eventIds.size(); idx++) {
            probabilities.push_back(eachProbability);
          }
        }
        else if (probabilities.size() != eventIds.size()) {
          // We must have the same number of probability values as audio events
          PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromDeprecatedJson.UnknownProbabilities",
                            "%s: The number of audio event IDs (%u) does not match number of probabilities (%zu)",
                            animNameDebug.c_str(), eventIds.size(), probabilities.size());
          return RESULT_FAIL;
        }
        
        // Check sum of all probabilities to ensure it is <= 1.0
        f32 totalProb = 0.0f;
        for (int probIdx = 0; probIdx < probabilities.size(); probIdx++) {
          totalProb += probabilities[probIdx];
          if(totalProb > 1.0) {
            PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromDeprecatedJson.TotalProbabilitiesTooHigh",
                              "%s: The total probability of all audio events combined exceeds 1.0",
                              animNameDebug.c_str());
            return RESULT_FAIL;
          }
        }
        
        AudioEventGroupRef eventGroup;
        for (int idx = 0; idx < eventIds.size(); ++idx) {
          probability = probabilities[idx];
          const auto eventId = static_cast<GameEvent::GenericEvent>( eventIds[idx].asUInt() );
          if (GameEvent::GenericEvent::Invalid == eventId) {
            PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromDeprecatedJson.InvalidGameEvent",
                              "'%s' @ %i ms : Has an invalid audio event", animNameDebug.c_str(), _triggerTime_ms);
            // Move on to next event
            continue;
          }
          eventGroup.AddEvent(eventId, volume, probability);
        }
        Result addResult = AddAudioRef( AudioRef( std::move(eventGroup) ) );
        if (addResult != RESULT_OK) {
          return addResult;
        }
      }
      else {
        JsonTools::GetValueOptional(jsonRoot, kKey_probability, probability);
        const auto eventId = static_cast<GameEvent::GenericEvent>( eventIds.asUInt() );
        if (GameEvent::GenericEvent::Invalid == eventId) {
          PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromDeprecatedJson.InvalidGameEvent",
                            "'%s' @ %i ms : Has an invalid audio event", animNameDebug.c_str(), _triggerTime_ms);
          return RESULT_FAIL;
        }
        AudioEventGroupRef eventGroup;
        eventGroup.AddEvent(eventId, volume, probability);
        Result addResult = AddAudioRef( AudioRef( std::move(eventGroup) ) );
        if(addResult != RESULT_OK) {
          return addResult;
        }
      }
      return RESULT_OK;
    }
    
    
    Result RobotAudioKeyFrame::SetMembersFromFlatBuf(const CozmoAnim::RobotAudio* audioKeyframe,
                                                    const std::string& animNameDebug)
    {
      using namespace AudioEngine;
      using namespace AudioKeyFrameType;
      using namespace AudioMetaData;
      
      const auto* states = audioKeyframe->states();
      if (nullptr != states) {
        for (const auto& aState : *states) {
          const auto groupId = static_cast<GameState::StateGroupType>(aState->stateGroupId());
          const auto stateId = static_cast<GameState::GenericState>(aState->stateId());
          if ((GameState::StateGroupType::Invalid == groupId) || (GameState::GenericState::Invalid == stateId)) {
            PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromFlatBuf.InvalidGameState",
                              "'%s' @ %i ms : Has an invalid stateGroupId (%i) or stateId (%i)",
                              animNameDebug.c_str(), _triggerTime_ms, groupId, stateId);
            // Move to next State
            continue;
          }
          Result addResult = AddAudioRef(AudioRef(AudioStateRef(groupId, stateId)));
          if(addResult != RESULT_OK) {
            return addResult;
          }
        }
      }
      
      const auto* switches = audioKeyframe->switches();
      if (nullptr != switches) {
        for (const auto& aSwitch : *switches) {
          const auto groupId = static_cast<SwitchState::SwitchGroupType>(aSwitch->switchGroupId());
          const auto stateId = static_cast<SwitchState::GenericSwitch>(aSwitch->stateId());
          if ((SwitchState::SwitchGroupType::Invalid == groupId) || (SwitchState::GenericSwitch::Invalid == stateId)) {
            PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromFlatBuf.InvalidSwitchState",
                              "'%s' @ %i ms : Has an invalid switchGroupId (%i) or stateId (%i)",
                              animNameDebug.c_str(), _triggerTime_ms, groupId, stateId);
            // Move to next Switch
            continue;
          }
          Result addResult = AddAudioRef(AudioRef(AudioSwitchRef(groupId, stateId)));
          if(addResult != RESULT_OK) {
            return addResult;
          }
        }
      }
      
      const auto* parameters = audioKeyframe->parameters();
      if (nullptr != parameters) {
        for (const auto& aParameter : *parameters) {
          const auto parameterId = static_cast<GameParameter::ParameterType>(aParameter->parameterId());
          if (GameParameter::ParameterType::Invalid == parameterId) {
            PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromFlatBuf.InvalidParameter",
                              "'%s' @ %i ms : Has an invalid parameterId", animNameDebug.c_str(), _triggerTime_ms);
            // Move to next Parameter
            continue;
          }
          auto parameterRef = AudioParameterRef(parameterId,
                                                aParameter->value(),
                                                aParameter->time_ms(),
                                                static_cast<Multiplexer::CurveType>(aParameter->curve()));
          Result addResult = AddAudioRef(AudioRef(std::move(parameterRef)));
          if(addResult != RESULT_OK) {
            return addResult;
          }
        }
      }
      // Events need to be added last to the AudioRef list, they need to be posted last when performing a key frame
      const auto* eventGroups = audioKeyframe->eventGroups();
      if (nullptr != eventGroups) {
        // Loop through groups
        for (const auto& aGroup : *eventGroups) {
          // Create event group
          AudioEventGroupRef anEventGroup;
          const auto* eventIds      = aGroup->eventIds();
          const auto* volumes       = aGroup->volumes();
          const auto* probabilities = aGroup->probabilities();
          
          if ((eventIds->size() != volumes->size()) || (eventIds->size() != probabilities->size())) {
            PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromFlatBuf.InvlaidEventGroup",
                              "'%s' @ %i ms : EventIds, Volumes & Probabilities don't have the same count",
                              animNameDebug.c_str(), _triggerTime_ms);
            // Move to next Event Group
            continue;
          }
          
          // Loop through events in group
          for (uint idx = 0; idx < eventIds->size();  ++idx) {
            const auto& anEventId = static_cast<GameEvent::GenericEvent>(eventIds->Get(idx));
            if (GameEvent::GenericEvent::Invalid == anEventId) {
              PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromFlatBuf.InvalidGameEvent",
                                "'%s' @ %i ms : Has an invalid audio event", animNameDebug.c_str(), _triggerTime_ms);
              // Move on to next Event
              continue;
            }
            anEventGroup.AddEvent(anEventId, volumes->Get(idx), probabilities->Get(idx));
          }
          
          if (anEventGroup.Events.empty()) {
            PRINT_NAMED_ERROR("RobotAudioKeyFrame.SetMembersFromFlatBuf.InvalidGameEventGroup",
                              "'%s' @ %i ms : Has an empty event group", animNameDebug.c_str(), _triggerTime_ms);
            return  RESULT_FAIL;
          }
          Result addResult = AddAudioRef(AudioRef(std::move(anEventGroup)));
          if (addResult != RESULT_OK) {
            return addResult;
          }
        }
      }
      return RESULT_OK;
    }
    
#pragma mark -
#pragma mark EventKeyFrame
    //
    // EventKeyFrame
    //
    
    #if CAN_STREAM
      RobotInterface::EngineToRobot* EventKeyFrame::GetStreamMessage(const TimeStamp_t timeSinceAnimStart_ms) const
      {
        // This function isn't actually used. Instead GetAnimEvent() is used by animationStreamer.
        DEV_ASSERT(false, "EventKeyFrame.GetStreamMessage.ShouldntCallThis");
        return nullptr;
      }
    #endif

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
      _event_id = e;
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
          _event_id = e;
        } else {
          PRINT_NAMED_WARNING("EventKeyFrame.EventIDNotString", "");
          return RESULT_FAIL;
        }
      }

      return RESULT_OK;
    }
    
#pragma mark -
#pragma mark BackpackLightsKeyFrame
    
    BackpackLightsKeyFrame::BackpackLightsKeyFrame()
    {
      _streamMsg.layer = 1; // 1 == BPL_ANIMATION
    }
    
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
_streamMsg.lights[__LED_NAME__].onColor = color; \
_streamMsg.lights[__LED_NAME__].offColor = color; \
_streamMsg.lights[__LED_NAME__].onFrames = 0; \
_streamMsg.lights[__LED_NAME__].offFrames = 0; \
_streamMsg.lights[__LED_NAME__].transitionOnFrames = 0; \
_streamMsg.lights[__LED_NAME__].transitionOffFrames = 0; \
_streamMsg.lights[__LED_NAME__].offset = 0; } while(0)

      GET_COLOR_FROM_JSON(Front,  (int)LEDId::LED_BACKPACK_FRONT);
      GET_COLOR_FROM_JSON(Middle, (int)LEDId::LED_BACKPACK_MIDDLE);
      GET_COLOR_FROM_JSON(Back,   (int)LEDId::LED_BACKPACK_BACK);
      
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, durationTime_ms, motionDuration_ms);
      _keyframeDuration_ms = _motionDuration_ms;

      return RESULT_OK;
    }
  
    #if CAN_STREAM
      RobotInterface::EngineToRobot* BackpackLightsKeyFrame::GetStreamMessage(const TimeStamp_t timeSinceAnimStart_ms) const
      {
        return new RobotInterface::EngineToRobot(_streamMsg);
      }
    #endif
    
#pragma mark -
#pragma mark BodyMotionKeyFrame
    
    BodyMotionKeyFrame::BodyMotionKeyFrame()
    {
      _streamMsg.accel = 0.f;
      
      // The stop message should command the wheel speeds to zero immediately, so command
      // zero velocity and 'infinite' radius
      _stopMsg.speed = 0.f;
      _stopMsg.accel = 0.f;
      _stopMsg.curvatureRadius_mm = std::numeric_limits<decltype(_stopMsg.curvatureRadius_mm)>::max();
    }
    
    BodyMotionKeyFrame::BodyMotionKeyFrame(s16 speed, s16 curvatureRadius_mm, s32 duration_ms)
    : BodyMotionKeyFrame()
    {
      bool isPointTurn = curvatureRadius_mm == 0;
      
      _keyframeDuration_ms = duration_ms;
      _streamMsg.speed = isPointTurn ? DEG_TO_RAD(speed) : speed;
      _streamMsg.curvatureRadius_mm = curvatureRadius_mm;
      _streamMsg.accel = isPointTurn ? 50.f : 0.f;  // 50 is what has been used on V1
    }
    
    void BodyMotionKeyFrame::CheckRotationSpeed(const std::string& animNameDebug)
    {
      // Check that speed is valid
      if (std::abs(_streamMsg.speed) > MAX_BODY_ROTATION_SPEED_DEG_PER_SEC) {
        PRINT_CH_DEBUG("Animations", "BodyMotionKeyFrame.CheckRotationSpeed.PointTurnSpeedExceedsLimit",
                       "%s: PointTurn speed %f deg/s exceeds limit of %f deg/s. Clamping",
                       animNameDebug.c_str(), std::abs(_streamMsg.speed), MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
        _streamMsg.speed = CLIP((f32)_streamMsg.speed,
                                -MAX_BODY_ROTATION_SPEED_DEG_PER_SEC,
                                MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
      }
    }

    void BodyMotionKeyFrame::CheckStraightSpeed(const std::string& animNameDebug)
    {
      // Check that speed is valid
      if (std::abs(_streamMsg.speed) > MAX_WHEEL_SPEED_MMPS) {
        PRINT_CH_DEBUG("Animations", "BodyMotionKeyFrame.CheckStraightSpeed.StraightSpeedExceedsLimit",
                       "%s: Speed %f mm/s exceeds limit of %f mm/s. Clamping",
                       animNameDebug.c_str(), std::abs(_streamMsg.speed), MAX_WHEEL_SPEED_MMPS);
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
        PRINT_CH_DEBUG("Animations", "BodyMotionKeyFrame.CheckTurnSpeed.ArcSpeedExceedsLimit",
                       "%s: Speed %f mm/s exceeds limit of %f mm/s. Clamping",
                       animNameDebug.c_str(), std::abs(_streamMsg.speed), MAX_WHEEL_SPEED_MMPS);
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
      Result res = RESULT_OK;
      
      SafeNumericCast(bodyKeyframe->durationTime_ms(), _motionDuration_ms, animNameDebug.c_str());
      SafeNumericCast(bodyKeyframe->speed(),           _streamMsg.speed,   animNameDebug.c_str());
      _keyframeDuration_ms = _motionDuration_ms;

      const std::string& radiusStr = bodyKeyframe->radius_mm()->str();
      if (has_any_digits(radiusStr)) {
        SafeNumericCast(std::atoi(radiusStr.c_str()), _streamMsg.curvatureRadius_mm, animNameDebug.c_str());
        CheckTurnSpeed(animNameDebug);
        if (_streamMsg.curvatureRadius_mm == 0) {
          _streamMsg.accel = 50.f;  // 50 was used in V1 for point turns
        }
      } else {
        res = ProcessRadiusString(radiusStr, animNameDebug);
      }

      return res;
    }

    Result BodyMotionKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      Result res = RESULT_OK;
      
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, durationTime_ms, motionDuration_ms);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, speed, streamMsg.speed);
      _keyframeDuration_ms = _motionDuration_ms;

      if(!jsonRoot.isMember("radius_mm")) {
        PRINT_NAMED_ERROR("BodyMotionKeyFrame.SetMembersFromJson.MissingRadius",
                          "%s: Missing 'radius_mm' field.",
                          animNameDebug.c_str());
        return RESULT_FAIL;
      } else if(jsonRoot["radius_mm"].isString()) {
        const std::string& radiusStr = jsonRoot["radius_mm"].asString();
        res = ProcessRadiusString(radiusStr, animNameDebug);
      } else {
        GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, radius_mm, streamMsg.curvatureRadius_mm);
        CheckTurnSpeed(animNameDebug);
        if (_streamMsg.curvatureRadius_mm == 0) {
          _streamMsg.accel = 50.f;  // 50 was used in V1 for point turns
        }
      }
      
      return res;
    }
    
    Result BodyMotionKeyFrame::ProcessRadiusString(const std::string& radiusStr, const std::string& animNameDebug)
    {
      if(radiusStr == "TURN_IN_PLACE" || radiusStr == "POINT_TURN") {
        _streamMsg.curvatureRadius_mm = 0;
        _streamMsg.accel = 50.f;  // 50 is what was used on V1 for point turns
        CheckRotationSpeed(animNameDebug);
        
        // Convert speed to radians from degrees
        _streamMsg.speed = DEG_TO_RAD(_streamMsg.speed);
        
      } else if(radiusStr == "STRAIGHT") {
        _streamMsg.curvatureRadius_mm = std::numeric_limits<s16>::max();
        _streamMsg.accel = 0.f;   // 0 is what was used on V1 for non point turns
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

    #if CAN_STREAM
      RobotInterface::EngineToRobot* BodyMotionKeyFrame::GetStreamMessage(const TimeStamp_t timeSinceAnimStart_ms) const
      {
        //PRINT_NAMED_INFO("BodyMotionKeyFrame.GetStreamMessage",
        //                 "currentTime=%d, duration=%d\n", timeSinceAnimStart_ms, _motionDuration_ms);
        if(IsFirstKeyframeTick(timeSinceAnimStart_ms)) {
          // Send the motion command at the beginning
          return new RobotInterface::EngineToRobot(_streamMsg);
        } else if(_enableStopMessage && GetTimeSinceTrigger(timeSinceAnimStart_ms) >= _motionDuration_ms) {
          // Send a stop command when the duration has passed
          return new RobotInterface::EngineToRobot(_stopMsg);
        } else {
          // Do nothing in the middle or if no done message is required.
          // (Note that IsDone() will return false during
          // this period so the animation track won't advance.)
          return nullptr;
        }
      }
    #endif
    
    bool BodyMotionKeyFrame::IsDone(const TimeStamp_t timeSinceAnimStart_ms) const
    {
      if(timeSinceAnimStart_ms < GetTriggerTime_ms()){
        return false;
      }
      
      // One additional frame is needed if a stop message should be sent
      auto timeShouldFinish = _enableStopMessage ? timeSinceAnimStart_ms - ANIM_TIME_STEP_MS : timeSinceAnimStart_ms;
      return GetTimeSinceTrigger(timeShouldFinish) >= _keyframeDuration_ms;
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
    
    #if CAN_STREAM
      RobotInterface::EngineToRobot* RecordHeadingKeyFrame::GetStreamMessage(const TimeStamp_t timeSinceAnimStart_ms) const
      {
        if (IsFirstKeyframeTick(timeSinceAnimStart_ms)) {
          return new RobotInterface::EngineToRobot(_streamMsg);
        }
        return nullptr;
      }
    #endif
    
    bool RecordHeadingKeyFrame::IsDone(const TimeStamp_t timeSinceAnimStart_ms) const
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
      _keyframeDuration_ms = duration_ms;
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
        PRINT_CH_DEBUG("Animations", "TurnToRecordedHeadingKeyFrame.CheckRotationSpeed.PointTurnSpeedExceedsLimit",
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
        PRINT_CH_DEBUG("Animations", "TurnToRecordedHeadingKeyFrame.CheckRotationAccel.PointTurnAccelExceedsLimit",
                       "%s: PointTurn accel %d deg/s^2 exceeds limit of %f deg/s^2. Clamping",
                       animNameDebug.c_str(),
                       std::abs(_streamMsg.accel_degPerSec2),
                       MAX_BODY_ROTATION_ACCEL_DEG_PER_SEC2);
        _streamMsg.accel_degPerSec2 = CLIP((f32)_streamMsg.accel_degPerSec2,
                                          -MAX_BODY_ROTATION_ACCEL_DEG_PER_SEC2,
                                          MAX_BODY_ROTATION_ACCEL_DEG_PER_SEC2);
      }
      if (std::abs(_streamMsg.decel_degPerSec2) > MAX_BODY_ROTATION_ACCEL_DEG_PER_SEC2) {
        PRINT_CH_DEBUG("Animations", "TurnToRecordedHeadingKeyFrame.CheckRotationAccel.PointTurnDecelExceedsLimit",
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
      SafeNumericCast(turnToRecordedHeadingKeyframe->durationTime_ms(),  _motionDuration_ms,        dbgName);
      SafeNumericCast(turnToRecordedHeadingKeyframe->offset_deg(),       _streamMsg.offset_deg,       dbgName);
      SafeNumericCast(turnToRecordedHeadingKeyframe->speed_degPerSec(),  _streamMsg.speed_degPerSec,  dbgName);
      SafeNumericCast(turnToRecordedHeadingKeyframe->accel_degPerSec2(), _streamMsg.accel_degPerSec2, dbgName);
      SafeNumericCast(turnToRecordedHeadingKeyframe->decel_degPerSec2(), _streamMsg.decel_degPerSec2, dbgName);
      SafeNumericCast(turnToRecordedHeadingKeyframe->tolerance_deg(),    _streamMsg.tolerance_deg,    dbgName);
      SafeNumericCast(turnToRecordedHeadingKeyframe->numHalfRevs(),      _streamMsg.numHalfRevs,      dbgName);
      _keyframeDuration_ms = _motionDuration_ms;
      _streamMsg.useShortestDir = turnToRecordedHeadingKeyframe->useShortestDir();
      
      CheckRotationSpeed(animNameDebug);
      
      return RESULT_OK;
    }
    
    Result TurnToRecordedHeadingKeyFrame::SetMembersFromJson(const Json::Value &jsonRoot, const std::string& animNameDebug)
    {
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, durationTime_ms,  motionDuration_ms);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, offset_deg,       streamMsg.offset_deg);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, speed_degPerSec,  streamMsg.speed_degPerSec);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, accel_degPerSec2, streamMsg.accel_degPerSec2);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, decel_degPerSec2, streamMsg.decel_degPerSec2);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, tolerance_deg,    streamMsg.tolerance_deg);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, numHalfRevs,      streamMsg.numHalfRevs);
      GET_MEMBER_FROM_JSON_AND_STORE_IN(jsonRoot, useShortestDir,   streamMsg.useShortestDir);
      _keyframeDuration_ms = _motionDuration_ms;

      CheckRotationSpeed(animNameDebug);

      return RESULT_OK;
    }
    
    #if CAN_STREAM
      RobotInterface::EngineToRobot* TurnToRecordedHeadingKeyFrame::GetStreamMessage(const TimeStamp_t timeSinceAnimStart_ms) const
      {
        if (IsFirstKeyframeTick(timeSinceAnimStart_ms)) {
          return new RobotInterface::EngineToRobot(_streamMsg);
        }
        return nullptr;
      }
    #endif
    
  } // namespace Cozmo
} // namespace Anki
