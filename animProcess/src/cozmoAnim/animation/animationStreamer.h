/**
 * File: animationStreamer.h
 *
 * Authors: Andrew Stein
 * Created: 2015-06-25
 *
 * Description: 
 * 
 *   Handles streaming a given animation from a CannedAnimationContainer
 *   to a robot.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Anki_Cozmo_AnimationStreamer_H__
#define __Anki_Cozmo_AnimationStreamer_H__

#include "anki/common/types.h"
#include "anki/vision/basestation/image.h"
#include "cozmoAnim/animation/animation.h"
#include "cozmoAnim/animation/track.h"
#include "clad/types/liveIdleAnimationParameters.h"

#include <list>
#include <memory>

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  class ProceduralFace;
  class CannedAnimationContainer;
  class CozmoAnimContext;
  class TrackLayerComponent;
  
  namespace Audio {
    class AnimationAudioClient;
  }
  
  
  class AnimationStreamer
  {
  public:
    
    using Tag = AnimationTag;
    using FaceTrack = Animations::Track<ProceduralFaceKeyFrame>;
    
    // TODO: This could be removed in favor of just referring to ::Anki::Cozmo, but avoiding touching too much code now.
    static const Tag kNotAnimatingTag = ::Anki::Cozmo::kNotAnimatingTag;
    
    AnimationStreamer(const CozmoAnimContext* context);
    
    ~AnimationStreamer();
    
    Result Init();
    
    // Sets an animation to be streamed and how many times to stream it.
    // Use numLoops = 0 to play the animation indefinitely.
    // Returns a tag you can use to monitor whether the robot is done playing this
    // animation.
    // If interruptRunning == true, any currently-streaming animation will be aborted.
    // Actual streaming occurs on calls to Update().
    Result SetStreamingAnimation(const std::string& name,
                                 Tag tag,
                                 u32 numLoops = 1,
                                 bool interruptRunning = true);
    
    Result SetStreamingAnimation(Animation* anim,
                                 Tag tag,
                                 u32 numLoops = 1,
                                 bool interruptRunning = true);
    
    Result SetStreamingAnimation(u32 animID,
                                 Tag tag,
                                 u32 numLoops = 1,
                                 bool interruptRunning = true);
    
    // If any animation is set for streaming and isn't done yet, stream it.
    Result Update();
     
    // Returns true if the idle animation is playing
    bool IsIdleAnimating() const;
    
    const std::string GetStreamingAnimationName() const;
    const Animation* GetStreamingAnimation() const { return _streamingAnimation; }
    
    const Animation* GetCannedAnimation(const std::string& name) const;
    const CannedAnimationContainer& GetCannedAnimationContainer() const { return _animationContainer; }

    void SetDefaultParams();
    
    // Overload of SetParam from base class. Mostly just calls base class method.
    void SetParam(LiveIdleAnimationParameter whichParam, float newValue);
    
    // Set/Reset the amount of time to wait before forcing KeepFaceAlive() after the last stream has stopped
    // and there is no idle animation
    void SetKeepFaceAliveLastStreamTimeout(const f32 time_s)
      { _longEnoughSinceLastStreamTimeout_s = time_s; }
    void ResetKeepFaceAliveLastStreamTimeout();
    
    TrackLayerComponent* GetTrackLayerComponent() { return _trackLayerComponent.get(); }
    const TrackLayerComponent* GetTrackLayerComponent() const { return _trackLayerComponent.get(); }

    void SetLockedTracks(u8 whichTracks)   { _lockedTracks = whichTracks; }
    bool IsTrackLocked(u8 trackFlag) const { return ((_lockedTracks & trackFlag) == trackFlag); }
    
  private:
    
    // Initialize the streaming of an animation with a given tag
    // (This will call anim->Init())
    Result InitStream(Animation* anim, Tag withTag);
    
    // Actually stream the animation (called each tick)
    Result UpdateStream(Animation* anim, bool storeFace);
    
    // This performs the test cases for the animation while loop
    bool ShouldProcessAnimationFrame( Animation* anim, TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms );
    
    Result SendStartOfAnimation();
    Result SendEndOfAnimation();
    
    // Enables/Disables the backpack lights animation layer on the robot
    // if it hasn't already been enabled/disabled
    Result EnableBackpackAnimationLayer(bool enable);
    
    // Check whether the animation is done
    bool IsFinished(Animation* anim) const;
    
    // Update generate frames needed by the "live" idle animation and add them
    // to the _idleAnimation to be streamed.
    Result UpdateLiveAnimation();
    
    // If we are currently streaming, kill it, and make sure not to leave a
    // random face displayed (stream last face keyframe)
    void Abort();
    
    
    void StopTracks(const u8 whichTracks);
    
    void StopTracksInUse() {
      // In case we are aborting an animation, stop any tracks that were in use
      // (For now, this just means motor-based tracks.) Note that we don't
      // stop tracks we weren't using, in case we were, for example, playing
      // a head animation while driving a path.
      StopTracks(_tracksInUse);
    }

    
    
    const CozmoAnimContext* _context = nullptr;
    
    // Container for all known "canned" animations (i.e. non-live)
    CannedAnimationContainer& _animationContainer;
    
    Animation*  _idleAnimation = nullptr;
    Animation*  _streamingAnimation = nullptr;
    Animation*  _neutralFaceAnimation = nullptr;

    std::string _lastPlayedAnimationId;

    u32 _streamingAnimID;
    
    std::unique_ptr<TrackLayerComponent>  _trackLayerComponent;
    
    void BufferFaceToSend(const ProceduralFace& procFace);
    void BufferFaceToSend(const Vision::ImageRGB& image);
    
    // Used to stream _just_ the stuff left in the various layers (all procedural stuff)
    Result StreamLayers();
    
    bool _isIdling = false;
    
    u32 _numLoops = 1;
    u32 _loopCtr  = 0;

    // Start and end messages sent to engine
    bool _startOfAnimationSent = false;
    bool _endOfAnimationSent   = false;
    
    bool _wasAnimationInterruptedWithNothing = false;
    
    bool _backpackAnimationLayerEnabled = false;
    
    // When this animation started playing (was initialized) in milliseconds, in
    // "real" basestation time
    TimeStamp_t _startTime_ms;
    
    // Where we are in the animation in terms of what has been streamed out, since
    // we don't stream in real time. Each time we send an audio frame to the
    // robot (silence or actual audio), this increments by one audio sample
    // length, since that's what keeps time for streaming animations (not a
    // clock)
    TimeStamp_t _streamingTime_ms;
    
    // When animation is waiting for audio, track how much time has passed so we can abort in needed
//    TimeStamp_t _audioBufferingTime_ms = 0;
    
    // Last time we streamed anything
    f32 _lastStreamTime = std::numeric_limits<f32>::lowest();

#   define PLAY_ROBOT_AUDIO_ON_DEVICE 0
#   if PLAY_ROBOT_AUDIO_ON_DEVICE
    // TODO: Remove these once we aren't playing robot audio on the device
    TimeStamp_t _playedRobotAudio_ms;
    std::deque<const RobotAudioKeyFrame*> _onDeviceRobotAudioKeyFrameQueue;
    const RobotAudioKeyFrame* _lastPlayedOnDeviceRobotAudioKeyFrame;
#   endif
    
    // Sends msg to appropriate destination as long as the specified track is unlocked
    bool SendIfTrackUnlocked(RobotInterface::EngineToRobot* msg, AnimTrackFlag track);
    
    Tag _tag;
    
    // For track locking
    u8 _lockedTracks;
    
    // Which tracks are currently playing
    u8 _tracksInUse;
    
    
    // For live animation
    std::map<LiveIdleAnimationParameter, f32> _liveAnimParams;

    s32            _bodyMoveDuration_ms  = 0;
    s32            _liftMoveDuration_ms  = 0;
    s32            _headMoveDuration_ms  = 0;
    s32            _bodyMoveSpacing_ms   = 0;
    s32            _liftMoveSpacing_ms   = 0;
    s32            _headMoveSpacing_ms   = 0;
    
    std::unique_ptr<Audio::AnimationAudioClient> _audioClient;
    
    // Time to wait before forcing KeepFaceAlive() after the latest stream has stopped and there is no
    // idle animation
    f32 _longEnoughSinceLastStreamTimeout_s;
    
    AnimationTag _liveIdleTurnEyeShiftTag = kNotAnimatingTag;

  }; // class AnimationStreamer
  
} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_AnimationStreamer_H__ */
