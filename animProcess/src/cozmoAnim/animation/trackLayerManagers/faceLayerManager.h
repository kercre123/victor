/**
 * File: faceLayerManager.h
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

#ifndef __Anki_Cozmo_FaceLayerManager_H__
#define __Anki_Cozmo_FaceLayerManager_H__

#include "cozmoAnim/animation/trackLayerManagers/iTrackLayerManager.h"

#include "clad/types/keepFaceAliveParameters.h"

#include <map>

namespace Anki {
namespace Cozmo {

// Forward declaration
class ProceduralFace;

class FaceLayerManager : public ITrackLayerManager<ProceduralFaceKeyFrame>
{
public:
  
  using FaceTrack = Animations::Track<ProceduralFaceKeyFrame>;
  
  FaceLayerManager(const Util::RandomGenerator& rng);
  
  // Helper to fold the next procedural face from the given track (if one is
  // ready to play) into the passed-in procedural face params.
  bool GetFaceHelper(const Animations::Track<ProceduralFaceKeyFrame>& track,
                     TimeStamp_t timeSinceAnimStart_ms,
                     ProceduralFaceKeyFrame& procFace,
                     bool shouldReplace) const;
  
  // Generates and adds keyframes to various layers to keep the face "alive"
  void KeepFaceAlive(const std::map<KeepFaceAliveParameter,f32>& params,
                     const TimeStamp_t timeSinceKeepAliveStart_ms);
  
  // Remove all keep face alive layers
  void RemoveKeepFaceAlive(u32 duration_ms);
  
  // Eye shifts keyframes are generated with a relative start time - they should then
  // be updated to reflect their true playback time within a track
  
  // Generates a single keyframe with shifted eyes according to the arguments
  void GenerateEyeShift(f32 xPix, f32 yPix,
                        f32 xMax, f32 yMax,
                        f32 lookUpMaxScale,
                        f32 lookDownMinScale,
                        f32 outerEyeScaleIncrease,
                        TimeStamp_t duration_ms,
                        ProceduralFaceKeyFrame& frame) const;
  
  // Generates a single keyframe with shifted eyes according to the passed in params
  void GenerateEyeShift(const std::map<KeepFaceAliveParameter,f32>& params,
                        ProceduralFaceKeyFrame& frame) const;
  
  // Generates a track of all keyframes necessary to make the eyes blink
  void GenerateBlink(Animations::Track<ProceduralFaceKeyFrame>& track,
                     const TimeStamp_t timeSinceKeepAliveStart_ms) const;
  
  // Generates a track of all keyframes necessary to make the eyes squint
  void GenerateSquint(f32 squintScaleX, f32 squintScaleY, f32 upperLidAngle,
                      Animations::Track<ProceduralFaceKeyFrame>& track) const;
  
  // Generates a track of all keyframes necessary to make the face have distortion
  // Returns how many keyframes were generated
  u32 GenerateFaceDistortion(float distortionDegree, Animations::Track<ProceduralFaceKeyFrame>& track) const;
  
  u32 GetMaxBlinkSpacingTimeForScreenProtection_ms() const;
  
private:
  
  // KeepFaceAlive members:
  s32          _nextBlink_ms         = 0;
  s32          _nextEyeDart_ms       = 0;
  
};

} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_FaceLayerManager_H__ */
