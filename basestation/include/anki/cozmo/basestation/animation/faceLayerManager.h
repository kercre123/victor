/**
 * File: faceLayerManager.h
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

#ifndef __Anki_Cozmo_FaceLayerManager_H__
#define __Anki_Cozmo_FaceLayerManager_H__

#include "anki/common/types.h"
#include "anki/cozmo/basestation/animation/animation.h"
#include "anki/cozmo/basestation/animations/track.h"

#include "clad/types/liveIdleAnimationParameters.h"

#include <map>

namespace Anki {
namespace Cozmo {
  
// Forward declaration
class Robot;
class ProceduralFace;
class CozmoContext;

class FaceLayerManager
{
public:
  
  using Tag = AnimationTag;
  using FaceTrack = Animations::Track<ProceduralFaceKeyFrame>;
  
  FaceLayerManager(const CozmoContext* context);
  
  Result Update(const Robot& robot);
  
  // Add a procedural face "layer" to be combined with whatever is streaming
  Result AddLayer(const std::string& name, FaceTrack&& faceTrack, TimeStamp_t delay_ms = 0);
  
  // Add a procedural face "layer" that is applied and then has its final
  // adjustment "held" until removed.
  // A handle/tag for the layer is returned, which is needed for removal.
  Tag AddPersistentLayer(const std::string& name, FaceTrack&& faceTrack);
  
  // Remove a previously-added persistent face layer using its tag.
  // If duration > 0, that amount of time will be used to transition back
  // to no adjustment
  void RemovePersistentLayer(Tag tag, s32 duration_ms = 0);
  
  // Add a keyframe to the end of an existing persistent face layer
  void AddToPersistentLayer(Tag tag, ProceduralFaceKeyFrame&& keyframe);
  
  // Remove any existing procedural eye dart created by KeepFaceAlive
  void RemoveKeepAliveEyeDart(s32 duration_ms);

  bool HaveFaceLayersToSend();
  
  bool UpdateFace(ProceduralFace& procFace);
  
  void KeepFaceAlive(Robot& robot, const std::map<LiveIdleAnimationParameter,f32>& params);
 
  size_t GetNumLayers() const { return _faceLayers.size(); }
  
  u32 GetMaxBlinkSpacingTimeForScreenProtection_ms() const;
  
  // Helper to fold the next procedural face from the given track (if one is
  // ready to play) into the passed-in procedural face params.
  static bool GetFaceHelper(Animations::Track<ProceduralFaceKeyFrame>& track,
                            TimeStamp_t startTime_ms, TimeStamp_t currTime_ms,
                            ProceduralFace& procFace,
                            bool shouldReplace);
  
private:
  
  // For layering procedural face animations on top of whatever is currently
  // playing:
  struct FaceLayer {
    FaceTrack   track;
    TimeStamp_t startTime_ms;
    TimeStamp_t streamTime_ms;
    bool        isPersistent;
    bool        sentOnce;
    Tag         tag;
    std::string name;
  };
  
  std::map<Tag, FaceLayer> _faceLayers;
  
  // KeepFaceAlive members:
  s32            _nextBlink_ms         = 0;
  s32            _nextEyeDart_ms       = 0;
  Tag            _eyeDartTag           = NotAnimatingTag;
  
  // Face Glitches
  f32 _nextFaceRepair_sec = 0.f;
  
  Util::RandomGenerator& _rng;
  
  Tag _layerTagCtr = 0;
  void IncrementLayerTagCtr();
};
  
} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_FaceLayerManager_H__ */
