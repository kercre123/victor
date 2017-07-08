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

#include "anki/cozmo/basestation/animations/trackLayerManagers/faceLayerManager.h"

#include "anki/cozmo/basestation/animations/proceduralFaceDrawer.h"
#include "anki/cozmo/basestation/scanlineDistorter.h"
#include "anki/cozmo/basestation/utils/hasSettableParameters_impl.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#define DEBUG_FACE_LAYERING 0

#define CONSOLE_GROUP_NAME "FaceLayers"

namespace Anki {
  namespace Cozmo {
    
namespace {
  CONSOLE_VAR(f32, kMaxBlinkSpacingTimeForScreenProtection_ms, CONSOLE_GROUP_NAME, 30000);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FaceLayerManager::FaceLayerManager(const Util::RandomGenerator& rng)
: ITrackLayerManager<ProceduralFaceKeyFrame>(rng)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FaceLayerManager::GetFaceHelper(Animations::Track<ProceduralFaceKeyFrame>& track,
                                     TimeStamp_t startTime_ms, TimeStamp_t currTime_ms,
                                     ProceduralFaceKeyFrame& procFace,
                                     bool shouldReplace)
{
  bool paramsSet = false;
  
  const TimeStamp_t currStreamTime = currTime_ms - startTime_ms;
  if(track.HasFramesLeft()) {
    ProceduralFaceKeyFrame& currentKeyFrame = track.GetCurrentKeyFrame();
    if(currentKeyFrame.IsTimeToPlay(startTime_ms, currTime_ms))
    {
      ProceduralFace interpolatedFace;
      
      const ProceduralFaceKeyFrame* nextFrame = track.GetNextKeyFrame();
      if (nextFrame != nullptr) {
        if (nextFrame->IsTimeToPlay(startTime_ms, currTime_ms)) {
          // If it's time to play the next frame and the current frame at the same time, something's wrong!
          PRINT_NAMED_WARNING("AnimationStreamer.GetFaceHelper.FramesTooClose",
                              "currentFrameTriggerTime: %d ms, nextFrameTriggerTime: %d, StreamTime: %d",
                              currentKeyFrame.GetTriggerTime(), nextFrame->GetTriggerTime(), currStreamTime);
          
          // Something is wrong. Just move to next frame...
          track.MoveToNextKeyFrame();
          
        } else {
          /*
           // If we're within one sample period following the currFrame, just play the current frame
           if (currStreamTime - currentKeyFrame.GetTriggerTime() < IKeyFrame::SAMPLE_LENGTH_MS) {
           interpolatedParams = currentKeyFrame.GetFace().GetParams();
           paramsSet = true;
           }
           // We're on the way to the next frame, but not too close to it: interpolate.
           else if (nextFrame->GetTriggerTime() - currStreamTime >= IKeyFrame::SAMPLE_LENGTH_MS) {
           */
          interpolatedFace = currentKeyFrame.GetInterpolatedFace(*nextFrame, currTime_ms - startTime_ms);
          paramsSet = true;
          //}
          
          if (nextFrame->IsTimeToPlay(startTime_ms, currTime_ms + IKeyFrame::SAMPLE_LENGTH_MS)) {
            track.MoveToNextKeyFrame();
          }
          
        }
      } else {
        // There's no next frame to interpolate towards: just send this keyframe
        // and move forward
        interpolatedFace = currentKeyFrame.GetFace();
        track.MoveToNextKeyFrame();
        paramsSet = true;
      }
      
      if(paramsSet) {
        if(DEBUG_FACE_LAYERING) {
          const Point2f& facePosition = interpolatedFace.GetFacePosition();
          PRINT_NAMED_DEBUG("AnimationStreamer.GetFaceHelper.EyeShift",
                            "Applying eye shift from face layer of (%.1f,%.1f)",
                            facePosition.x(), facePosition.y());
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceLayerManager::RemoveKeepFaceAlive(s32 duration_ms)
{
  if(NotAnimatingTag != _eyeDartTag) {
    RemovePersistentLayer(_eyeDartTag, duration_ms);
    _eyeDartTag = NotAnimatingTag;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
static inline T GetParam(const std::map<LiveIdleAnimationParameter,f32>& params, LiveIdleAnimationParameter name) {
  return static_cast<T>(params.at(name));
}

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

void FaceLayerManager::GenerateEyeShift(const std::map<LiveIdleAnimationParameter,f32>& params,
                                         ProceduralFaceKeyFrame& frame) const
{
  using Param = LiveIdleAnimationParameter;
  
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
  
  ProceduralFaceKeyFrame keyframe(procFace, duration);
  frame = std::move(keyframe);
}

void FaceLayerManager::GenerateBlink(Animations::Track<ProceduralFaceKeyFrame>& track) const
{
  ProceduralFace blinkFace;
  
  TimeStamp_t totalOffset = 0;
  bool moreBlinkFrames = false;
  do {
    TimeStamp_t timeInc;
    moreBlinkFrames = ProceduralFaceDrawer::GetNextBlinkFrame(blinkFace, timeInc);
    totalOffset += timeInc;
    track.AddKeyFrameToBack(ProceduralFaceKeyFrame(blinkFace, totalOffset));
  } while(moreBlinkFrames);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceLayerManager::KeepFaceAlive(const std::map<LiveIdleAnimationParameter,f32>& params)
{
  using Param = LiveIdleAnimationParameter;
  
  _nextBlink_ms   -= BS_TIME_STEP;
  _nextEyeDart_ms -= BS_TIME_STEP;
  
  // Eye darts
  const f32 MaxDist = GetParam<f32>(params, Param::EyeDartMaxDistance_pix);
  if(_nextEyeDart_ms <= 0 && MaxDist > 0.f)
  {
    const size_t numLayers = GetNumLayers();
    const bool noOtherFaceLayers = (numLayers == 0 ||
                                    (numLayers == 1 && HasLayerWithTag(_eyeDartTag)));
    
    // If there's no other face layer active right now, do the dart. Otherwise,
    // skip it
    if(noOtherFaceLayers)
    {
      ProceduralFaceKeyFrame frame;
      GenerateEyeShift(params, frame);
      
      if(_eyeDartTag == NotAnimatingTag)
      {
        FaceTrack faceTrack;
        faceTrack.AddKeyFrameToBack(frame);
        _eyeDartTag = AddPersistentLayer("KeepAliveEyeDart", faceTrack);
      }
      else
      {
        AddToPersistentLayer(_eyeDartTag, frame);
      }
      
      _nextEyeDart_ms = GetRNG().RandIntInRange(GetParam<s32>(params, Param::EyeDartSpacingMinTime_ms),
                                                GetParam<s32>(params, Param::EyeDartSpacingMaxTime_ms));
    }
  }
  
  // Blinks
  if(_nextBlink_ms <= 0)
  {
    Animations::Track<ProceduralFaceKeyFrame> track;
    GenerateBlink(track);
    
    if(DEBUG_FACE_LAYERING)
    {
      // Sanity check: we should never command two blinks at the same time
      bool alreadyBlinking = HasLayerWithName("Blink");
      
      if(!alreadyBlinking)
      {
        AddLayer("Blink", track);
      }
    }
    else
    {
      AddLayer("Blink", track);
    }
    
    s32 blinkSpaceMin_ms = GetParam<s32>(params, Param::BlinkSpacingMinTime_ms);
    s32 blinkSpaceMax_ms = GetParam<s32>(params, Param::BlinkSpacingMaxTime_ms);
    if(blinkSpaceMax_ms <= blinkSpaceMin_ms)
    {
      PRINT_NAMED_WARNING("AnimationStreamer.KeepFaceAlive.BadBlinkSpacingParams",
                          "Max (%d) must be greater than min (%d)",
                          blinkSpaceMax_ms, blinkSpaceMin_ms);
      blinkSpaceMin_ms = kMaxBlinkSpacingTimeForScreenProtection_ms * .25f;
      blinkSpaceMax_ms = kMaxBlinkSpacingTimeForScreenProtection_ms;
    }
    _nextBlink_ms = GetRNG().RandIntInRange(blinkSpaceMin_ms, blinkSpaceMax_ms);
    
  }
  
} // KeepFaceAlive()


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

void FaceLayerManager::GenerateSquint(f32 squintScaleX, f32 squintScaleY, f32 upperLidAngle,
                                       Animations::Track<ProceduralFaceKeyFrame>& track) const
{
  ProceduralFace squintFace;
  
  const f32 DockSquintScaleY = 0.35f;
  const f32 DockSquintScaleX = 1.05f;
  squintFace.SetParameterBothEyes(ProceduralFace::Parameter::EyeScaleY, DockSquintScaleY);
  squintFace.SetParameterBothEyes(ProceduralFace::Parameter::EyeScaleX, DockSquintScaleX);
  squintFace.SetParameterBothEyes(ProceduralFace::Parameter::UpperLidAngle, -10);
  
  track.AddKeyFrameToBack(ProceduralFaceKeyFrame()); // need start frame at t=0 to get interpolation
  track.AddKeyFrameToBack(ProceduralFaceKeyFrame(squintFace, 250));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 FaceLayerManager::GetMaxBlinkSpacingTimeForScreenProtection_ms() const
{
  return kMaxBlinkSpacingTimeForScreenProtection_ms;
}

} // namespace Cozmo
} // namespace Anki

