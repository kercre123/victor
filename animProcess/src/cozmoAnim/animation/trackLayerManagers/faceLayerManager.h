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
#include "cannedAnimLib/proceduralFace/proceduralFaceModifierTypes.h"
#include "clad/types/keepFaceAliveParameters.h"
#include <map>
#include <string>


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
  static bool GetFaceHelper(Animations::Track<ProceduralFaceKeyFrame>& track,
                            TimeStamp_t startTime_ms, TimeStamp_t currTime_ms,
                            ProceduralFaceKeyFrame& procFace,
                            bool shouldReplace);
  
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
  void GenerateBlink(Animations::Track<ProceduralFaceKeyFrame>& track, BlinkEventList& out_eventList) const;
  
  
  // Generate eye blink sequence
  // Add BlinkStateEvnets to eventList for other layers to sync with
  // Return RESULT_FAIL if there is already a blink layer
  Result AddBlinkToFaceTrack(const std::string& layerName, BlinkEventList& out_eventList);
  
  // Get the next eye blink time
  s32 GetNextBlinkTime_ms(const std::map<KeepFaceAliveParameter, f32>& params) const;
  
  // Generate eye dart
  // Set eye dart interpolationTime_ms for other layers to sync with
  Result AddEyeDartToFaceTrack(const std::string& layerName,
                               const std::map<KeepFaceAliveParameter,f32>& params,
                               TimeStamp_t& out_interpolationTime_ms);
  
  // Get the next eye dart time
  s32 GetNextEyeDartTime_ms(const std::map<KeepFaceAliveParameter, f32>& params) const;
  
  // Add "alive" frames to Face Track
  void AddKeepFaceAliveTrack(const std::string& layerName);
  
  // Generates a track of all keyframes necessary to make the eyes squint
  void GenerateSquint(f32 squintScaleX, f32 squintScaleY, f32 upperLidAngle,
                      Animations::Track<ProceduralFaceKeyFrame>& track) const;
  
  // Generates a track of all keyframes necessary to make the face have distortion
  // Returns how many keyframes were generated
  u32 GenerateFaceDistortion(float distortionDegree, Animations::Track<ProceduralFaceKeyFrame>& track) const;
  
  u32 GetMaxBlinkSpacingTimeForScreenProtection_ms() const;
  
};

} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_FaceLayerManager_H__ */
