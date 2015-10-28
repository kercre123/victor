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

  // For defining animations at runtime (e.g. procedural faces)
  template<class KeyFrameType>
  Result AddKeyFrame(const KeyFrameType& kf);

  Result Init(uint8_t tag);
  Result Update(Robot& robot);

  // An animation is Empty if *all* its tracks are empty
  bool IsEmpty() const;

  // An animation is finished when none of its track have frames left to play
  bool IsFinished() const;

  void Clear();

  const std::string& GetName() const { return _name; }

private:


  // Name of this animation
  std::string _name;
  bool _isInitialized;
  uint8_t _tag;
  bool _startOfAnimationSent;

  // When this animation started playing (was initialized) in milliseconds, in
  // "real" basestation time
  TimeStamp_t _startTime_ms;

  // Where we are in the animation in terms of what has been streamed out, since
  // we don't stream in real time. Each time we send an audio frame to the
  // robot (silence or actual audio), this increments by one audio sample
  // length, since that's what keeps time for streaming animations (not a
  // clock)
  TimeStamp_t _streamingTime_ms;

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

  template<class KeyFrameType>
  Animations::Track<KeyFrameType>& GetTrack();

  // TODO: Remove these once we aren't playing robot audio on the device
  TimeStamp_t _playedRobotAudio_ms;
  std::deque<const RobotAudioKeyFrame*> _onDeviceRobotAudioKeyFrameQueue;
  const RobotAudioKeyFrame* _lastPlayedOnDeviceRobotAudioKeyFrame;

  bool _endOfAnimationSent;

  ProceduralFace _proceduralFace;

  bool BufferMessageToSend(RobotInterface::EngineToRobot* msg);
  Result SendBufferedMessages(Robot& robot);

  bool AllTracksBuffered() const;
  std::list<RobotInterface::EngineToRobot*> _sendBuffer;
  s32 _numBytesToSend;

  // Send larger keyframes "hot" for reliable transport (this includes
  // audio samples and face images)
  static const bool SEND_LARGE_KEYFRAMES_HOT = false;

  // "Flow control" for not overrunning reliable transport in a single
  // update tick
  static const s32 MAX_BYTES_FOR_RELIABLE_TRANSPORT;

}; // class Animation


template<class KeyFrameType>
Result Animation::AddKeyFrame(const KeyFrameType& kf)
{
  Result addResult = GetTrack<KeyFrameType>().AddKeyFrame(kf);
  if(RESULT_OK != addResult) {
    PRINT_NAMED_ERROR("Animiation.AddKeyFrame.Failed", "");
  } else {
    // If we add a keyframe after initialization (at which time this animation
    // could have been empty), make sure to mark that we haven't yet sent
    // end of animation.
    _endOfAnimationSent = false;
  }

  return addResult;
}
    

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_ANIMATION_H
