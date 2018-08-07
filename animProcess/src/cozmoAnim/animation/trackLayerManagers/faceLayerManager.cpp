/**
 * File: faceLayerManager.cpp
 *
 * Authors: Andrew Stein
 * Created: 05/16/2017
 *
 * Description: Specific track layer manager for ProceduralFaceKeyFrames
 *              Handles procedural face layering, which includes things like KeepAlive, 
 *              look-ats while turning, blinks, and repair glitches.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cozmoAnim/animation/trackLayerManagers/faceLayerManager.h"

#include "cannedAnimLib/proceduralFace/proceduralFaceDrawer.h"
#include "cannedAnimLib/proceduralFace/scanlineDistorter.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#define DEBUG_FACE_LAYERING 0

#define CONSOLE_GROUP_NAME "Face.Layers"

namespace Anki {
namespace Vector {
    
namespace {
CONSOLE_VAR(f32, kMaxBlinkSpacingTimeForScreenProtection_ms, CONSOLE_GROUP_NAME, 30000);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FaceLayerManager::FaceLayerManager(const Util::RandomGenerator& rng)
: ITrackLayerManager<ProceduralFaceKeyFrame>(rng)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FaceLayerManager::GetFaceHelper(const Animations::Track<ProceduralFaceKeyFrame>& track,
                                     TimeStamp_t timeSinceAnimStart_ms,
                                     ProceduralFaceKeyFrame& procFace,
                                     bool shouldReplace) const
{
  bool paramsSet = false;
  
  if(track.HasFramesLeft()) {
    ProceduralFaceKeyFrame& currentKeyFrame = track.GetCurrentKeyFrame();
    if(currentKeyFrame.IsTimeToPlay(timeSinceAnimStart_ms))
    {
      ProceduralFace interpolatedFace;
      
      const ProceduralFaceKeyFrame* nextFrame = track.GetNextKeyFrame();
      if (nextFrame != nullptr) {
        if (nextFrame->IsTimeToPlay(timeSinceAnimStart_ms)) {
          // If it's time to play the next frame and the current frame at the same time, something's wrong!
          PRINT_NAMED_WARNING("FaceLayerManager.GetFaceHelper.FramesTooClose",
                              "currentFrameTriggerTime: %d ms, nextFrameTriggerTime: %d, StreamTime: %d",
                              currentKeyFrame.GetTriggerTime_ms(), nextFrame->GetTriggerTime_ms(), timeSinceAnimStart_ms);
        } else {
          /*
           // If we're within one sample period following the currFrame, just play the current frame
           if (currStreamTime - currentKeyFrame.GetTriggerTime_ms() < ANIM_TIME_STEP_MS) {
           interpolatedParams = currentKeyFrame.GetFace().GetParams();
           paramsSet = true;
           }
           // We're on the way to the next frame, but not too close to it: interpolate.
           else if (nextFrame->GetTriggerTime_ms() - currStreamTime >= ANIM_TIME_STEP_MS) {
           */
          interpolatedFace = currentKeyFrame.GetInterpolatedFace(*nextFrame, timeSinceAnimStart_ms);
          paramsSet = true;
          //}
        }
      } else {
        // There's no next frame to interpolate towards: just send this keyframe
        interpolatedFace = currentKeyFrame.GetFace();
        paramsSet = true;
      }
      
      if(paramsSet) {
        if(DEBUG_FACE_LAYERING) {
          PRINT_NAMED_DEBUG("AnimationStreamer.GetFaceHelper.EyeShift",
                            "Applying eye shift from face layer of (%.1f,%.1f)",
                            interpolatedFace.GetFacePosition().x(),
                            interpolatedFace.GetFacePosition().y());
        }
        
        if (shouldReplace)
        {
          procFace = interpolatedFace;
        }
        else
        {
          const_cast<ProceduralFace&>(procFace.GetFace()).Combine(interpolatedFace);
        }
      }
    } // if(nextFrame != nullptr
  } // if(track.HasFramesLeft())
  
  return paramsSet;
} // GetFaceHelper()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
static inline T GetParam(const std::map<KeepFaceAliveParameter,f32>& params, KeepFaceAliveParameter name) {
  return static_cast<T>(params.at(name));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceLayerManager::GenerateEyeShift(f32 xPix, f32 yPix,
                                         f32 xMax, f32 yMax,
                                         f32 lookUpMaxScale,
                                         f32 lookDownMinScale,
                                         f32 outerEyeScaleIncrease,
                                         TimeStamp_t duration_ms,
                                         ProceduralFaceKeyFrame& frame) const
{
  ProceduralFace procFace;
  ProceduralFace::Value xMin=0, yMin=0;
  procFace.GetEyeBoundingBox(xMin, xMax, yMin, yMax);
  procFace.LookAt(xPix, yPix,
                  std::max(xMin, ProceduralFace::WIDTH-xMax),
                  std::max(yMin, ProceduralFace::HEIGHT-yMax),
                  lookUpMaxScale, lookDownMinScale, outerEyeScaleIncrease);
  
  ProceduralFaceKeyFrame keyframe(procFace, duration_ms);
  frame = std::move(keyframe);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceLayerManager::GenerateEyeShift(const std::map<KeepFaceAliveParameter,f32>& params,
                                        ProceduralFaceKeyFrame& frame) const
{
  using Param = KeepFaceAliveParameter;
  
  const f32 MaxDist = GetParam<f32>(params, Param::EyeDartMaxDistance_pix);
  const f32 xDart = GetRNG().RandIntInRange(-MaxDist, MaxDist);
  const f32 yDart = GetRNG().RandIntInRange(-MaxDist, MaxDist);
  
  // Randomly choose how long the shift should take
  const s32 duration = GetRNG().RandIntInRange(GetParam<s32>(params, Param::EyeDartMinDuration_ms),
                                               GetParam<s32>(params, Param::EyeDartMaxDuration_ms));
  
  const f32 normDist = 5.f;
  ProceduralFace procFace;
  procFace.LookAt(xDart, yDart, normDist, normDist,
                  GetParam<f32>(params, Param::EyeDartUpMaxScale),
                  GetParam<f32>(params, Param::EyeDartDownMinScale),
                  GetParam<f32>(params, Param::EyeDartOuterEyeScaleIncrease));
  
  // Will be replaced when added to track
  const TimeStamp_t triggerTime_temp = 0;
  ProceduralFaceKeyFrame keyframe(procFace, triggerTime_temp, duration);
  frame = std::move(keyframe);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceLayerManager::GenerateBlink(Animations::Track<ProceduralFaceKeyFrame>& track,
                                     const TimeStamp_t timeSinceKeepAliveStart_ms,
                                     BlinkEventList& out_eventList) const
{
  ProceduralFace blinkFace;
  TimeStamp_t totalOffset = timeSinceKeepAliveStart_ms;
  BlinkState blinkState;
  TimeStamp_t timeInc;
  bool moreBlinkFrames = false;
  out_eventList.clear();
  do {
    moreBlinkFrames = ProceduralFaceDrawer::GetNextBlinkFrame(blinkFace, blinkState, timeInc);
    track.AddKeyFrameToBack(ProceduralFaceKeyFrame(blinkFace, totalOffset, timeInc));
    out_eventList.emplace_back(totalOffset, blinkState);
    totalOffset += timeInc;
  } while(moreBlinkFrames);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result FaceLayerManager::AddBlinkToFaceTrack(const std::string& layerName,
                                               const TimeStamp_t timeSinceKeepAliveStart_ms,
                                               BlinkEventList& out_eventList)
{
  if (HasLayer(layerName)) {
    out_eventList.clear();
    return RESULT_FAIL;
  }
  Animations::Track<ProceduralFaceKeyFrame> faceTrack;
  GenerateBlink(faceTrack, timeSinceKeepAliveStart_ms, out_eventList);
  Result result = AddLayer(layerName, faceTrack);
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
s32 FaceLayerManager::GetNextBlinkTime_ms(const std::map<KeepFaceAliveParameter, f32>& params) const
{
  s32 blinkSpaceMin_ms = GetParam<s32>(params, KeepFaceAliveParameter::BlinkSpacingMinTime_ms);
  s32 blinkSpaceMax_ms = GetParam<s32>(params, KeepFaceAliveParameter::BlinkSpacingMaxTime_ms);
  if(blinkSpaceMax_ms <= blinkSpaceMin_ms)
  {
    PRINT_NAMED_WARNING("AnimationStreamer.KeepFaceAlive.BadBlinkSpacingParams",
                        "Max (%d) must be greater than min (%d)",
                        blinkSpaceMax_ms, blinkSpaceMin_ms);
    blinkSpaceMin_ms = kMaxBlinkSpacingTimeForScreenProtection_ms * .25f;
    blinkSpaceMax_ms = kMaxBlinkSpacingTimeForScreenProtection_ms;
  }
  return GetRNG().RandIntInRange(blinkSpaceMin_ms, blinkSpaceMax_ms);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result FaceLayerManager::AddEyeDartToFaceTrack(const std::string& layerName,
                                               const std::map<KeepFaceAliveParameter,f32>& params,
                                               const TimeStamp_t timeSinceKeepAliveStart_ms,
                                               TimeStamp_t& out_interpolationTime_ms)
{
  const f32 MaxDist = GetParam<f32>(params, KeepFaceAliveParameter::EyeDartMaxDistance_pix);
  out_interpolationTime_ms = 0;
  if (MaxDist > 0.f) {
    const size_t numLayers = GetNumLayers();
    const bool hasDartLayer = HasLayer(layerName);
    const bool noOtherFaceLayers = (numLayers == 0 ||
                                    (numLayers == 1 && hasDartLayer));

    // If there's no other face layer active right now, do the dart. Otherwise,
    // skip it
    if(noOtherFaceLayers) {
      ProceduralFaceKeyFrame frame;
      GenerateEyeShift(params, frame);
      out_interpolationTime_ms = frame.GetTriggerTime_ms();
      
      if(!hasDartLayer) {
        FaceTrack faceTrack;
        // Generate eye shift generates frames with a relative offset for its trigger time
        frame.SetTriggerTime_ms(frame.GetTriggerTime_ms() + timeSinceKeepAliveStart_ms);
        faceTrack.AddKeyFrameToBack(frame);
        AddPersistentLayer(layerName, faceTrack);
      }
      else {
        AddToPersistentLayer(layerName, frame);
      }
    }
  }

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
s32 FaceLayerManager::GetNextEyeDartTime_ms(const std::map<KeepFaceAliveParameter, f32>& params) const
{
  return GetRNG().RandIntInRange(GetParam<s32>(params, KeepFaceAliveParameter::EyeDartSpacingMinTime_ms),
                                 GetParam<s32>(params, KeepFaceAliveParameter::EyeDartSpacingMaxTime_ms));
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceLayerManager::AddKeepFaceAliveTrack(const std::string& layerName)
{
  ProceduralFaceKeyFrame frame;
  FaceTrack faceTrack;
  faceTrack.AddKeyFrameToBack(frame);
  AddLayer(layerName, faceTrack);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 FaceLayerManager::GenerateFaceDistortion(float distortionDegree,
                                               Animations::Track<ProceduralFaceKeyFrame>& track) const
{
  u32 numFrames = 0;
  ProceduralFace repairFace;
  
  TimeStamp_t totalOffset = 0;
  bool moreDistortionFrames = false;
  do {
    TimeStamp_t timeInc;
    moreDistortionFrames = ScanlineDistorter::GetNextDistortionFrame(distortionDegree, repairFace, timeInc);
    totalOffset += timeInc;
    track.AddKeyFrameToBack(ProceduralFaceKeyFrame(repairFace, totalOffset));
    ++numFrames;
  } while(moreDistortionFrames);
  return numFrames;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceLayerManager::GenerateSquint(f32 squintScaleX,
                                      f32 squintScaleY,
                                      f32 upperLidAngle,
                                      Animations::Track<ProceduralFaceKeyFrame>& track,
                                      const TimeStamp_t timeSinceKeepAliveStart_ms) const
{
  ProceduralFace squintFace;
  const f32 DockSquintScaleY = 0.35f;
  const f32 DockSquintScaleX = 1.05f;
  const TimeStamp_t interpolationTime_ms = 250;
  squintFace.SetParameterBothEyes(ProceduralFace::Parameter::EyeScaleY, DockSquintScaleY);
  squintFace.SetParameterBothEyes(ProceduralFace::Parameter::EyeScaleX, DockSquintScaleX);
  squintFace.SetParameterBothEyes(ProceduralFace::Parameter::UpperLidAngle, -10.0f);
  // need start at t=0 (a.k.a. timeSinceKeepAliveStart_ms) to get interpolation
  track.AddKeyFrameToBack(ProceduralFaceKeyFrame(timeSinceKeepAliveStart_ms, interpolationTime_ms));
  track.AddKeyFrameToBack(ProceduralFaceKeyFrame(squintFace, (timeSinceKeepAliveStart_ms + interpolationTime_ms)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 FaceLayerManager::GetMaxBlinkSpacingTimeForScreenProtection_ms() const
{
  return kMaxBlinkSpacingTimeForScreenProtection_ms;
}

} // namespace Vector
} // namespace Anki

