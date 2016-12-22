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

#ifndef ANKI_COZMO_ANIMATION_STREAMER_H
#define ANKI_COZMO_ANIMATION_STREAMER_H

#include "anki/common/types.h"
#include "anki/cozmo/basestation/animation/animation.h"
#include "anki/cozmo/basestation/animations/track.h"
#include "anki/cozmo/basestation/utils/hasSettableParameters.h"
#include "clad/types/liveIdleAnimationParameters.h"
#include "clad/externalInterface/messageGameToEngine.h"

#include <list>
#include <memory>

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  class Robot;
  class ProceduralFace;
  class CozmoContext;
  class CannedAnimationContainer;
  class AnimationGroupContainer;
  
  class IExternalInterface;
  namespace ExternalInterface {
    class MessageGameToEngine;
  }
  
  namespace Audio {
    class RobotAudioClient;
  }  
  
  class AnimationStreamer : public HasSettableParameters<LiveIdleAnimationParameter, ExternalInterface::MessageGameToEngineTag::SetLiveIdleAnimationParameters, f32>
  {
  public:
    using Tag = u8;
    static const AnimationTrigger NeutralFaceTrigger;
    
    static const Tag  NotAnimatingTag  = 0;
    static const Tag  IdleAnimationTag = 255;
    
    AnimationStreamer(const CozmoContext* context, Audio::RobotAudioClient& audioClient);
    
    ~AnimationStreamer();
    
    // Sets an animation to be streamed and how many times to stream it.
    // Use numLoops = 0 to play the animation indefinitely.
    // Returns a tag you can use to monitor whether the robot is done playing this
    // animation.
    // If interruptRunning == true, any currently-streaming animation will be aborted.
    // Actual streaming occurs on calls to Update().
    Tag SetStreamingAnimation(const std::string& name, u32 numLoops = 1, bool interruptRunning = true);
    Tag SetStreamingAnimation(Animation* anim, u32 numLoops = 1, bool interruptRunning = true);
    
    // Set the animation to be played when no other animation has been specified.  Use the empty string to
    // disable idle animation. NOTE: this wipes out any idle animation stack (from the push/pop actions below)
    Result SetIdleAnimation(AnimationTrigger animName);
    
    // Set the idle animation and also add it to the idle animation stack, so we can use pop later. The current
    // idle (even if it came from SetIdleAnimation) is always on the stack
    Result PushIdleAnimation(AnimationTrigger animName);
    
    // Return to the idle animation which was running prior to the most recent call to PushIdleAnimation.
    // Returns RESULT_OK on success and RESULT_FAIL if the stack of idle animations was empty.
    // Will not pop the last idle off the stack.
    Result PopIdleAnimation();

    const std::string& GetIdleAnimationName() const;
    
    // Add a procedural face "layer" to be combined with whatever is streaming
    using FaceTrack = Animations::Track<ProceduralFaceKeyFrame>;
    Result AddFaceLayer(const std::string& name, FaceTrack&& faceTrack, TimeStamp_t delay_ms = 0);
    
    // Add a procedural face "layer" that is applied and then has its final
    // adjustemtn "held" until removed.
    // A handle/tag for the layer i s returned, which is needed for removal.
    Tag AddPersistentFaceLayer(const std::string& name, FaceTrack&& faceTrack);
    
    // Remove a previously-added persistent face layer using its tag.
    // If duration > 0, that amount of time will be used to transition back
    // to no adjustment
    void RemovePersistentFaceLayer(Tag tag, s32 duration_ms = 0);
    
    // Add a keyframe to the end of an existing persistent face layer
    void AddToPersistentFaceLayer(Tag tag, ProceduralFaceKeyFrame&& keyframe);
    
    // Remove any existing procedural eye dart created by KeepFaceAlive
    void RemoveKeepAliveEyeDart(s32 duration_ms);
    
    // If any animation is set for streaming and isn't done yet, stream it.
    Result Update(Robot& robot);
     
    // Returns true if the idle animation is playing
    bool IsIdleAnimating() const;
    
    const std::string GetStreamingAnimationName() const;
    const Animation* GetStreamingAnimation() const { return _streamingAnimation; }
    
    const std::string& GetAnimationNameFromGroup(const std::string& name, const Robot& robot) const;
    const Animation* GetCannedAnimation(const std::string& name) const;

    // Required by HasSettableParameters:
    virtual void SetDefaultParams() override;
    
    // Overload of SetParam from base class. Mostly just calls base class method.
    void SetParam(LiveIdleAnimationParameter whichParam, float newValue);
    
    // "Flow control" for not getting too far ahead of the robot, to help prevent
    // too much delay when we want to layer something on "now". This is number of
    // audio frames.
    static const s32 NUM_AUDIO_FRAMES_LEAD;
    
    ProceduralFace* GetLastProceduralFace() { return _lastProceduralFace.get(); }

  private:
    
    // Initialize the streaming of an animation with a given tag
    // (This will call anim->Init())
    Result InitStream(Animation* anim, Tag withTag);
    
    // Actually stream the animation (called each tick)
    Result UpdateStream(Robot& robot, Animation* anim, bool storeFace);
    
    // This is performs the test cases for the animation while loop
    bool ShouldProcessAnimationFrame( Animation* anim, TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms );
    
    Result SendStartOfAnimation();
    Result SendEndOfAnimation(Robot& robot);
    
    // Stream one audio sample from the AudioManager
    // If sendSilence==true, will buffer an AudioSilence message if no audio is
    // available (needed while streaming canned animations)
    Result BufferAudioToSend(bool sendSilence, TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms);
    
    // Check whether the animation is done
    bool IsFinished(Animation* anim) const;
    
    // Update generate frames needed by the "live" idle animation and add them
    // to the _idleAnimation to be streamed.
    Result UpdateLiveAnimation(Robot& robot);
    
    void UpdateAmountToSend(Robot& robot);
    
    // If we are currently streaming, kill it, and make sure not to leave a
    // random face displayed (stream last face keyframe)
    void Abort();
    
    const CozmoContext* _context = nullptr;
    
    // Container for all known "canned" animations (i.e. non-live)
    CannedAnimationContainer& _animationContainer;
    AnimationGroupContainer&  _animationGroups;
    
    Animation*  _idleAnimation = nullptr;
    Animation*  _streamingAnimation = nullptr;
    Animation*  _neutralFaceAnimation = nullptr;
    TimeStamp_t _timeSpentIdling_ms = 0;
    std::vector<AnimationTrigger> _idleAnimationNameStack;

    std::string _lastPlayedAnimationId;
    
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
    
    // Helper to fold the next procedural face from the given track (if one is
    // ready to play) into the passed-in procedural face params.
    bool GetFaceHelper(Animations::Track<ProceduralFaceKeyFrame>& track,
                       TimeStamp_t startTime_ms, TimeStamp_t currTime_ms,
                       ProceduralFace& procFace,
                       bool shouldReplace);
    
    bool HaveFaceLayersToSend();
    
    void UpdateFace(Robot& robot, Animation* anim, bool storeFace);
    
    void BufferFaceToSend(const ProceduralFace& procFace);
    
    void KeepFaceAlive(Robot& robot);
    
    // Used to stream _just_ the stuff left in face layers or audio in the buffer
    Result StreamFaceLayers(Robot& robot);
    
    void IncrementTagCtr();
    void IncrementLayerTagCtr();
    
    bool _isIdling = false;
    
    u32 _numLoops = 1;
    u32 _loopCtr  = 0;
    Tag _tagCtr   = 0;
    Tag _layerTagCtr = 0;
    
    bool _startOfAnimationSent = false;
    bool _endOfAnimationSent   = false;
    bool _wasAnimationInterruptedWithNothing = false;
    
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
    TimeStamp_t _audioBufferingTime_ms = 0;
    
    // Last time we streamed anything
    f32 _lastStreamTime = std::numeric_limits<f32>::lowest();

#   define PLAY_ROBOT_AUDIO_ON_DEVICE 0
#   if PLAY_ROBOT_AUDIO_ON_DEVICE
    // TODO: Remove these once we aren't playing robot audio on the device
    TimeStamp_t _playedRobotAudio_ms;
    std::deque<const RobotAudioKeyFrame*> _onDeviceRobotAudioKeyFrameQueue;
    const RobotAudioKeyFrame* _lastPlayedOnDeviceRobotAudioKeyFrame;
#   endif
    
    // Manage the send buffer, which is where we put keyframe messages ready to
    // go to the robot, and parcel them out according to how many bytes the
    // reliable UDP channel and the robot's animation buffer can handle on a
    // given tick.
    bool BufferMessageToSend(RobotInterface::EngineToRobot* msg);
    Result SendBufferedMessages(Robot& robot);
    
    void ClearSendBuffer();
    
    std::list<RobotInterface::EngineToRobot*> _sendBuffer;
    s32 _numBytesToSend = 0;
    s32 _numAudioFramesToSend = 0;
    Tag _tag;
    
    // "Flow control" for not overrunning reliable transport in a single
    // update tick
    static const s32 MAX_BYTES_FOR_RELIABLE_TRANSPORT;
    
    Util::RandomGenerator& _rng;
    
    // For live animation
    Animation      _liveAnimation;
    bool           _isLiveTwitchEnabled  = false;
    s32            _nextBlink_ms         = 0;
    s32            _nextEyeDart_ms       = 0;
    Tag            _eyeDartTag           = NotAnimatingTag;
    //s32            _nextLookAround_ms    = 0;
    s32            _bodyMoveDuration_ms  = 0;
    s32            _liftMoveDuration_ms  = 0;
    s32            _headMoveDuration_ms  = 0;
    s32            _bodyMoveSpacing_ms   = 0;
    s32            _liftMoveSpacing_ms   = 0;
    s32            _headMoveSpacing_ms   = 0;
    
    Audio::RobotAudioClient& _audioClient;
    
    std::unique_ptr<ProceduralFace> _lastProceduralFace;
    
    // For handling incoming messages
    std::vector<Signal::SmartHandle> _eventHandlers;
    void SetupHandlers(IExternalInterface* externalInterface);

  }; // class AnimationStreamer


} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ANIMATION_STREAMER_H
