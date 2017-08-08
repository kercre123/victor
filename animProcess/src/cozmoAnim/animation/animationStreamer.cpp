/**
 * File: animationStreamer.cpp
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

#include "anki/common/basestation/utils/timer.h"
#include "cozmoAnim/animation/animationStreamer.h"
#include "cozmoAnim/animation/faceAnimationManager.h"
#include "cozmoAnim/animation/trackLayerManagers/faceLayerManager.h"
#include "cozmoAnim/animation/cannedAnimationContainer.h"
#include "cozmoAnim/animation/proceduralFaceDrawer.h"
//#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "cozmoAnim/animation/trackLayerComponent.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/cozmoContext.h"
#include "cozmoAnim/robotDataLoader.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/types/animationKeyFrames.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"

#include "cozmoAnim/engineMessages.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine_sendToEngine_helper.h"
#include "clad/robotInterface/messageEngineToRobot_sendToRobot_helper.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#define DEBUG_ANIMATION_STREAMING 1
#define DEBUG_ANIMATION_STREAMING_AUDIO 0

namespace Anki {
namespace Cozmo {
  
  namespace{
    
  // Default time to wait before forcing KeepFaceAlive() after the latest stream has stopped and there is no
  // idle animation
  const f32 kDefaultLongEnoughSinceLastStreamTimeout_s = 0.5f;
  
  CONSOLE_VAR(bool, kFullAnimationAbortOnAudioTimeout, "AnimationStreamer", false);
  CONSOLE_VAR(u32, kAnimationAudioAllowedBufferTime_ms, "AnimationStreamer", 250);
  } // namespace
  
//  const AnimationTrigger AnimationStreamer::NeutralFaceTrigger = AnimationTrigger::NeutralFace;
  
  
  AnimationStreamer::AnimationStreamer(const CozmoContext* context) //, Audio::RobotAudioClient& audioClient)
  : IAnimationStreamer()
  , _context(context)
  , _animationContainer(*(_context->GetDataLoader()->GetCannedAnimations()))
  , _trackLayerComponent(new TrackLayerComponent(context))
  , _rng(*_context->GetRandom())
//  , _liveAnimation(EnumToString(AnimationTrigger::ProceduralLive))
  , _liveAnimation("ProceduralLive")
//  , _audioClient( audioClient )
  , _longEnoughSinceLastStreamTimeout_s(kDefaultLongEnoughSinceLastStreamTimeout_s)
  {
    _liveAnimation.SetIsLive(true);
    
    DEV_ASSERT(nullptr != _context, "AnimationStreamer.Constructor.NullContext");
    
    SetDefaultParams();
    
//    SetupHandlers(_context->GetExternalInterface());
    
//    // Set up the neutral face to use when resetting procedural animations
//    const std::string neutralFaceAnimGroupName = _context->GetRobotManager()->GetAnimationForTrigger(NeutralFaceTrigger);
//    const AnimationGroup* group = _animationGroups.GetAnimationGroup(neutralFaceAnimGroupName);
//    if(group == nullptr || group->IsEmpty()) {
//      PRINT_NAMED_ERROR("AnimationStreamer.Constructor.BadNeutralAnimGroup",
//                        "Neutral animation group %s for trigger %s is empty or null",
//                        neutralFaceAnimGroupName.c_str(), EnumToString(NeutralFaceTrigger));
//    }
//    else
//    {
//      if(group->GetNumAnimations() > 1)
//      {
//        PRINT_NAMED_WARNING("AnimationStreamer.Constructor.MultipleNeutralFaceAnimations",
//                            "Neutral face animation group %s has %zu animations instead of one. "
//                            "Using first.", neutralFaceAnimGroupName.c_str(),
//                            group->GetNumAnimations());
//      }
//      
//      const std::string neutralFaceAnimName = group->GetFirstAnimationName();
//      _neutralFaceAnimation = _animationContainer.GetAnimation(neutralFaceAnimName);
//      if (nullptr != _neutralFaceAnimation)
//      {
//        auto frame = _neutralFaceAnimation->GetTrack<ProceduralFaceKeyFrame>().GetFirstKeyFrame();
//        ProceduralFace::SetResetData(frame->GetFace());
//      }
//      else
//      {
//        PRINT_NAMED_ERROR("AnimationStreamer.Constructor.NeutralFaceDataNotFound",
//                          "Could not find expected neutral face animation file called %s",
//                          neutralFaceAnimName.c_str());
//      }
//    }
    
    // Do this after the ProceduralFace class has set to use the right neutral face
    _trackLayerComponent->Init();
  }
  
  
  const Animation* AnimationStreamer::GetCannedAnimation(const std::string& name) const
  {
    return _animationContainer.GetAnimation(name);
  }
  
  AnimationStreamer::~AnimationStreamer()
  {
  }

  AnimationStreamer::Tag AnimationStreamer::SetStreamingAnimation(u32 animID, u32 numLoops, bool interruptRunning)
  {
    std::string animName = "";
    if (!_animationContainer.GetAnimNameByID(animID, animName)) {
      PRINT_NAMED_WARNING("AnimationStreamer.SetStreamingAnimation.InvalidAnimID", "%d", animID);
      return 0;
    }
    return SetStreamingAnimation(animName, numLoops, interruptRunning);
  }
  
  
  AnimationStreamer::Tag AnimationStreamer::SetStreamingAnimation(const std::string& name, u32 numLoops, bool interruptRunning)
  {
    // Special case: stop streaming the current animation
    if(name.empty()) {
      if(DEBUG_ANIMATION_STREAMING) {
        PRINT_NAMED_DEBUG("AnimationStreamer.SetStreamingAnimation",
                          "Stopping streaming of animation '%s'.",
                          GetStreamingAnimationName().c_str());
      }

      return SetStreamingAnimation(nullptr);
    }
    
    return SetStreamingAnimation(_animationContainer.GetAnimation(name), numLoops, interruptRunning);
  }
  
  AnimationStreamer::Tag AnimationStreamer::SetStreamingAnimation(Animation* anim, u32 numLoops, bool interruptRunning)
  {
    const bool wasStreamingSomething = nullptr != _streamingAnimation;
    const bool wasIdling = nullptr != _idleAnimation;
    
    if(wasStreamingSomething)
    {
      if(nullptr != anim && !interruptRunning) {
        PRINT_NAMED_INFO("AnimationStreamer.SetStreamingAnimation.NotInterrupting",
                         "Already streaming %s, will not interrupt with %s",
                         _streamingAnimation->GetName().c_str(),
                         anim->GetName().c_str());
        return NotAnimatingTag;
      }
      
      PRINT_NAMED_WARNING("AnimationStreamer.SetStreamingAnimation.Aborting",
                          "Animation %s is interrupting animation %s",
                          anim != nullptr ? anim->GetName().c_str() : "NULL",
                          _streamingAnimation->GetName().c_str());
    }
    
    // If there's something already streaming or we're purposefully clearing the buffer, abort
    if(wasStreamingSomething || wasIdling || nullptr == anim)
    {
      Abort();
    }
    
    _streamingAnimation = anim;
    
    if (!GetCannedAnimationContainer().GetAnimIDByName(_streamingAnimation->GetName(), _streamingAnimID)) {
      PRINT_NAMED_WARNING("AnimationStreamer.SetStreamingAnimation.AnimIDNotFound", "%s", _streamingAnimation->GetName().c_str());
      _streamingAnimID = 0;
    }
    
    if(_streamingAnimation == nullptr) {
      // Set flag if we are interrupting a streaming animation with nothing.
      // If we get to KeepFaceAlive with this flag set, we'll stream neutral face for safety.
      if(wasStreamingSomething) {
        _wasAnimationInterruptedWithNothing |= true;
      }
      return NotAnimatingTag;
    }
    else
    {
      _lastPlayedAnimationId = _streamingAnimation->GetName();
      
      IncrementTagCtr();
    
      // Get the animation ready to play
      InitStream(_streamingAnimation, _tagCtr);
      
      _numLoops = numLoops;
      _loopCtr = 0;
      
      if(DEBUG_ANIMATION_STREAMING) {
        PRINT_NAMED_DEBUG("AnimationStreamer.SetStreamingAnimation",
                          "Will start streaming '%s' animation %d times with tag=%d.",
                          _streamingAnimation->GetName().c_str(), numLoops, _tagCtr);
      }
      
      return _tagCtr;
    }
  }

  void AnimationStreamer::IncrementTagCtr()
  {
    // Increment the tag counter and keep it from being one of the "special"
    // values used to indicate "not animating" or "idle animation" or any existing
    // tag in use
    ++_tagCtr;
    while( (_tagCtr == NotAnimatingTag) || (_tagCtr == IdleAnimationTag) )
    {
      ++_tagCtr;
    }
  }
  
  
  void AnimationStreamer::Abort()
  {
    if (_tag != NotAnimatingTag) {
//      using namespace ExternalInterface;
//      _context->GetExternalInterface()->Broadcast(MessageEngineToGame(AnimationAborted(_tag)));
    }
    
    if (nullptr != _streamingAnimation || nullptr != _idleAnimation)
    {
      // Log streamer state for diagnostics
      const auto * anim = (_streamingAnimation != nullptr ? _streamingAnimation : _idleAnimation);
      const char * type = (_streamingAnimation != nullptr ? "Streaming" : "Idle");
      
      PRINT_NAMED_INFO("AnimationStreamer.Abort",
                       //"Tag=%d %s:%s hasFramesLeft=%d audioComplete=%d startSent=%d endSent=%d sendBuffer=%s",
                       "Tag=%d %s:%s hasFramesLeft=%d startSent=%d endSent=%d",
                       _tag,
                       type,
                       anim->GetName().c_str(),
                       anim->HasFramesLeft(),
//                       _audioClient.AnimationIsComplete(),
                       _startOfAnimationSent,
                       _endOfAnimationSent);
      
      // Reset streamer state
      _startOfAnimationSent = false;
      _endOfAnimationSent = false;
      EnableBackpackAnimationLayer(false);

//      if (_audioClient.HasAnimation()) {
//        _audioClient.GetCurrentAnimation()->AbortAnimation();
//      }
//      _audioClient.ClearCurrentAnimation();
    }
  } // Abort()

  
  
  Result AnimationStreamer::InitStream(Animation* anim, Tag withTag)
  {
    Result lastResult = anim->Init();
    if(lastResult == RESULT_OK)
    {
      // Switch interlacing for both systems that create images to put on the face.
      // Since animations are relatively short, and InitStream gets called for each
      // new animation or each loop of the same animation, this guarantees we will
      // change scanlines periodically to avoid burn-in. Note that KeepFaceAlive
      // uses a procedural blink, which also switches the scanlines.
//      ProceduralFaceDrawer::SwitchInterlacing();
//      FaceAnimationManager::SwitchInterlacing();
      
      _tag = withTag;
      
      _startTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      
      // Initialize "fake" streaming time to the same start time so we can compare
      // to it for determining when its time to stream out a keyframe
      _streamingTime_ms = _startTime_ms;

      
      // If this is an empty (e.g. live) animation, there is no need to
      // send end of animation keyframe until we actually send a keyframe
      _endOfAnimationSent = anim->IsEmpty();
      _startOfAnimationSent = false;
      
//      // Prep sound
//      _audioClient.CreateAudioAnimation( anim );
//      _audioBufferingTime_ms = 0;
      
      // Make sure any eye dart (which is persistent) gets removed so it doesn't
      // affect the animation we are about to start streaming. Give it a little
      // duration so it doesn't pop.
      // (Special case: allow KeepAlive to play on top of the "Live" idle.)
      if(anim != &_liveAnimation) {
        _trackLayerComponent->RemoveKeepFaceAlive(3*IKeyFrame::SAMPLE_LENGTH_MS);
      }
    }
    return lastResult;
  }
  
  // TODO: Take in EngineToRobot& instead of ptr?
  bool AnimationStreamer::SendIfTrackUnlocked(RobotInterface::EngineToRobot* msg, AnimTrackFlag track)
  {
    bool res = false;
    if(msg != nullptr) {
      if (!IsTrackLocked((u8)track)) {
        switch(track) {
          case AnimTrackFlag::HEAD_TRACK:
          case AnimTrackFlag::LIFT_TRACK:
          case AnimTrackFlag::BODY_TRACK:
          case AnimTrackFlag::BACKPACK_LIGHTS_TRACK:
            res = Messages::SendToRobot(*msg);
            break;
          default:
            // Audio, face, and event frames are handled separately since they don't actually result in a EngineToRobot message
            PRINT_NAMED_WARNING("AnimationStreamer.SendIfTrackUnlocked.InvalidTrack", "%s", EnumToString(track));
            break;
        }
      }
      delete msg;
    }
    return res;
  }
  
  void AnimationStreamer::SetParam(LiveIdleAnimationParameter whichParam, float newValue)
  {
    if(LiveIdleAnimationParameter::BlinkSpacingMaxTime_ms == whichParam)
    {
      const auto maxSpacing_ms = _trackLayerComponent->GetMaxBlinkSpacingTimeForScreenProtection_ms();
      if( newValue > maxSpacing_ms)
      {
        PRINT_NAMED_WARNING("AnimationStreamer.SetParam.MaxBlinkSpacingTooLong",
                            "Clamping max blink spacing to %dms to avoid screen burn-in",
                            maxSpacing_ms);
        
        newValue = maxSpacing_ms;
      }
    }
    _liveAnimParams[whichParam] = newValue;
  }
  
  
  void AnimationStreamer::BufferFaceToSend(const ProceduralFace& procFace)
  {
//    AnimKeyFrame::FaceImage faceImageMsg;
//    Result rleResult = FaceAnimationManager::CompressRLE(ProceduralFaceDrawer::DrawFace(procFace), faceImageMsg.image);
//    
//    if(RESULT_OK != rleResult) {
//      PRINT_NAMED_ERROR("ProceduralFaceKeyFrame.GetStreamMessageHelper",
//                        "Failed to get RLE frame from procedural face.");
//    } else {
//      if(DEBUG_ANIMATION_STREAMING) {
//        PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace",
//                          "Streaming ProceduralFaceKeyFrame at t=%dms.",
//                          _streamingTime_ms - _startTime_ms);
//      }
//      BufferMessageToSend(new RobotInterface::EngineToRobot(std::move(faceImageMsg)));
//    }


    Vision::ImageRGB faceImg = ProceduralFaceDrawer::DrawFace(procFace);
    
    // Draws frame to face display
    // TODO: Currently resizing from V1 dimensions to V2 dimensions, but should eventually generate the face
    //       directly to V2 dimensions the first time.
    faceImg.Resize(FaceDisplay::FACE_DISPLAY_HEIGHT, FaceDisplay::FACE_DISPLAY_WIDTH);
    cv::Mat img565;
    cv::cvtColor(faceImg.get_CvMat_(), img565, cv::COLOR_RGB2BGR565);
    FaceDisplay::getInstance()->FaceDraw(reinterpret_cast<u16*>(img565.ptr()));
  }
  
  Result AnimationStreamer::EnableBackpackAnimationLayer(bool enable)
  {
    RobotInterface::BackpackSetLayer msg;
    
    if (enable && !_backpackAnimationLayerEnabled) {
      msg.layer = 1; // 1 == BPL_ANIMATION
      _backpackAnimationLayerEnabled = true;
    } else if (!enable && _backpackAnimationLayerEnabled) {
      msg.layer = 0; // 1 == BPL_USER
      _backpackAnimationLayerEnabled = false;
    } else {
      // Do nothing
      return RESULT_OK;
    }

    if (!RobotInterface::SendMessageToRobot(msg)) {
      return RESULT_FAIL;
    }

    return RESULT_OK;
  }
  
  Result AnimationStreamer::SendStartOfAnimation()
  {
    if(DEBUG_ANIMATION_STREAMING) {
      PRINT_NAMED_DEBUG("AnimationStreamer.SendStartOfAnimation.BufferedStartOfAnimation", "Tag=%d", _tag);
    }
    
    RobotInterface::AnimationStarted startMsg;
    startMsg.id = _streamingAnimID;
    startMsg.tag = _tag;
    if (!RobotInterface::SendMessageToEngine(startMsg)) {
      return RESULT_FAIL;
    }
    
    _startOfAnimationSent = true;
    _endOfAnimationSent = false;
    return RESULT_OK;
  }
  
  // TODO: Is this actually being called at the right time?
  //       Need to call this after triggerTime+durationTime of last keyframe has expired.
  Result AnimationStreamer::SendEndOfAnimation()
  {
    Result lastResult = RESULT_OK;
    
    DEV_ASSERT(_startOfAnimationSent,
               "Should not be sending end of animation without having first sent start of animation.");
    
    if(DEBUG_ANIMATION_STREAMING) {
      PRINT_NAMED_INFO("AnimationStreamer.SendEndOfAnimation", "Streaming EndOfAnimation at t=%dms.",
                       _streamingTime_ms - _startTime_ms);
    }
    
    RobotInterface::AnimationEnded endMsg;
    endMsg.id = _streamingAnimID;
    endMsg.tag = _tag;
    if (!RobotInterface::SendMessageToEngine(endMsg)) {
      return RESULT_FAIL;
    }
    
    _endOfAnimationSent = true;
    _startOfAnimationSent = false;
    
    // Every time we end an animation we should also re-enable BPL_USER layer on robot
    EnableBackpackAnimationLayer(false);
    
    return lastResult;
  } // SendEndOfAnimation()


  Result AnimationStreamer::StreamLayers()
  {
    Result lastResult = RESULT_OK;
    
    // Add more stuff to send buffer from various layers
    while(_trackLayerComponent->HaveLayersToSend())
    {
      // We don't have an animation but we still have procedural layers to so
      // apply them
      TrackLayerComponent::LayeredKeyFrames layeredKeyFrames;
      _trackLayerComponent->ApplyLayersToAnim(nullptr,
                                              _startTime_ms,
                                              _streamingTime_ms,
                                              layeredKeyFrames,
                                              false,
                                              nullptr);
      
//      // If we have audio to send
//      if(layeredKeyFrames.haveAudioKeyFrame)
//      {
//        BufferMessageToSend(new RobotInterface::EngineToRobot(std::move(layeredKeyFrames.audioKeyFrame)));
//      }
//      // Otherwise we don't have an audio keyframe so send silence
//      else
//      {
//        BufferMessageToSend(new RobotInterface::EngineToRobot(AnimKeyFrame::AudioSilence()));
//      }
      
//      // If we haven't sent the start of animation yet do so now (after audio)
//      if(!_startOfAnimationSent)
//      {
//        IncrementTagCtr();
//        _tag = _tagCtr;
//        SendStartOfAnimation();
//      }  
      
      // If we have backpack keyframes to send
      if(layeredKeyFrames.haveBackpackKeyFrame)
      {
        if (SendIfTrackUnlocked(layeredKeyFrames.backpackKeyFrame.GetStreamMessage(), AnimTrackFlag::BACKPACK_LIGHTS_TRACK)) {
          EnableBackpackAnimationLayer(true);
        }
      }
      
      // If we have face keyframes to send
      if(layeredKeyFrames.haveFaceKeyFrame)
      {
        BufferFaceToSend(layeredKeyFrames.faceKeyFrame.GetFace());
      }
      
      // Increment fake "streaming" time, so we can evaluate below whether
      // it's time to stream out any of the other tracks. Note that it is still
      // relative to the same start time.
      _streamingTime_ms += RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
    }
    
//    // If we just finished buffering all the layers, send an end of animation message
//    if(_startOfAnimationSent &&
//       !_trackLayerComponent->HaveLayersToSend() &&
//       !_endOfAnimationSent) {
//      lastResult = SendEndOfAnimation();
//    }
    
    return lastResult;
  }// StreamLayers()
  
  
  Result AnimationStreamer::UpdateStream(Animation* anim, bool storeFace)
  {
    Result lastResult = RESULT_OK;
    
    if(!anim->IsInitialized()) {
      PRINT_NAMED_ERROR("Animation.Update", "%s: Animation must be initialized before it can be played/updated.",
                        anim != nullptr ? anim->GetName().c_str() : "<NULL>");
      return RESULT_FAIL;
    }
    
    const TimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    
    // Grab references to some of the tracks
    // Some tracks are managed by the TrackLayerComponent
    auto & robotAudioTrack  = anim->GetTrack<RobotAudioKeyFrame>();
    auto & deviceAudioTrack = anim->GetTrack<DeviceAudioKeyFrame>();
    auto & headTrack        = anim->GetTrack<HeadAngleKeyFrame>();
    auto & liftTrack        = anim->GetTrack<LiftHeightKeyFrame>();
    auto & bodyTrack        = anim->GetTrack<BodyMotionKeyFrame>();
    auto & recordHeadingTrack = anim->GetTrack<RecordHeadingKeyFrame>();
    auto & turnToRecordedHeadingTrack = anim->GetTrack<TurnToRecordedHeadingKeyFrame>();
    auto & eventTrack       = anim->GetTrack<EventKeyFrame>();
    auto & faceAnimTrack    = anim->GetTrack<FaceAnimationKeyFrame>();
    
    // Is it time to play robot audio?
    // TODO: Do it this way or with GetCurrentStreamingMessage() like all the other tracks?
    if (robotAudioTrack.HasFramesLeft() &&
        robotAudioTrack.GetCurrentKeyFrame().IsTimeToPlay(_startTime_ms, currTime_ms))
    {
      // TODO: Play via wwise
      robotAudioTrack.GetCurrentKeyFrame();
      robotAudioTrack.MoveToNextKeyFrame();
    }
    
    // Is it time to play device audio? (using actual basestation time)
    if(deviceAudioTrack.HasFramesLeft() &&
       deviceAudioTrack.GetCurrentKeyFrame().IsTimeToPlay(_startTime_ms, currTime_ms))
    {
       deviceAudioTrack.GetCurrentKeyFrame().PlayOnDevice();
       deviceAudioTrack.MoveToNextKeyFrame();
    }
    
    
    // Add more stuff to the send buffer. Note that we are not counting individual
    // keyframes here, but instead _audio_ keyframes (with which we will buffer
    // any co-timed keyframes from other tracks).
    
    //while ( ShouldProcessAnimationFrame(anim, _startTime_ms, _streamingTime_ms) )
    if (anim->HasFramesLeft())
    {
      
      if(DEBUG_ANIMATION_STREAMING) {
        // Very verbose!
        //PRINT_NAMED_INFO("Animation.Update", "%d bytes left to send this Update.",
        //                 numBytesToSend);
      }
      
      // Apply any track layers to the animation
      TrackLayerComponent::LayeredKeyFrames layeredKeyFrames;
      _trackLayerComponent->ApplyLayersToAnim(anim,
                                              _startTime_ms,
                                              _streamingTime_ms,
                                              layeredKeyFrames,
                                              storeFace,
//                                              (haveAnimAudio ? &rawAudio : nullptr));
                                              nullptr);
      
//      // Audio keyframe always goes first
//      if(layeredKeyFrames.haveAudioKeyFrame)
//      {
//        BufferMessageToSend(new RobotInterface::EngineToRobot(std::move(layeredKeyFrames.audioKeyFrame)));
//      }
//      else
//      {
//        BufferMessageToSend(new RobotInterface::EngineToRobot(AnimKeyFrame::AudioSilence()));
//      }
      
      //
      // We are guaranteed to have sent some kind of audio frame at this point.
      // Now send any other frames that are ready, so they will be timed with
      // that audio frame (silent or not).
      //
      
#     if DEBUG_ANIMATION_STREAMING
#       define DEBUG_STREAM_KEYFRAME_MESSAGE(__KF_NAME__) \
                  PRINT_NAMED_INFO("AnimationStreamer.UpdateStream", \
                                   "Streaming %sKeyFrame at t=%dms.", __KF_NAME__, \
                                   _streamingTime_ms - _startTime_ms)
#     else
#       define DEBUG_STREAM_KEYFRAME_MESSAGE(__KF_NAME__)
#     endif
        
      // Note that start of animation message is also sent _after_ audio keyframe,
      // to keep things consistent in how the robot's AnimationController expects
      // to receive things
      if(!_startOfAnimationSent) {
        SendStartOfAnimation();
      }
      
      if(SendIfTrackUnlocked(headTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms), AnimTrackFlag::HEAD_TRACK)) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("HeadAngle");
      }
      
      if(SendIfTrackUnlocked(liftTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms), AnimTrackFlag::LIFT_TRACK)) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("LiftHeight");
      }
      
//      // TODO: This can just send message directly up to engine
//      if(SendIfTrackUnlocked(eventTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms), AnimTrackFlag::EVENT_TRACK)) {
//        DEBUG_STREAM_KEYFRAME_MESSAGE("Event");
//      }

      if (eventTrack.HasFramesLeft() &&
          eventTrack.GetCurrentKeyFrame().IsTimeToPlay(_startTime_ms, currTime_ms))
      {
        DEBUG_STREAM_KEYFRAME_MESSAGE("Event");
        
        // Get keyframe and send contents to engine
        auto eventKeyFrame = eventTrack.GetCurrentKeyFrame();

        RobotInterface::AnimationEvent eventMsg;
        eventMsg.event_id = eventKeyFrame.GetAnimEvent();
        eventMsg.timestamp = currTime_ms;
        eventMsg.tag = _tag;
        RobotInterface::SendMessageToEngine(eventMsg);

        eventTrack.MoveToNextKeyFrame();
      }
      
      // Non-procedural faces (raw pixel data/images) take precedence over procedural faces (parameterized faces
      // like idles, keep alive, or normal animated faces)
      if(SendIfTrackUnlocked(faceAnimTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms), AnimTrackFlag::FACE_IMAGE_TRACK)) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("FaceAnimation");
      }
      else if(!faceAnimTrack.HasFramesLeft() && layeredKeyFrames.haveFaceKeyFrame)
      {
        BufferFaceToSend(layeredKeyFrames.faceKeyFrame.GetFace());
      }
      
      if(layeredKeyFrames.haveBackpackKeyFrame)
      {
        if (SendIfTrackUnlocked(layeredKeyFrames.backpackKeyFrame.GetStreamMessage(), AnimTrackFlag::BACKPACK_LIGHTS_TRACK)) {
          EnableBackpackAnimationLayer(true);
        }
      }
      
      if(SendIfTrackUnlocked(bodyTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms), AnimTrackFlag::BODY_TRACK)) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("BodyMotion");
      }
      
      if(SendIfTrackUnlocked(recordHeadingTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms), AnimTrackFlag::BODY_TRACK)) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("RecordHeading");
      }

      if(SendIfTrackUnlocked(turnToRecordedHeadingTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms), AnimTrackFlag::BODY_TRACK)) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("TurnToRecordedHeading");
      }
      
#     undef DEBUG_STREAM_KEYFRAME_MESSAGE
      
      
      // Increment fake "streaming" time, so we can evaluate below whether
      // it's time to stream out any of the other tracks. Note that it is still
      // relative to the same start time.
      _streamingTime_ms += ANIM_TIME_STEP;  //RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
      
    } // while(buffering frames)
    
    // Send an end-of-animation keyframe when done
    if( !anim->HasFramesLeft() &&
//        _audioClient.AnimationIsComplete() &&
        _startOfAnimationSent &&
        !_endOfAnimationSent)
    {      
//       _audioClient.ClearCurrentAnimation();
      lastResult = SendEndOfAnimation();
    }
    
    return lastResult;
  } // UpdateStream()
  
  
  bool AnimationStreamer::ShouldProcessAnimationFrame( Animation* anim, TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms )
  {
    bool result = false;
    /*
    if ( _sendBuffer.empty() ) {
      
      // There are animation frames, but no audio to play
      if ( anim->HasFramesLeft() && !_audioClient.HasAnimation() ) {
        result = true;
      }
      // There is audio to play
      else if ( _audioClient.HasAnimation() ) {
        // Update the RobotAudioAnimation object
        _audioClient.GetCurrentAnimation()->Update(startTime_ms, streamingTime_ms);
        // Check if audio is ready to proceed.
        result = _audioClient.UpdateAnimationIsReady( startTime_ms, streamingTime_ms );
        
        // If audio takes too long abort animation
        if ( !result ) {
          // Watch for Audio time outs
          
          const auto state = _audioClient.GetCurrentAnimation()->GetAnimationState();
          if ( state == Audio::RobotAudioAnimation::AnimationState::Preparing ) {
            // Don't start timer until the Audio Animation has started posting audio events
            return false;
          }
          
          if ( _audioBufferingTime_ms == 0 ) {
            _audioBufferingTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
          }
          else {
            if ( (BaseStationTimer::getInstance()->GetCurrentTimeStamp() - _audioBufferingTime_ms) > kAnimationAudioAllowedBufferTime_ms ) {
              PRINT_NAMED_WARNING("AnimationStreamer.ShouldProcessAnimationFrame",
                                  "Abort animation '%s' timed out after %d ms, audio event @ %d, buffer State %s",
                                  anim->GetName().c_str(),
                                  (BaseStationTimer::getInstance()->GetCurrentTimeStamp() - _audioBufferingTime_ms),
                                  (streamingTime_ms - startTime_ms),
                                  Audio::RobotAudioAnimation::GetStringForAnimationState( _audioClient.GetCurrentAnimation()->GetAnimationState() ).c_str() );
              
              if (kFullAnimationAbortOnAudioTimeout) {
                // Abort the entire animation
                Abort();
              }
              else {
                // Abort only the animation audio
                _audioClient.GetCurrentAnimation()->AbortAnimation();
                _audioClient.ClearCurrentAnimation();
              }
            }
          }
          
          if (DEBUG_ANIMATION_STREAMING_AUDIO) {
            PRINT_NAMED_INFO("AnimationStreamer.ShouldProcessAnimationFrame",
                             "Audio Animation Is NOT Ready | buffering time: %d ms",
                             (BaseStationTimer::getInstance()->GetCurrentTimeStamp() - _audioBufferingTime_ms));
          }
        }
        else {
          // Audio is streaming
          _audioBufferingTime_ms = 0;

          if (DEBUG_ANIMATION_STREAMING_AUDIO) {
            PRINT_NAMED_INFO("AnimationStreamer.ShouldProcessAnimationFrame",
                             "Audio Animation IS Ready");
          }
        }
      }
    }
    */
    return result;
  }

  
  Result AnimationStreamer::Update()
  {
    ANKI_CPU_PROFILE("AnimationStreamer::Update");
    
    Result lastResult = RESULT_OK;
    
    bool streamUpdated = false;
    
    // TODO: Send this data up to engine via some new CozmoAnimState message
    /*
    const CozmoContext* cozmoContext = robot.GetContext();
    VizManager* vizManager = robot.GetContext()->GetVizManager();
    DEV_ASSERT(nullptr != vizManager, "Expecting a non-null VizManager");

    // Update name in viz:
    if(nullptr == _streamingAnimation && nullptr == _idleAnimation)
    {
      vizManager->SetText(VizManager::ANIMATION_NAME, NamedColors::WHITE, "Anim: <none>");
      cozmoContext->SetSdkStatus(SdkStatusType::Anim, "None");
    } else if(nullptr != _streamingAnimation) {
      vizManager->SetText(VizManager::ANIMATION_NAME, NamedColors::WHITE, "Anim: %s",
                          _streamingAnimation->GetName().c_str());
      cozmoContext->SetSdkStatus(SdkStatusType::Anim, std::string(_streamingAnimation->GetName()));
    } else if(nullptr != _idleAnimation) {
      vizManager->SetText(VizManager::ANIMATION_NAME, NamedColors::WHITE, "Anim[Idle]: %s",
                          _idleAnimation->GetName().c_str());
      cozmoContext->SetSdkStatus(SdkStatusType::Anim, std::string("Idle:") + _idleAnimation->GetName());
    }
     */
    
    _trackLayerComponent->Update();
    
    // Always keep face alive, unless we have a streaming animation, since we rely on it
    // to do all face updating and we don't want to step on it's hand-designed toes.
    // Wait a 1/2 second before running after we finish the last streaming animation
    // to help reduce stepping on the next animation's toes when we have things
    // sequenced.
    // NOTE: lastStreamTime>0 check so that we don't start keeping face alive before
    //       first animation of any kind is sent.
    const bool haveStreamingAnimation = _streamingAnimation != nullptr;
    const bool haveStreamedAnything   = _lastStreamTime > 0.f;
    const bool usingLiveIdle          = _idleAnimation == &_liveAnimation;
    const bool haveIdleAnimation      = _idleAnimation != nullptr;
    const bool longEnoughSinceStream  = (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - _lastStreamTime) > _longEnoughSinceLastStreamTimeout_s;
    
    if(!haveStreamingAnimation &&
       haveStreamedAnything &&
       (usingLiveIdle || (!haveIdleAnimation && longEnoughSinceStream)))
    {
      // If we were interrupted from streaming an animation and we've met all the
      // conditions to even be in this function, then we should make sure we've
      // got neutral face back on the screen
      if(_wasAnimationInterruptedWithNothing) {
        SetStreamingAnimation(_neutralFaceAnimation);
        _wasAnimationInterruptedWithNothing = false;
      }
      
      _trackLayerComponent->KeepFaceAlive(_liveAnimParams);
    }
    
    if(_streamingAnimation != nullptr) {
      _timeSpentIdling_ms = 0;
      
      if(IsFinished(_streamingAnimation)) {
        
        ++_loopCtr;
        
        if(_numLoops == 0 || _loopCtr < _numLoops) {
         if(DEBUG_ANIMATION_STREAMING) {
           PRINT_NAMED_INFO("AnimationStreamer.Update.Looping",
                            "Finished loop %d of %d of '%s' animation. Restarting.",
                            _loopCtr, _numLoops,
                            _streamingAnimation->GetName().c_str());
         }
          
          // Reset the animation so it can be played again:
          InitStream(_streamingAnimation, _tagCtr);
          
          // To avoid streaming faceLayers set true and start streaming animation next Update() tick.
          streamUpdated = true;
        }
        else {
          if(DEBUG_ANIMATION_STREAMING) {
            PRINT_NAMED_INFO("AnimationStreamer.Update.FinishedStreaming",
                             "Finished streaming '%s' animation.",
                             _streamingAnimation->GetName().c_str());
          }
          
          _streamingAnimation = nullptr;
        }
        
      }
      else {
        // We do want to store this face to the robot since it's coming from an actual animation
        lastResult = UpdateStream(_streamingAnimation, true);
        _isIdling = false;
        streamUpdated = true;
        _lastStreamTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      }
    } // if(_streamingAnimation != nullptr)
    
    else if (_isLiveTwitchEnabled) {
      
      // If not streaming animation is set, do live animation
      _idleAnimation = &_liveAnimation;

      // Update the live animation's keyframes
      lastResult = UpdateLiveAnimation();
      if(RESULT_OK != lastResult) {
        PRINT_NAMED_ERROR("AnimationStreamer.Update.LiveUpdateFailed",
                          "Failed updating live animation from current robot state.");
        return lastResult;
      }
      
      if(!_isIdling || IsFinished(_idleAnimation)) { // re-check because isIdling could have
        if(DEBUG_ANIMATION_STREAMING) {
          PRINT_NAMED_INFO("AnimationStreamer.Update.IdleAnimInit",
                           "(Re-)Initializing idle animation: '%s'.",
                           _idleAnimation->GetName().c_str());
        }
        
        // Just finished playing a loop, or we weren't just idling. Either way,
        // (re-)init the animation so it can be played (again)
        InitStream(_idleAnimation, IdleAnimationTag);
        _isIdling = true;
        
        // To avoid streaming faceLayers set true and start streaming idle animation next Update() tick.
        streamUpdated = true;
      }
      else {
        // This is just an idle animation, so we don't want to save the face to the robot
        lastResult = UpdateStream(_idleAnimation, false);
        streamUpdated = true;
        _lastStreamTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      }
      _timeSpentIdling_ms += ANIM_TIME_STEP;
    }

    
    // If we didn't do any streaming above, but we've still got layers to stream
    if(!streamUpdated)
    {
      if(_trackLayerComponent->HaveLayersToSend())
      {
        lastResult = StreamLayers();
      }
    }
    
    return lastResult;
  } // AnimationStreamer::Update()
  
  
  
  void AnimationStreamer::SetDefaultParams()
  {

#   define SET_DEFAULT(__NAME__, __VALUE__) \
    SetParam(LiveIdleAnimationParameter::__NAME__,  static_cast<f32>(__VALUE__))
    
    SET_DEFAULT(BlinkSpacingMinTime_ms, 3000);
    SET_DEFAULT(BlinkSpacingMaxTime_ms, 4000);
    SET_DEFAULT(TimeBeforeWiggleMotions_ms, 1000);
    SET_DEFAULT(BodyMovementSpacingMin_ms, 100);
    SET_DEFAULT(BodyMovementSpacingMax_ms, 1000);
    SET_DEFAULT(BodyMovementDurationMin_ms, 250);
    SET_DEFAULT(BodyMovementDurationMax_ms, 1500);
    SET_DEFAULT(BodyMovementSpeedMinMax_mmps, 10);
    SET_DEFAULT(BodyMovementStraightFraction, 0.5f);
    SET_DEFAULT(LiftMovementDurationMin_ms, 50);
    SET_DEFAULT(LiftMovementDurationMax_ms, 500);
    SET_DEFAULT(LiftMovementSpacingMin_ms,  250);
    SET_DEFAULT(LiftMovementSpacingMax_ms, 2000);
    SET_DEFAULT(LiftHeightMean_mm, 35);
    SET_DEFAULT(LiftHeightVariability_mm, 8);
    SET_DEFAULT(HeadMovementDurationMin_ms, 50);
    SET_DEFAULT(HeadMovementDurationMax_ms, 500);
    SET_DEFAULT(HeadMovementSpacingMin_ms, 250);
    SET_DEFAULT(HeadMovementSpacingMax_ms, 1000);
    SET_DEFAULT(HeadAngleVariability_deg, 6);
    SET_DEFAULT(EyeDartSpacingMinTime_ms, 250);
    SET_DEFAULT(EyeDartSpacingMaxTime_ms, 1000);
    SET_DEFAULT(EyeDartMaxDistance_pix, 6);
    SET_DEFAULT(EyeDartMinScale, 0.92f);
    SET_DEFAULT(EyeDartMaxScale, 1.08f);
    SET_DEFAULT(EyeDartMinDuration_ms, 50);
    SET_DEFAULT(EyeDartMaxDuration_ms, 200);
    SET_DEFAULT(EyeDartOuterEyeScaleIncrease, 0.1f);
    SET_DEFAULT(EyeDartUpMaxScale, 1.1f);
    SET_DEFAULT(EyeDartDownMinScale, 0.85f);
    
#   undef SET_DEFAULT

  } // SetDefaultParams()
  
  Result AnimationStreamer::UpdateLiveAnimation()
  {
#   define GET_PARAM(__TYPE__, __NAME__) (static_cast<__TYPE__>(_liveAnimParams[LiveIdleAnimationParameter::__NAME__]))
    
    Result lastResult = RESULT_OK;
   
    // Don't start wiggling until we've been idling for a bit and make sure we
    // are not picking or placing
    if(_isLiveTwitchEnabled &&
       _timeSpentIdling_ms >= GET_PARAM(s32, TimeBeforeWiggleMotions_ms)
       // && !robot.GetDockingComponent().IsPickingOrPlacing()   // TODO: If picking and place, need to lock these tracks
       )
    {
      // If wheels are available, add a little random movement to keep Cozmo looking alive
      const bool wheelsAvailable = !IsTrackLocked((u8)AnimTrackFlag::BODY_TRACK);
      const bool timeToMoveBody = (_bodyMoveDuration_ms+_bodyMoveSpacing_ms) <= 0;
      if(wheelsAvailable && timeToMoveBody)
      {
        _bodyMoveDuration_ms = _rng.RandIntInRange(GET_PARAM(s32, BodyMovementDurationMin_ms),
                                                   GET_PARAM(s32, BodyMovementDurationMax_ms));
        s16 speed = _rng.RandIntInRange(-GET_PARAM(s32, BodyMovementSpeedMinMax_mmps),
                                         GET_PARAM(s32, BodyMovementSpeedMinMax_mmps));

        // Drive straight sometimes, turn in place the rest of the time
        s16 curvature = std::numeric_limits<s16>::max(); // drive straight
        if(_rng.RandDblInRange(0., 1.) > GET_PARAM(f32, BodyMovementStraightFraction)) {
          curvature = 0;
          
          // If turning in place, look in the direction of the turn
          const s32 x = (speed < 0 ? -1.f : 1.f) * _rng.RandIntInRange(0, ProceduralFace::WIDTH/6);
          const s32 y = _rng.RandIntInRange(-ProceduralFace::HEIGHT/6, ProceduralFace::HEIGHT/6);
          if(DEBUG_ANIMATION_STREAMING) {
            PRINT_NAMED_DEBUG("AnimationStreamer.UpdateLiveAnimation.EyeLeadTurn",
                              "Point turn eye shift (%d,%d)", x, y);
          }
          
          _trackLayerComponent->AddOrUpdateEyeShift(_liveIdleTurnEyeShiftTag,
                                                    "LiveIdleTurn",
                                                    x, y,
                                                    IKeyFrame::SAMPLE_LENGTH_MS,
                                                    ProceduralFace::WIDTH/2, ProceduralFace::HEIGHT/2);
        }
        else if(_liveIdleTurnEyeShiftTag != NotAnimatingTag)
        {
          _trackLayerComponent->RemoveEyeShift(_liveIdleTurnEyeShiftTag);
          _liveIdleTurnEyeShiftTag = NotAnimatingTag;
        }
        
        if(DEBUG_ANIMATION_STREAMING) {
          PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.BodyTwitch",
                           "Speed=%d, curvature=%d, duration=%d",
                           speed, curvature, _bodyMoveDuration_ms);
        }
        
        if(RESULT_OK != _liveAnimation.AddKeyFrameToBack(BodyMotionKeyFrame(speed, curvature, _bodyMoveDuration_ms))) {
          PRINT_NAMED_ERROR("AnimationStreamer.UpdateLiveAnimation.AddBodyMotionKeyFrameFailed", "");
          return RESULT_FAIL;
        }
        
        _bodyMoveSpacing_ms = _rng.RandIntInRange(GET_PARAM(s32, BodyMovementSpacingMin_ms),
                                                  GET_PARAM(s32, BodyMovementSpacingMax_ms));
        
      } else {
        _bodyMoveDuration_ms -= ANIM_TIME_STEP;
      }
      
      // If lift is available, add a little random movement to keep Cozmo looking alive
      const bool liftIsAvailable = !IsTrackLocked((u8)AnimTrackFlag::LIFT_TRACK);
      const bool timeToMoveLIft = (_liftMoveDuration_ms + _liftMoveSpacing_ms) <= 0;
      if(liftIsAvailable && timeToMoveLIft) //  && !robot.GetCarryingComponent().IsCarryingObject())   // TODO Engine should lock lift track after pickup
      {
        _liftMoveDuration_ms = _rng.RandIntInRange(GET_PARAM(s32, LiftMovementDurationMin_ms),
                                                   GET_PARAM(s32, LiftMovementDurationMax_ms));
        
        if(DEBUG_ANIMATION_STREAMING) {
          PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.LiftTwitch",
                           "duration=%d", _liftMoveDuration_ms);
        }
        LiftHeightKeyFrame kf(GET_PARAM(u8, LiftHeightMean_mm),
                              GET_PARAM(u8, LiftHeightVariability_mm),
                              _liftMoveDuration_ms);
        if(RESULT_OK != _liveAnimation.AddKeyFrameToBack(kf)) {
          PRINT_NAMED_ERROR("AnimationStreamer.UpdateLiveAnimation.AddLiftHeightKeyFrameFailed", "");
          return RESULT_FAIL;
        }
        
        _liftMoveSpacing_ms = _rng.RandIntInRange(GET_PARAM(s32, LiftMovementSpacingMin_ms),
                                                  GET_PARAM(s32, LiftMovementSpacingMax_ms));
        
      } else {
        _liftMoveDuration_ms -= ANIM_TIME_STEP;
      }
      
      // If head is available, add a little random movement to keep Cozmo looking alive
      const bool headIsAvailable = !IsTrackLocked((u8)AnimTrackFlag::HEAD_TRACK);
      const bool timeToMoveHead = (_headMoveDuration_ms+_headMoveSpacing_ms) <= 0;
      if(headIsAvailable && timeToMoveHead)
      {
        _headMoveDuration_ms = _rng.RandIntInRange(GET_PARAM(s32, HeadMovementDurationMin_ms),
                                                   GET_PARAM(s32, HeadMovementDurationMax_ms));
        const s8 currentAngle_deg = 10; // static_cast<s8>(RAD_TO_DEG(robot.GetHeadAngle()));
                                        // TODO: Hard coded for now, but should we actually maintain some robot state here for this?

        if(DEBUG_ANIMATION_STREAMING) {
          PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.HeadTwitch",
                           "duration=%d", _headMoveDuration_ms);
        }
        HeadAngleKeyFrame kf(currentAngle_deg, GET_PARAM(u8, HeadAngleVariability_deg), _headMoveDuration_ms);
        if(RESULT_OK != _liveAnimation.AddKeyFrameToBack(kf)) {
          PRINT_NAMED_ERROR("AnimationStreamer.UpdateLiveAnimation.AddHeadAngleKeyFrameFailed", "");
          return RESULT_FAIL;
        }
        
        _headMoveSpacing_ms = _rng.RandIntInRange(GET_PARAM(s32, HeadMovementSpacingMin_ms),
                                                  GET_PARAM(s32, HeadMovementSpacingMax_ms));
        
      } else {
        _headMoveDuration_ms -= ANIM_TIME_STEP;
      }
      
    } // if(_isLiveTwitchEnabled && _timeSpentIdling_ms >= kTimeBeforeWiggleMotions_ms)
    
    return lastResult;
#   undef GET_PARAM
     
  } // UpdateLiveAnimation()
  
  bool AnimationStreamer::IsIdleAnimating() const
  {
    return _isIdling;
  }

  const std::string AnimationStreamer::GetStreamingAnimationName() const
  {
    return _streamingAnimation ? _streamingAnimation->GetName() : "";
  }
  
  bool AnimationStreamer::IsFinished(Animation* anim) const
  {
    return _endOfAnimationSent && !anim->HasFramesLeft();
  }

  void AnimationStreamer::ResetKeepFaceAliveLastStreamTimeout()
  {
    _longEnoughSinceLastStreamTimeout_s = kDefaultLongEnoughSinceLastStreamTimeout_s;
  }
  
} // namespace Cozmo
} // namespace Anki

