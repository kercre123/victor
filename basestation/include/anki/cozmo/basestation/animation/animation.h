/**
 * File: animation.h
 *
 * Authors: Andrew Stein
 * Created: 2015-06-25
 *
 * Description:
 *    Class for storing a single animation, which is made of
 *    tracks of keyframes. Also manages streaming those keyframes
 *    to a robot.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef ANKI_COZMO_CANNED_ANIMATION_H
#define ANKI_COZMO_CANNED_ANIMATION_H

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/keyframe.h"
#include "anki/cozmo/basestation/proceduralFace.h"
#include "anki/cozmo/basestation/animations/track.h"
#include <list>
#include <queue>

namespace Anki {
namespace Cozmo {

// Forward declaration
namespace RobotInterface {
class EngineToRobot;
enum class EngineToRobotTag : uint8_t;
}
class Robot;

class Animation
{
public:

  Animation(const std::string& name = "");

  // For reading canned animations from files
  Result DefineFromJson(const std::string& name, Json::Value& json);

  // For defining animations at runtime (e.g. live animation)
  template<class KeyFrameType>
  Result AddKeyFrame(const KeyFrameType& kf);

  // Get a track by KeyFrameType
  template<class KeyFrameType>
  Animations::Track<KeyFrameType>& GetTrack();
  
  // Calls all tracks' Init() methods
  Result Init();

  bool IsInitialized() const { return _isInitialized; }
  
  // An animation is Empty if *all* its tracks are empty
  bool IsEmpty() const;

  // True if any track has has frames left to play
  bool HasFramesLeft() const;
  
  void Clear();

  const std::string& GetName() const { return _name; }
  
private:

  // Name of this animation
  std::string _name;
  bool _isInitialized;

  // All the animation tracks, storing different kinds of KeyFrames
  Animations::Track<HeadAngleKeyFrame>      _headTrack;
  Animations::Track<LiftHeightKeyFrame>     _liftTrack;
  Animations::Track<FaceAnimationKeyFrame>  _faceAnimTrack;
  Animations::Track<ProceduralFaceKeyFrame> _proceduralFaceTrack;
  Animations::Track<FacePositionKeyFrame>   _facePosTrack;
  Animations::Track<BlinkKeyFrame>          _blinkTrack;
  Animations::Track<BackpackLightsKeyFrame> _backpackLightsTrack;
  Animations::Track<BodyMotionKeyFrame>     _bodyPosTrack;
  Animations::Track<DeviceAudioKeyFrame>    _deviceAudioTrack;
  Animations::Track<RobotAudioKeyFrame>     _robotAudioTrack;

  //ProceduralFace _proceduralFace;
  
}; // class Animation


template<class KeyFrameType>
Result Animation::AddKeyFrame(const KeyFrameType& kf)
{
  Result addResult = GetTrack<KeyFrameType>().AddKeyFrame(kf);
  if(RESULT_OK != addResult) {
    PRINT_NAMED_ERROR("Animiation.AddKeyFrame.Failed", "");
  }

  return addResult;
}
    

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_ANIMATION_H
