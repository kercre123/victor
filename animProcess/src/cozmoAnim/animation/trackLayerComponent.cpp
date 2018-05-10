/**
 * File: trackLayerComponent.cpp
 *
 * Authors: Al Chaussee
 * Created: 06/28/2017
 *
 * Description:
 *
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cozmoAnim/animation/trackLayerComponent.h"
#include "cozmoAnim/animation/animationStreamer.h"
#include "cozmoAnim/animation/trackLayerManagers/audioLayerManager.h"
#include "cozmoAnim/animation/trackLayerManagers/backpackLayerManager.h"
#include "cozmoAnim/animation/trackLayerManagers/faceLayerManager.h"
//#include "anki/cozmo/basestation/components/desiredFaceDistortionComponent.h"
#include "cozmoAnim/animContext.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
namespace {
// Keep Face Alive Layer Names
const std::string kEyeBlinkLayerName  = "KeepAliveEyeBlink";
const std::string kEyeDartLayerName   = "KeepAliveEyeDart";
const std::string kEyeNoiseLayerName  = "KeepAliveEyeNoise";
  
// TODO: Restore audio glitch
//CONSOLE_VAR(bool, kGenerateGlitchAudio, "ProceduralAnims", false);
bool kGenerateGlitchAudio = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TrackLayerComponent::TrackLayerComponent(const AnimContext* context)
: _audioLayerManager(new AudioLayerManager(*context->GetRandom()))
, _backpackLayerManager(new BackpackLayerManager(*context->GetRandom()))
, _faceLayerManager(new FaceLayerManager(*context->GetRandom()))
, _lastProceduralFace(new ProceduralFace())
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TrackLayerComponent::~TrackLayerComponent()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::Init()
{
  _lastProceduralFace->Reset();
  // Setup Keep Alive Activities
  SetupKeepFaceAliveActivities();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::Update()
{
  // TODO: Restore glitch ability via messaging from engine
  /*
  auto& desiredGlitchComponent = _context->GetNeedsManager()->GetDesiredFaceDistortionComponent();
  const float desiredGlitchDegree = desiredGlitchComponent.GetCurrentDesiredDistortion();
  if(Util::IsFltGTZero(desiredGlitchDegree))
  {
    AddGlitch(desiredGlitchDegree);
  }
   */
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::ApplyLayersToAnim(Animation* anim,
                                            TimeStamp_t startTime_ms,
                                            TimeStamp_t streamTime_ms,
                                            LayeredKeyFrames& layeredKeyframes,
                                            bool storeFace)
{
  // Apply layers of individual tracks to anim
  ApplyAudioLayersToAnim(anim, startTime_ms, streamTime_ms, layeredKeyframes);
  ApplyBackpackLayersToAnim(anim, startTime_ms, streamTime_ms, layeredKeyframes);
  ApplyFaceLayersToAnim(anim, startTime_ms, streamTime_ms, layeredKeyframes, storeFace);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::KeepFaceAlive(const std::map<KeepFaceAliveParameter, f32>& params)
{
  // Loop through keep alive activities and perform if timer expired
  bool hasFaceLayer = false;
  for (auto& keepAliveActivity : _keepAliveModifiers) {
    keepAliveActivity.nextPerformanceTime_ms -= ANIM_TIME_STEP_MS;
    if (keepAliveActivity.nextPerformanceTime_ms <= 0) {
      // Run Activity
      bool success = keepAliveActivity.performFunc(params);
      if (success) {
        hasFaceLayer |= keepAliveActivity.hasFaceLayers;
      }
      keepAliveActivity.UpdateNextPerformanceTime(params);
    }
  }
  
  if (!hasFaceLayer) {
    // Add Keep Alive Face Layer
    _faceLayerManager->AddKeepFaceAliveTrack(kEyeNoiseLayerName);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::RemoveKeepFaceAlive(u32 duration_ms)
{
  _audioLayerManager->RemovePersistentLayer(kEyeDartLayerName, duration_ms);
  _faceLayerManager->RemovePersistentLayer(kEyeDartLayerName, duration_ms);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::ResetKeepFaceAliveTimers()
{
  for (auto& activityIt : _keepAliveModifiers) {
    activityIt.nextPerformanceTime_ms = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TrackLayerComponent::HaveLayersToSend() const
{
  return (_audioLayerManager->HaveLayersToSend() ||
          _backpackLayerManager->HaveLayersToSend() ||
          _faceLayerManager->HaveLayersToSend());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::AddSquint(const std::string& name, f32 squintScaleX, f32 squintScaleY, f32 upperLidAngle)
{
  Animations::Track<ProceduralFaceKeyFrame> faceTrack;
  _faceLayerManager->GenerateSquint(squintScaleX, squintScaleY, upperLidAngle, faceTrack);
  _faceLayerManager->AddPersistentLayer(name, faceTrack);
  _audioLayerManager->AddEyeSquintToAudioTrack(name);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::RemoveSquint(const std::string& name, u32 duration_ms)
{
  _faceLayerManager->RemovePersistentLayer(name, duration_ms);
  _audioLayerManager->RemovePersistentLayer(name, duration_ms);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::AddOrUpdateEyeShift(const std::string& name,
                                              f32 xPix,
                                              f32 yPix,
                                              TimeStamp_t duration_ms,
                                              f32 xMax,
                                              f32 yMax,
                                              f32 lookUpMaxScale,
                                              f32 lookDownMinScale,
                                              f32 outerEyeScaleIncrease)
{
  ProceduralFaceKeyFrame eyeShift;
  _faceLayerManager->GenerateEyeShift(xPix, yPix,
                                      xMax, yMax,
                                      lookUpMaxScale,
                                      lookDownMinScale,
                                      outerEyeScaleIncrease,
                                      duration_ms, eyeShift);
  
  if(!_faceLayerManager->HasLayer(name))
  {
    AnimationStreamer::FaceTrack faceTrack;
    if(duration_ms > 0)
    {
      // Add an initial no-adjustment frame so we have something to interpolate
      // from on our way to the specified shift
      faceTrack.AddKeyFrameToBack(ProceduralFaceKeyFrame());
    }
    faceTrack.AddKeyFrameToBack(std::move(eyeShift));
    _faceLayerManager->AddPersistentLayer(name, faceTrack);
  }
  else
  {
    _faceLayerManager->AddToPersistentLayer(name, eyeShift);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::RemoveEyeShift(const std::string& name, u32 duration_ms)
{
  _faceLayerManager->RemovePersistentLayer(name, duration_ms);
  _audioLayerManager->RemovePersistentLayer(name, duration_ms);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::AddGlitch(f32 glitchDegree)
{
  PRINT_CH_DEBUG("Animations","TrackLayerComponent.AddGlitch","Degree %.2f",glitchDegree);
  Animations::Track<ProceduralFaceKeyFrame> faceTrack;
  const u32 numFrames = _faceLayerManager->GenerateFaceDistortion(glitchDegree, faceTrack);
  _faceLayerManager->AddLayer("Glitch", faceTrack);

  Animations::Track<BackpackLightsKeyFrame> track;
  _backpackLayerManager->GenerateGlitchLights(track);
  _backpackLayerManager->AddLayer("Glitch", track);

  if(kGenerateGlitchAudio)
  {
    Animations::Track<RobotAudioKeyFrame> audioTrack;
    _audioLayerManager->GenerateGlitchAudio(numFrames, audioTrack);
    _audioLayerManager->AddLayer("Glitch", audioTrack);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 TrackLayerComponent::GetMaxBlinkSpacingTimeForScreenProtection_ms() const
{
  return _faceLayerManager->GetMaxBlinkSpacingTimeForScreenProtection_ms();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::SetupKeepFaceAliveActivities()
{
  _keepAliveModifiers.clear();
  // Eye Blink
  auto eyeBlinkPerform = [this](const KeepAliveModifier::ParameterMap& parameterMap) {
    BlinkEventList eventList;
    Result result = _faceLayerManager->AddBlinkToFaceTrack(kEyeBlinkLayerName, eventList);
    if (RESULT_OK == result) {
      _audioLayerManager->AddEyeBlinkToAudioTrack(kEyeBlinkLayerName, eventList);
    }
    else {
      PRINT_NAMED_WARNING("TrackLayerComponent.SetupKeepFaceAliveActivities.eyeBlinkPerform",
                          "AddBlinkToFaceTrack.Failed");
    }
    return (RESULT_OK == result);
  };
  auto eyeBlinkTimeFunc = [this](const KeepAliveModifier::ParameterMap& parameterMap) {
    return _faceLayerManager->GetNextBlinkTime_ms(parameterMap);
  };
  _keepAliveModifiers.emplace_back( kEyeBlinkLayerName, eyeBlinkPerform, eyeBlinkTimeFunc, true );
  
  // Eye Dart
  auto eyeDartPerform = [this](const KeepAliveModifier::ParameterMap& parameterMap) {
    TimeStamp_t interpolationTime_ms = 0;
    Result result = _faceLayerManager->AddEyeDartToFaceTrack(kEyeDartLayerName, parameterMap, interpolationTime_ms);
    if (RESULT_OK == result) {
      _audioLayerManager->AddEyeDartToAudioTrack(kEyeDartLayerName, interpolationTime_ms);
    }
    else {
      PRINT_NAMED_WARNING("TrackLayerComponent.SetupKeepFaceAliveActivities.eyeDartPerform",
                          "AddBlinkToFaceTrack.Failed");
    }
    return (RESULT_OK == result);
  };
  auto eyeDartTimeFunc = [this](const KeepAliveModifier::ParameterMap& parameterMap) {
    return _faceLayerManager->GetNextEyeDartTime_ms(parameterMap);
  };
  _keepAliveModifiers.emplace_back( kEyeDartLayerName, eyeDartPerform, eyeDartTimeFunc, true );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::ApplyAudioLayersToAnim(Animation* anim,
                                                 TimeStamp_t startTime_ms,
                                                 TimeStamp_t streamTime_ms,
                                                 LayeredKeyFrames& layeredKeyFrames)
{
  if (anim != nullptr) {
    auto* frame = anim->GetTrack<RobotAudioKeyFrame>().GetCurrentKeyFrame(streamTime_ms - startTime_ms);
    if (frame != nullptr) {
      layeredKeyFrames.audioKeyFrame = *frame;
      layeredKeyFrames.haveAudioKeyFrame = true;
    }
  }
  
  if (_audioLayerManager->HaveLayersToSend()) {
    auto applyFunc = [](Animations::Track<RobotAudioKeyFrame>& layerTrack,
                        TimeStamp_t startTime_ms,
                        TimeStamp_t streamTime_ms,
                        RobotAudioKeyFrame& outFrame)
    {
      bool updatedFrame = false;
      while ( layerTrack.HasFramesLeft() ) {
        // Check if there is a frame for this time
        auto& frame = layerTrack.GetCurrentKeyFrame();
        if (!frame.IsTimeToPlay(startTime_ms, streamTime_ms)) {
          // NOT time to play this frame, break out of while loop
          break;
        }
        
        // Merge audio key frames
        outFrame.MergeKeyFrame(frame);
        updatedFrame = true;
        // Prep for next key frame
        layerTrack.MoveToNextKeyFrame();
      }
      return updatedFrame;
    };
    
    layeredKeyFrames.haveAudioKeyFrame |= _audioLayerManager->ApplyLayersToFrame(layeredKeyFrames.audioKeyFrame,
                                                                                 applyFunc);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::ApplyBackpackLayersToAnim(Animation* anim,
                                                    TimeStamp_t startTime_ms,
                                                    TimeStamp_t streamTime_ms,
                                                    LayeredKeyFrames& layeredKeyFrames)
{
  // If we have an anim and it has a backpack keyframe at this time
  if(anim != nullptr)
  {
    auto* frame = anim->GetTrack<BackpackLightsKeyFrame>().GetCurrentKeyFrame(streamTime_ms - startTime_ms);
    if(frame != nullptr)
    {
      layeredKeyFrames.backpackKeyFrame = *frame;
      layeredKeyFrames.haveBackpackKeyFrame = true;
    }
  }
  
  // If the backpackLayerManager has layers then we need to combine them together (with the keyframe from the anim)
  if(_backpackLayerManager->HaveLayersToSend())
  {
    auto applyFunc = [](Animations::Track<BackpackLightsKeyFrame>& layerTrack,
                        TimeStamp_t startTime_ms,
                        TimeStamp_t streamTime_ms,
                        BackpackLightsKeyFrame& outFrame)
    {
      // TODO: VIC-447: Restore glitching
      // NOTE: This might be a bug!!
      //       For live animations this will delete the tracks when calling layerTrack.GetCurrentKeyFrame()
      // Get the current keyframe from the layer's track (will move to the next keyframe automatically)
      auto* frame = layerTrack.GetCurrentKeyFrame(streamTime_ms - startTime_ms);
      if(frame != nullptr)
      {
        // TODO: Blend frame and outFrame?
        // Would need to account for whether or not the anim has a backpack keyframe
        outFrame = *frame;
        return true;
      }
      return false;
    };
    
    const bool haveBackpack = _backpackLayerManager->ApplyLayersToFrame(layeredKeyFrames.backpackKeyFrame,
                                                                        applyFunc);
    layeredKeyFrames.haveBackpackKeyFrame |= haveBackpack;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackLayerComponent::ApplyFaceLayersToAnim(Animation* anim,
                                                TimeStamp_t startTime_ms,
                                                TimeStamp_t streamTime_ms,
                                                LayeredKeyFrames& layeredKeyFrames,
                                                bool storeFace)
{
  // Set faceKeyframe to the last procedural face
  DEV_ASSERT(_lastProceduralFace != nullptr,
             "TrackLayerComponent.ApplyFaceLayersToAnim.LastFaceIsNull");
  layeredKeyFrames.faceKeyFrame = ProceduralFaceKeyFrame(*_lastProceduralFace);
  
  // If we have an animation then update faceKeyframe with it
  if(anim != nullptr)
  {
    // Face keyframe from animation should replace whatever is in layeredKeyframes.faceKeyframe
    const bool kShouldReplace = true;
    const bool faceUpdated = _faceLayerManager->GetFaceHelper(anim->GetTrack<ProceduralFaceKeyFrame>(),
                                                              startTime_ms,
                                                              streamTime_ms,
                                                              layeredKeyFrames.faceKeyFrame,
                                                              kShouldReplace);
    layeredKeyFrames.haveFaceKeyFrame = faceUpdated;
    
    // Update _lastProceduralFace if the face was updated and we should store the face
    if(faceUpdated && storeFace)
    {
      *_lastProceduralFace = layeredKeyFrames.faceKeyFrame.GetFace();
    }
  }
  
  // If the faceLayerManager has layers then we need to combine them together
  if(_faceLayerManager->HaveLayersToSend())
  {
    auto applyFunc = [this](Animations::Track<ProceduralFaceKeyFrame>& track,
                            TimeStamp_t startTime_ms,
                            TimeStamp_t streamTime_ms,
                            ProceduralFaceKeyFrame& outFrame)
    {
      // Procedural layers should not replace what is already in outFrame, they need to be combined with it
      const bool kShouldReplace = false;
      return _faceLayerManager->GetFaceHelper(track, startTime_ms, streamTime_ms, outFrame, kShouldReplace);
    };
    
    layeredKeyFrames.haveFaceKeyFrame |= _faceLayerManager->ApplyLayersToFrame(layeredKeyFrames.faceKeyFrame,
                                                                               applyFunc);
  }
}


}
}
