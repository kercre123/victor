/**
 * File: faceLayerManager.cpp
 *
 * Authors: Andrew Stein
 * Created: 05/16/2017
 *
 * Description:
 *
 *   Component of the AnimationStreamer to handle procedural face layering, which includes things like KeepAlive,
 *   look-ats while turning, blinks, and repair glitches.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/animation/faceLayerManager.h"

#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/proceduralFaceDrawer.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/scanlineDistorter.h"
#include "anki/cozmo/basestation/utils/hasSettableParameters_impl.h"

#include "clad/externalInterface/messageGameToEngineTag.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"

#define DEBUG_FACE_LAYERING 0

namespace Anki {
namespace Cozmo {

namespace {
  
  static const char * const kConsoleGroupName = "FaceLayers";
  
  CONSOLE_VAR(f32, kMaxBlinkSpacingTimeForScreenProtection_ms, kConsoleGroupName, 30000);
  
  CONSOLE_VAR(bool, kNeeds_ShowFaceRepairGlitches, kConsoleGroupName, false);
  
  // Face repair degree chosen randomly b/w these two values
  CONSOLE_VAR(f32, kNeeds_FaceRepairDegreeMin, kConsoleGroupName, 0.75f);
  CONSOLE_VAR(f32, kNeeds_FaceRepairDegreeMax, kConsoleGroupName, 3.00f);
  
  // Face repair frequence chosen randomly between these two values
  CONSOLE_VAR(f32, kNeeds_FaceRepairSpacingMin_sec, kConsoleGroupName, 0.5f);
  CONSOLE_VAR(f32, kNeeds_FaceRepairSpacingMax_sec, kConsoleGroupName, 1.5f);
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FaceLayerManager::FaceLayerManager(const CozmoContext* context)
: _rng(*context->GetRandom())
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceLayerManager::IncrementLayerTagCtr()
{
  // Increment the tag counter and keep it from being the "special"
  // value used to indicate "not animating" or any existing
  // layer tag in use
  ++_layerTagCtr;
  while(_layerTagCtr == NotAnimatingTag ||
        _faceLayers.find(_layerTagCtr) != _faceLayers.end())
  {
    ++_layerTagCtr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result FaceLayerManager::AddLayer(const std::string &name, FaceTrack &&faceTrack, TimeStamp_t delay_ms)
{
  Result lastResult = RESULT_OK;
  
  faceTrack.SetIsLive(true);
  
  IncrementLayerTagCtr();
  
  FaceLayer newLayer;
  newLayer.tag = _layerTagCtr;
  newLayer.track = faceTrack; // COPY the track in
  newLayer.track.Init();
  newLayer.startTime_ms = delay_ms;
  newLayer.streamTime_ms = 0;
  newLayer.isPersistent = false;
  newLayer.sentOnce = false;
  newLayer.name = name;
  
  if(DEBUG_FACE_LAYERING) {
    PRINT_NAMED_DEBUG("AnimationStreamer.AddFaceLayer",
                      "%s, Tag = %d (Total layers=%lu)",
                      newLayer.name.c_str(), newLayer.tag, (unsigned long)_faceLayers.size()+1);
  }
  
  _faceLayers[_layerTagCtr] = std::move(newLayer);
  
  return lastResult;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FaceLayerManager::Tag FaceLayerManager::AddPersistentLayer(const std::string &name, FaceTrack &&faceTrack)
{
  faceTrack.SetIsLive(false); // don't want keyframes to delete as they play
  
  IncrementLayerTagCtr();
  
  FaceLayer newLayer;
  newLayer.tag = _layerTagCtr;
  newLayer.track = faceTrack;
  newLayer.track.Init();
  newLayer.startTime_ms = 0;
  newLayer.streamTime_ms = 0;
  newLayer.isPersistent = true;
  newLayer.sentOnce = false;
  newLayer.name = name;

  if(DEBUG_FACE_LAYERING){
    PRINT_NAMED_DEBUG("AnimationStreamer.AddPersistentFaceLayer",
                      "%s, Tag = %d (Total layers=%lu)",
                      newLayer.name.c_str(), newLayer.tag, (unsigned long)_faceLayers.size());
  }
  
  _faceLayers[_layerTagCtr] = std::move(newLayer);
  
  return _layerTagCtr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceLayerManager::RemovePersistentLayer(Tag tag, s32 duration_ms)
{
  auto layerIter = _faceLayers.find(tag);
  if(layerIter != _faceLayers.end()) {
    PRINT_NAMED_INFO("AnimationStreamer.RemovePersistentFaceLayer",
                     "%s, Tag = %d (Layers remaining=%lu)",
                     layerIter->second.name.c_str(), layerIter->first, (unsigned long)_faceLayers.size()-1);
    

    // Add a layer that takes us back from where this persistent frame leaves
    // off to no adjustment at all.
    FaceTrack faceTrack;
    faceTrack.SetIsLive(true);
    if(duration_ms > 0)
    {
      ProceduralFaceKeyFrame firstFrame(layerIter->second.track.GetCurrentKeyFrame());
      firstFrame.SetTriggerTime(0);
      faceTrack.AddKeyFrameToBack(std::move(firstFrame));
    }
    ProceduralFaceKeyFrame lastFrame;
    lastFrame.SetTriggerTime(duration_ms);
    faceTrack.AddKeyFrameToBack(std::move(lastFrame));
    
    AddLayer("Remove" + layerIter->second.name, std::move(faceTrack));
    
    _faceLayers.erase(layerIter);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceLayerManager::AddToPersistentLayer(Tag tag, Anki::Cozmo::ProceduralFaceKeyFrame &&keyframe)
{
  auto layerIter = _faceLayers.find(tag);
  if(layerIter != _faceLayers.end()) {
    auto & track = layerIter->second.track;
    assert(nullptr != track.GetLastKeyFrame());
    // Make keyframe trigger one sample length (plus any internal delay) past
    // the last keyframe's trigger time
    keyframe.SetTriggerTime(track.GetLastKeyFrame()->GetTriggerTime() +
                            IKeyFrame::SAMPLE_LENGTH_MS +
                            keyframe.GetTriggerTime());
    track.AddKeyFrameToBack(keyframe);
    layerIter->second.sentOnce = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FaceLayerManager::GetFaceHelper(Animations::Track<ProceduralFaceKeyFrame>& track,
                                     TimeStamp_t startTime_ms, TimeStamp_t currTime_ms,
                                     ProceduralFace& procFace,
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
          procFace.Combine(interpolatedFace);
        }
      }
    } // if(nextFrame != nullptr
  } // if(track.HasFramesLeft())
  
  return paramsSet;
} // GetFaceHelper()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceLayerManager::RemoveKeepAliveEyeDart(s32 duration_ms)
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceLayerManager::KeepFaceAlive(Robot& robot, const std::map<LiveIdleAnimationParameter,f32>& params)
{
  using Param = LiveIdleAnimationParameter;
  
  _nextBlink_ms   -= BS_TIME_STEP;
  _nextEyeDart_ms -= BS_TIME_STEP;
  
  // Eye darts
  const f32 MaxDist = GetParam<f32>(params, Param::EyeDartMaxDistance_pix);
  if(_nextEyeDart_ms <= 0 && MaxDist > 0.f)
  {
    const bool noOtherFaceLayers = (_faceLayers.empty() ||
                                    (_faceLayers.size()==1 && _faceLayers.find(_eyeDartTag) != _faceLayers.end()));
    
    // If there's no other face layer active right now, do the dart. Otherwise,
    // skip it
    if(noOtherFaceLayers)
    {
      const f32 xDart = _rng.RandIntInRange(-MaxDist, MaxDist);
      const f32 yDart = _rng.RandIntInRange(-MaxDist, MaxDist);
      
      // Randomly choose how long the shift should take
      const s32 duration = _rng.RandIntInRange(GetParam<s32>(params, Param::EyeDartMinDuration_ms),
                                               GetParam<s32>(params, Param::EyeDartMaxDuration_ms));
      
      //PRINT_NAMED_DEBUG("AnimationStreamer.KeepFaceAlive.EyeDart",
      //                  "shift=(%.1f,%.1f)", xDart, yDart);
      
      const f32 normDist = 5.f;
      ProceduralFace procFace;
      procFace.LookAt(xDart, yDart, normDist, normDist,
                      GetParam<f32>(params, Param::EyeDartUpMaxScale),
                      GetParam<f32>(params, Param::EyeDartDownMinScale),
                      GetParam<f32>(params, Param::EyeDartOuterEyeScaleIncrease));
      
      if(_eyeDartTag == NotAnimatingTag) {
        FaceTrack faceTrack;
        faceTrack.AddKeyFrameToBack(ProceduralFaceKeyFrame(procFace, duration));
        _eyeDartTag = AddPersistentLayer("KeepAliveEyeDart", std::move(faceTrack));
      } else {
        AddToPersistentLayer(_eyeDartTag, ProceduralFaceKeyFrame(procFace, duration));
      }
      
      _nextEyeDart_ms = _rng.RandIntInRange(GetParam<s32>(params, Param::EyeDartSpacingMinTime_ms),
                                            GetParam<s32>(params, Param::EyeDartSpacingMaxTime_ms));
    }
  }
  
  // Blinks
  if(_nextBlink_ms <= 0)
  {
    ProceduralFace blinkFace;
    
    FaceTrack faceTrack;
    TimeStamp_t totalOffset = 0;
    bool moreBlinkFrames = false;
    do {
      TimeStamp_t timeInc;
      moreBlinkFrames = ProceduralFaceDrawer::GetNextBlinkFrame(blinkFace, timeInc);
      totalOffset += timeInc;
      faceTrack.AddKeyFrameToBack(ProceduralFaceKeyFrame(blinkFace, totalOffset));
    } while(moreBlinkFrames);

    if(DEBUG_FACE_LAYERING)
    {
      // Sanity checkt: we should never command two blinks at the same time
      bool alreadyBlinking = false;
      for(auto & layer : _faceLayers) {
        if(layer.second.name == "Blink") {
          PRINT_NAMED_WARNING("AnimationStreamer.KeepFaceAlive.DuplicateBlink",
                              "Seems like there's already a blink layer. Skipping this blink.");
          alreadyBlinking = true;
        }
      }
      
      if(!alreadyBlinking) {
        AddLayer("Blink", std::move(faceTrack));
      }
    } else {
      AddLayer("Blink", std::move(faceTrack));
    } // DEBUG_FACE_LAYERING
    
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
    _nextBlink_ms = _rng.RandIntInRange(blinkSpaceMin_ms, blinkSpaceMax_ms);
    
  }
  
} // KeepFaceAlive()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result FaceLayerManager::Update(const Robot& robot)
{
  // For prototyping. Eventually driven by actual needs system, not console var
  // E.g. something like if(robot.GetNeedsManager().GetCurNeedsState().DoesFaceNeedRepair())
  // Alternatively, just add a setter to FaceLayerManager for letting an external component
  // specify the next glitch time and its "degree" (and move the Rand() calls to that
  // component. The advantage of the latter is that it doesn't need to know about the Needs
  // system at all.
  if(kNeeds_ShowFaceRepairGlitches)
  {
    const f32 currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
    if(currentTime_s > _nextFaceRepair_sec)
    {
      // TODO: Perhaps tie degree to current amount of repair need instead of being random
      const f32 distortionDegree = _rng.RandDblInRange(kNeeds_FaceRepairDegreeMin, kNeeds_FaceRepairDegreeMax);
      
      ProceduralFace repairFace;
      
      AnimationStreamer::FaceTrack faceTrack;
      TimeStamp_t totalOffset = 0;
      bool moreDistortionFrames = false;
      do {
        TimeStamp_t timeInc;
        moreDistortionFrames = ScanlineDistorter::GetNextDistortionFrame(distortionDegree, repairFace, timeInc);
        totalOffset += timeInc;
        faceTrack.AddKeyFrameToBack(ProceduralFaceKeyFrame(repairFace, totalOffset));
      } while(moreDistortionFrames);
      
      AddLayer("ScanlineDistortion", std::move(faceTrack));
      
      // TODO: Perhaps tie frequency of showing this to amount of repair need instead of being random
      _nextFaceRepair_sec = currentTime_s + _rng.RandDblInRange(kNeeds_FaceRepairSpacingMin_sec,
                                                                kNeeds_FaceRepairSpacingMax_sec);
    }
  }
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FaceLayerManager::UpdateFace(ProceduralFace& procFace)
{
  if(DEBUG_FACE_LAYERING)
  {
    if(!_faceLayers.empty())
    {
      PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace.ApplyingFaceLayers",
                        "NumLayers=%lu", (unsigned long)_faceLayers.size());
    }
  }
  
  bool faceUpdated = false;
  
  std::list<Tag> tagsToErase;
  
  for(auto faceLayerIter = _faceLayers.begin(); faceLayerIter != _faceLayers.end(); ++faceLayerIter)
  {
    auto & faceLayer = faceLayerIter->second;
    
    if(DEBUG_FACE_LAYERING) {
      PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace.ApplyFaceLayer",
                        "%slayer %s with tag %d",
                        faceLayer.isPersistent ? "Persistent" : "",
                        faceLayer.name.c_str(), faceLayer.tag);
    }
    
    // Note that shouldReplace==false here because face layers do not replace
    // what's on the face, by definition, they layer on top of what's already there.
    faceUpdated |= GetFaceHelper(faceLayer.track, faceLayer.startTime_ms,
                                 faceLayer.streamTime_ms, procFace, false);
    
    faceLayer.streamTime_ms += RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
    
    if(!faceLayer.track.HasFramesLeft()) {
      
      // This layer is done...
      if(faceLayer.isPersistent) {
        if(faceLayer.track.IsEmpty()) {
          PRINT_NAMED_WARNING("AnimationStreamer.UpdateFace.EmptyPersistentLayer",
                              "Persistent face layer is empty - perhaps live frames were "
                              "used? (tag=%d)", faceLayer.tag);
          faceLayer.isPersistent = false;
        } else {
          //...but is marked persistent, so keep applying last frame
          faceLayer.track.MoveToPrevKeyFrame(); // so we're not at end() anymore
          faceLayer.streamTime_ms -= RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
          if(DEBUG_FACE_LAYERING) {
            PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace.HoldingLayer",
                              "Holding last frame of face layer %s with tag %d",
                              faceLayer.name.c_str(), faceLayer.tag);
          }
          faceLayer.sentOnce = true; // mark that it has been sent at least once
          
          // We no longer need anything but the last frame (which should now be
          // "current")
          faceLayer.track.ClearUpToCurrent();
        }
      } else {
        //...and is not persistent, so delete it
        if(DEBUG_FACE_LAYERING) {
          PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace.RemovingFaceLayer",
                            "%s, Tag = %d (Layers remaining=%lu)",
                            faceLayer.name.c_str(), faceLayer.tag, (unsigned long)_faceLayers.size()-1);
        }
        tagsToErase.push_back(faceLayerIter->first);
      }
    }
  }
  
  // Actually erase elements from the map
  for(auto tag : tagsToErase) {
    _faceLayers.erase(tag);
  }
  
  return faceUpdated;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FaceLayerManager::HaveFaceLayersToSend()
{
  if(_faceLayers.empty()) {
    return false;
  } else {
    // There are face layers, but we want to ignore any that are persistent that
    // have already been sent once
    for(auto & layer : _faceLayers) {
      if(!layer.second.isPersistent || !layer.second.sentOnce) {
        // There's at least one non-persistent face layer, or a persistent face layer
        // that has not been sent in its entirety at least once: return that there
        // are still face layers to send
        return true;
      }
    }
    // All face layers are persistent ones that have been sent, so no need to keep sending them
    // by themselves. They only need to be re-applied while there's something
    // else being sent
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 FaceLayerManager::GetMaxBlinkSpacingTimeForScreenProtection_ms() const
{
  return kMaxBlinkSpacingTimeForScreenProtection_ms;
}
  
} // namespace Cozmo
} // namespace Anki

