/**
 * File: trackLayerComponent.h
 *
 * Authors: Al Chaussee
 * Created: 06/28/2017
 *
 * Description: Component which manages creating various procedural animations by
 *              using the trackLayerManagers to generate keyframes and add them to
 *              track layers
 *              Currently there are only three trackLayerManagers face, backpack, and audio
 *
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_TrackLayerComponent_H__
#define __Anki_Cozmo_TrackLayerComponent_H__

#include "coretech/common/shared/types.h"

#include "cannedAnimLib/cannedAnims/animation.h"

#include "clad/types/keepFaceAliveParameters.h"

#include <memory>

namespace Anki {
namespace Cozmo {

class AnimContext;

class AudioLayerManager;
class BackpackLayerManager;
class FaceLayerManager;
 
class TrackLayerComponent
{
public:

  // Output struct that contains the final keyframes to
  // stream to the robot
  struct LayeredKeyFrames {
    bool haveAudioKeyFrame = false;
    RobotAudioKeyFrame audioKeyFrame;
    
    bool haveBackpackKeyFrame = false;
    BackpackLightsKeyFrame backpackKeyFrame;
    
    bool haveFaceKeyFrame = false;
    ProceduralFaceKeyFrame faceKeyFrame;
  };
  
  TrackLayerComponent(const AnimContext* context);
  ~TrackLayerComponent();
  
  void Init();
  
  void Update();
  void AdvanceTracks(const TimeStamp_t toTime_ms);
  
  // Pulls the current keyframe from various tracks of the anim
  // and combines it with any track layers that may exist
  // Outputs layeredKeyframes struct which contains the final combined
  // keyframes from the anim and the various track layers
  void ApplyLayersToAnim(Animation* anim,
                         const TimeStamp_t timeSinceAnimStart_ms,
                         LayeredKeyFrames& layeredKeyFrames,
                         bool storeFace) const;
  
  // Keep Cozmo's face alive using the params specified
  // (call each tick while the face should be kept alive)
  void KeepFaceAlive(const std::map<KeepFaceAliveParameter, f32>& params,
                     const TimeStamp_t timeSinceKeepAliveStart_ms);
  
  // Removes the live face after duration_ms has passed
  // Note: Will not cancel/remove a blink that is in progress
  void RemoveKeepFaceAlive(u32 duration_ms);
  
  // Make Cozmo blink
  void AddBlink(const TimeStamp_t timeSinceKeepAliveStart_ms);
  
  // Make Cozmo squint (will continue to squint until removed)
  void AddSquint(const std::string& name, f32 squintScaleX, f32 squintScaleY, f32 upperLidAngle);

  // Removes specified squint after duration_ms has passed
  void RemoveSquint(const std::string& name, u32 duration_ms = 0);
  
  // Either start an eye shift or update an already existing eye shift with new params
  // Note: Eye shift will continue until removed so if eye shift with the same name
  // was already added without being removed, this will just update it
  void AddOrUpdateEyeShift(const std::string& name,
                           f32 xPix,
                           f32 yPix,
                           TimeStamp_t duration_ms,
                           f32 xMax = ProceduralFace::HEIGHT,
                           f32 yMax = ProceduralFace::WIDTH,
                           f32 lookUpMaxScale = 1.1f,
                           f32 lookDownMinScale = 0.85f,
                           f32 outerEyeScaleIncrease = 0.1f);
  
  // Removes the specified eye shift after duration_ms has passed
  void RemoveEyeShift(const std::string& name, u32 duration_ms = 0);
  
  // Make Cozmo glitch
  void AddGlitch(f32 glitchDegree);
  
  // Returns true if any of the layerManagers have layers to send
  bool HaveLayersToSend() const;
  
  u32 GetMaxBlinkSpacingTimeForScreenProtection_ms() const;
  
private:

  void ApplyAudioLayersToAnim(Animation* anim,
                              const TimeStamp_t timeSinceAnimStart_ms,
                              LayeredKeyFrames& layeredKeyFrames) const;
  
  void ApplyBackpackLayersToAnim(Animation* anim,
                                 const TimeStamp_t timeSinceAnimStart_ms,
                                 LayeredKeyFrames& layeredKeyFrames) const;
  
  void ApplyFaceLayersToAnim(Animation* anim,
                             const TimeStamp_t timeSinceAnimStart_ms,
                             LayeredKeyFrames& layeredKeyFrames,
                             bool storeFace) const;
  
  std::unique_ptr<AudioLayerManager>    _audioLayerManager;
  std::unique_ptr<BackpackLayerManager> _backpackLayerManager;
  std::unique_ptr<FaceLayerManager>     _faceLayerManager;
  
  std::unique_ptr<ProceduralFace> _lastProceduralFace;
};
  
}
}

#endif
