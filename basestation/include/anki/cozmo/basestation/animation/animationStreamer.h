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
  class CannedAnimationContainer;
  class CozmoContext;
  class AnimationGroupContainer;
  class FaceLayerManager;
  
  class IExternalInterface;
  namespace ExternalInterface {
    class MessageGameToEngine;
  }
  
  namespace Audio {
    class RobotAudioClient;
  }  
  
  //
  // IAnimationStreamer declares an abstract interface common to all implementations of an animation
  // streamer component. All methods are pure virtual and must be implemented by the interface provider.
  //  
  class IAnimationStreamer :
    public HasSettableParameters<LiveIdleAnimationParameter, ExternalInterface::MessageGameToEngineTag::SetLiveIdleAnimationParameters, f32>
  {
  public:
    
    using Tag = AnimationTag;
    using FaceTrack = Animations::Track<ProceduralFaceKeyFrame>;
    
    IAnimationStreamer(IExternalInterface * interface) :
      HasSettableParameters(interface)
    {
    }
    
    // Sets an animation to be streamed and how many times to stream it.
    // Use numLoops = 0 to play the animation indefinitely.
    // Returns a tag you can use to monitor whether the robot is done playing this
    // animation.
    // If interruptRunning == true, any currently-streaming animation will be aborted.
    // Actual streaming occurs on calls to Update().
    virtual Tag SetStreamingAnimation(Animation* anim, u32 numLoops = 1, bool interruptRunning = true) = 0;

    // Set the animation to be played when no other animation has been specified.  Use the empty string to
    // disable idle animation. NOTE: this wipes out any idle animation stack (from the push/pop actions below)
    virtual Result SetIdleAnimation(AnimationTrigger animName) = 0;

    // Set the idle animation and also add it to the idle animation stack, so we can use pop later. The current
    // idle (even if it came from SetIdleAnimation) is always on the stack
    virtual Result PushIdleAnimation(AnimationTrigger animName) = 0;
    
    // Return to the idle animation which was running prior to the most recent call to PushIdleAnimation.
    // Returns RESULT_OK on success and RESULT_FAIL if the stack of idle animations was empty.
    // Will not pop the last idle off the stack.
    virtual Result PopIdleAnimation() = 0;
    
    // Add a procedural face "layer" to be combined with whatever is streaming
    virtual Result AddFaceLayer(const std::string& name, FaceTrack&& faceTrack, TimeStamp_t delay_ms = 0) = 0;

    // Add a procedural face "layer" that is applied and then has its final
    // adjustment "held" until removed.
    // A handle/tag for the layer is returned, which is needed for removal.
    virtual Tag AddPersistentFaceLayer(const std::string& name, FaceTrack&& faceTrack) = 0;
    
    // Remove a previously-added persistent face layer using its tag.
    // If duration > 0, that amount of time will be used to transition back
    // to no adjustment
    virtual void RemovePersistentFaceLayer(Tag tag, s32 duration_ms = 0) = 0;

    // Add a keyframe to the end of an existing persistent face layer
    virtual void AddToPersistentFaceLayer(Tag tag, ProceduralFaceKeyFrame&& keyframe) = 0;

    // Remove any existing procedural eye dart created by KeepFaceAlive
    virtual void RemoveKeepAliveEyeDart(s32 duration_ms) = 0;
    
    // If any animation is set for streaming and isn't done yet, stream it.
    virtual Result Update(Robot& robot) = 0;
    
    virtual const Animation* GetStreamingAnimation() const = 0;
    
    virtual const std::string& GetAnimationNameFromGroup(const std::string& name, const Robot& robot) const = 0;
    
    // Set/Reset the amount of time to wait before forcing KeepFaceAlive() after the last stream has stopped
    // and there is no idle animation
    virtual void SetKeepFaceAliveLastStreamTimeout(const f32 time_s) = 0;
    virtual void ResetKeepFaceAliveLastStreamTimeout() = 0;
    
  };
  
  class AnimationStreamer : public IAnimationStreamer
  {
  public:
    
    static const AnimationTrigger NeutralFaceTrigger;
    
    // TODO: This could be removed in favor of just referring to ::Anki::Cozmo, but avoiding touching too much code now.
    static const Tag NotAnimatingTag = ::Anki::Cozmo::NotAnimatingTag;
    
    AnimationStreamer(const CozmoContext* context, Audio::RobotAudioClient& audioClient);
    
    ~AnimationStreamer();
    
    // Sets an animation to be streamed and how many times to stream it.
    // Use numLoops = 0 to play the animation indefinitely.
    // Returns a tag you can use to monitor whether the robot is done playing this
    // animation.
    // If interruptRunning == true, any currently-streaming animation will be aborted.
    // Actual streaming occurs on calls to Update().
    Tag SetStreamingAnimation(const std::string& name, u32 numLoops = 1, bool interruptRunning = true);
    Tag SetStreamingAnimation(Animation* anim, u32 numLoops = 1, bool interruptRunning = true) override;
    
    // Set the animation to be played when no other animation has been specified.  Use the empty string to
    // disable idle animation. NOTE: this wipes out any idle animation stack (from the push/pop actions below)
    Result SetIdleAnimation(AnimationTrigger animName) override;
    
    // Set the idle animation and also add it to the idle animation stack, so we can use pop later. The current
    // idle (even if it came from SetIdleAnimation) is always on the stack
    Result PushIdleAnimation(AnimationTrigger animName) override;
    
    // Return to the idle animation which was running prior to the most recent call to PushIdleAnimation.
    // Returns RESULT_OK on success and RESULT_FAIL if the stack of idle animations was empty.
    // Will not pop the last idle off the stack.
    Result PopIdleAnimation() override;

    const std::string& GetIdleAnimationName() const;
    
    // Add a procedural face "layer" to be combined with whatever is streaming
    Result AddFaceLayer(const std::string& name, FaceTrack&& faceTrack, TimeStamp_t delay_ms = 0) override;
    
    // Add a procedural face "layer" that is applied and then has its final
    // adjustment "held" until removed.
    // A handle/tag for the layer is returned, which is needed for removal.
    Tag AddPersistentFaceLayer(const std::string& name, FaceTrack&& faceTrack) override;
    
    // Remove a previously-added persistent face layer using its tag.
    // If duration > 0, that amount of time will be used to transition back
    // to no adjustment
    void RemovePersistentFaceLayer(Tag tag, s32 duration_ms = 0) override;
    
    // Add a keyframe to the end of an existing persistent face layer
    void AddToPersistentFaceLayer(Tag tag, ProceduralFaceKeyFrame&& keyframe) override;
    
    // Remove any existing procedural eye dart created by KeepFaceAlive
    void RemoveKeepAliveEyeDart(s32 duration_ms) override;
    
    // If any animation is set for streaming and isn't done yet, stream it.
    Result Update(Robot& robot) override;
     
    // Returns true if the idle animation is playing
    bool IsIdleAnimating() const;
    
    const std::string GetStreamingAnimationName() const;
    const Animation* GetStreamingAnimation() const override { return _streamingAnimation; }
    
    const std::string& GetAnimationNameFromGroup(const std::string& name, const Robot& robot) const override;
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
    
    // Set/Reset the amount of time to wait before forcing KeepFaceAlive() after the last stream has stopped
    // and there is no idle animation
    void SetKeepFaceAliveLastStreamTimeout(const f32 time_s) override
      { _longEnoughSinceLastStreamTimeout_s = time_s; }
    void ResetKeepFaceAliveLastStreamTimeout() override;

  private:
    
    // Initialize the streaming of an animation with a given tag
    // (This will call anim->Init())
    Result InitStream(Animation* anim, Tag withTag);
    
    // Actually stream the animation (called each tick)
    Result UpdateStream(Robot& robot, Animation* anim, bool storeFace);
    
    // This performs the test cases for the animation while loop
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
    
    std::unique_ptr<FaceLayerManager> _faceLayerMgr;
    
    void UpdateFace(Robot& robot, Animation* anim, bool storeFace);
    
    void BufferFaceToSend(const ProceduralFace& procFace);
    
    // Used to stream _just_ the stuff left in face layers or audio in the buffer
    Result StreamFaceLayers(Robot& robot);
    
    void IncrementTagCtr();
    
    bool _isIdling = false;
    
    u32 _numLoops = 1;
    u32 _loopCtr  = 0;
    Tag _tagCtr   = 0;
    
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
    
    // Time to wait before forcing KeepFaceAlive() after the latest stream has stopped and there is no
    // idle animation
    f32 _longEnoughSinceLastStreamTimeout_s;

  }; // class AnimationStreamer
  
} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_AnimationStreamer_H__ */
