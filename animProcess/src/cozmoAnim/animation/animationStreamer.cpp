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

#include "anki/common/basestation/array2d_impl.h"
#include "anki/common/basestation/utils/timer.h"
#include "cozmoAnim/animation/animationStreamer.h"
//#include "cozmoAnim/animation/trackLayerManagers/faceLayerManager.h"
#include "cozmoAnim/animation/cannedAnimationContainer.h"
#include "cozmoAnim/animation/proceduralFaceDrawer.h"
#include "cozmoAnim/animation/trackLayerComponent.h"
#include "cozmoAnim/audio/animationAudioClient.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/cozmoAnimContext.h"
#include "cozmoAnim/robotDataLoader.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/types/animationTypes.h"
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

#define DEBUG_ANIMATION_STREAMING 0
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
  
  AnimationStreamer::AnimationStreamer(const CozmoAnimContext* context)
  : _context(context)
  , _animationContainer(*(_context->GetDataLoader()->GetCannedAnimations()))
  , _trackLayerComponent(new TrackLayerComponent(context))
  , _lockedTracks(0)
  , _tracksInUse(0)
  , _audioClient( new Audio::AnimationAudioClient(context->GetAudioController()) )
  , _longEnoughSinceLastStreamTimeout_s(kDefaultLongEnoughSinceLastStreamTimeout_s)
  {    
  }
  
  Result AnimationStreamer::Init()
  {
    DEV_ASSERT(nullptr != _context, "AnimationStreamer.Init.NullContext");
    DEV_ASSERT(nullptr != _context->GetDataLoader(), "AnimationStreamer.Init.NullRobotDataLoader");
    DEV_ASSERT(nullptr != _context->GetDataLoader()->GetCannedAnimations(), "AnimationStreamer.Init.NullCannedAnimationsContainer");
    
    SetDefaultParams();
    
    // TODO: Restore ability to subscribe to messages here?
    //       It's currently hard to do with CPPlite messages.
    // SetupHandlers(_context->GetExternalInterface());
    
    
    // Set neutral face
    const std::string neutralFaceAnimName = "anim_neutral_eyes_01";
    _neutralFaceAnimation = _animationContainer.GetAnimation(neutralFaceAnimName);
    if (nullptr != _neutralFaceAnimation)
    {
      auto frame = _neutralFaceAnimation->GetTrack<ProceduralFaceKeyFrame>().GetFirstKeyFrame();
      ProceduralFace::SetResetData(frame->GetFace());
    }
    else
    {
      PRINT_NAMED_ERROR("AnimationStreamer.Constructor.NeutralFaceDataNotFound",
                        "Could not find expected neutral face animation file called %s",
                        neutralFaceAnimName.c_str());
    }
    
    
    // Do this after the ProceduralFace class has set to use the right neutral face
    _trackLayerComponent->Init();
    
    return RESULT_OK;
  }
  
  
  const Animation* AnimationStreamer::GetCannedAnimation(const std::string& name) const
  {
    return _animationContainer.GetAnimation(name);
  }
  
  AnimationStreamer::~AnimationStreamer()
  {
    FaceDisplay::removeInstance();
  }

  Result AnimationStreamer::SetStreamingAnimation(u32 animID, Tag tag, u32 numLoops, bool interruptRunning)
  {
    std::string animName = "";
    if (!_animationContainer.GetAnimNameByID(animID, animName)) {
      PRINT_NAMED_WARNING("AnimationStreamer.SetStreamingAnimation.InvalidAnimID", "%d", animID);
      return RESULT_FAIL;
    }
    return SetStreamingAnimation(animName, tag, numLoops, interruptRunning);
  }
  
  
  Result AnimationStreamer::SetStreamingAnimation(const std::string& name, Tag tag, u32 numLoops, bool interruptRunning)
  {
    // Special case: stop streaming the current animation
    if(name.empty()) {
      if(DEBUG_ANIMATION_STREAMING) {
        PRINT_NAMED_DEBUG("AnimationStreamer.SetStreamingAnimation.StoppingCurrent",
                          "Stopping streaming of animation '%s'.",
                          GetStreamingAnimationName().c_str());
      }

      return SetStreamingAnimation(nullptr, kNotAnimatingTag);
    }
    
    return SetStreamingAnimation(_animationContainer.GetAnimation(name), tag, numLoops, interruptRunning);
  }
  
  Result AnimationStreamer::SetStreamingAnimation(Animation* anim, Tag tag, u32 numLoops, bool interruptRunning)
  {
    if(DEBUG_ANIMATION_STREAMING && (anim != nullptr))
    {
      PRINT_NAMED_DEBUG("AnimationStreamer.SetStreamingAnimation", "Name:%s Tag:%d NumLoops:%d",
                        anim->GetName().c_str(), tag, numLoops);
    }
    
    const bool wasStreamingSomething = nullptr != _streamingAnimation;
    const bool wasIdling = nullptr != _idleAnimation;
    
    if(wasStreamingSomething)
    {
      if(nullptr != anim && !interruptRunning) {
        PRINT_NAMED_INFO("AnimationStreamer.SetStreamingAnimation.NotInterrupting",
                         "Already streaming %s, will not interrupt with %s",
                         _streamingAnimation->GetName().c_str(),
                         anim->GetName().c_str());
        return RESULT_FAIL;
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
      return RESULT_FAIL;
    }
    else
    {
      _lastPlayedAnimationId = _streamingAnimation->GetName();
    
      // Get the animation ready to play
      InitStream(_streamingAnimation, tag);
      
      _numLoops = numLoops;
      _loopCtr = 0;
      
      if(DEBUG_ANIMATION_STREAMING) {
        PRINT_NAMED_DEBUG("AnimationStreamer.SetStreamingAnimation",
                          "Will start streaming '%s' animation %d times with tag=%d.",
                          _streamingAnimation->GetName().c_str(), numLoops, tag);
      }
      
      return RESULT_OK;
    }
  }
  
  void AnimationStreamer::Abort()
  {
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
      
      if (_startOfAnimationSent) {
        SendEndOfAnimation();
      }

      EnableBackpackAnimationLayer(false);

      _audioClient->StopCozmoEvent();

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

      _endOfAnimationSent = false;
      _startOfAnimationSent = false;
      
//      // Prep sound
//      _audioClient.CreateAudioAnimation( anim );
//      _audioBufferingTime_ms = 0;
      
      // Make sure any eye dart (which is persistent) gets removed so it doesn't
      // affect the animation we are about to start streaming. Give it a little
      // duration so it doesn't pop.
      _trackLayerComponent->RemoveKeepFaceAlive(3*IKeyFrame::SAMPLE_LENGTH_MS);
    }
    return lastResult;
  }
  
  // Sends message to robot if track is unlocked and deletes it.
  //
  // TODO: Take in EngineToRobot& instead of ptr?
  //       i.e. Why doesn't KeyFrame::GetStreamMessage() return a ref?
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
            _tracksInUse |= (u8)track;
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
    Vision::ImageRGB faceImg = ProceduralFaceDrawer::DrawFace(procFace);
    BufferFaceToSend(faceImg);
  }
  
  void AnimationStreamer::BufferFaceToSend(const Vision::ImageRGB& faceImg)
  {
    ANKI_VERIFY(faceImg.GetNumCols() == FACE_DISPLAY_WIDTH &&
                faceImg.GetNumRows() == FACE_DISPLAY_HEIGHT,
                "AnimationStreamer.BufferFaceToSend.InvalidImageSize",
                "Got %d x %d. Expected %d x %d",
                faceImg.GetNumCols(), faceImg.GetNumRows(),
                FACE_DISPLAY_WIDTH, FACE_DISPLAY_HEIGHT);
    
    // Draws frame to face display
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
      PRINT_NAMED_DEBUG("AnimationStreamer.SendStartOfAnimation.BufferedStartOfAnimation", "Tag=%d, ID=%d, loopCtr=%d",
                        _tag, _streamingAnimID, _loopCtr);
    }

    if (_loopCtr == 0) {
      RobotInterface::AnimationStarted startMsg;
      startMsg.id = _streamingAnimID;
      startMsg.tag = _tag;
      if (!RobotInterface::SendMessageToEngine(startMsg)) {
        return RESULT_FAIL;
      }
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
      PRINT_NAMED_INFO("AnimationStreamer.SendEndOfAnimation.SendingToEngine", "t=%dms, loopCtr=%d, numLoops=%d",
                       _streamingTime_ms - _startTime_ms, _loopCtr, _numLoops);
    }
    
    if (_loopCtr == _numLoops - 1) {
      RobotInterface::AnimationEnded endMsg;
      endMsg.id = _streamingAnimID;
      endMsg.tag = _tag;
      endMsg.wasAborted = false;
      if (!RobotInterface::SendMessageToEngine(endMsg)) {
        return RESULT_FAIL;
      }
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
                                              false);
      
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
    auto & headTrack        = anim->GetTrack<HeadAngleKeyFrame>();
    auto & liftTrack        = anim->GetTrack<LiftHeightKeyFrame>();
    auto & bodyTrack        = anim->GetTrack<BodyMotionKeyFrame>();
    auto & recordHeadingTrack = anim->GetTrack<RecordHeadingKeyFrame>();
    auto & turnToRecordedHeadingTrack = anim->GetTrack<TurnToRecordedHeadingKeyFrame>();
    auto & eventTrack       = anim->GetTrack<EventKeyFrame>();
    auto & faceAnimTrack    = anim->GetTrack<FaceAnimationKeyFrame>();
    

    if(!_startOfAnimationSent) {
      SendStartOfAnimation();
    }
    
    // Is it time to play robot audio?
    // TODO: Do it this way or with GetCurrentStreamingMessage() like all the other tracks?
    if (robotAudioTrack.HasFramesLeft() &&
        robotAudioTrack.GetCurrentKeyFrame().IsTimeToPlay(_startTime_ms, currTime_ms))
    {
      _audioClient->PlayAudioKeyFrame( robotAudioTrack.GetCurrentKeyFrame(_context->GetRandom()) );
      robotAudioTrack.MoveToNextKeyFrame();
    }

    // Send keyframes at appropriates times
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
                                              storeFace);
      
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
      
      if(SendIfTrackUnlocked(headTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms), AnimTrackFlag::HEAD_TRACK)) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("HeadAngle");
      }
      
      if(SendIfTrackUnlocked(liftTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms), AnimTrackFlag::LIFT_TRACK)) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("LiftHeight");
      }

      // Send AnimationEvent directly up to engine if it's time to play one
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
      if(faceAnimTrack.HasFramesLeft() && !IsTrackLocked((u8)AnimTrackFlag::FACE_IMAGE_TRACK)) {
        auto & faceKeyFrame = faceAnimTrack.GetCurrentKeyFrame();
        if(faceKeyFrame.IsTimeToPlay(_streamingTime_ms - _startTime_ms))
        {
          const auto faceImgPtr = faceKeyFrame.GetFaceImage();
          if (faceImgPtr != nullptr) {
            DEBUG_STREAM_KEYFRAME_MESSAGE("FaceAnimation");
            BufferFaceToSend(*faceImgPtr);
          }
          
          if(faceKeyFrame.IsDone()) {
            faceAnimTrack.MoveToNextKeyFrame();
          }
        }
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
      _streamingTime_ms += ANIM_TIME_STEP_MS;  //RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
      
    } // while(buffering frames)
    
    // Send an end-of-animation keyframe when done
    if( !anim->HasFramesLeft() &&
        !_audioClient->HasActiveEvents() &&
        _startOfAnimationSent &&
        !_endOfAnimationSent)
    {      
//       _audioClient.ClearCurrentAnimation();
      StopTracksInUse();
      lastResult = SendEndOfAnimation();
    }
    
    return lastResult;
  } // UpdateStream()
  
  
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
    const bool haveIdleAnimation      = _idleAnimation != nullptr;
    const bool longEnoughSinceStream  = (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - _lastStreamTime) > _longEnoughSinceLastStreamTimeout_s;
    
    if(!haveStreamingAnimation &&
       haveStreamedAnything &&
       (!haveIdleAnimation && longEnoughSinceStream))
    {
      // If we were interrupted from streaming an animation and we've met all the
      // conditions to even be in this function, then we should make sure we've
      // got neutral face back on the screen
      if(_wasAnimationInterruptedWithNothing) {
        SetStreamingAnimation(_neutralFaceAnimation, kNotAnimatingTag );
        _wasAnimationInterruptedWithNothing = false;
      }
      
      _trackLayerComponent->KeepFaceAlive(_liveAnimParams);
    }
    
    if(_streamingAnimation != nullptr) {
      
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
          InitStream(_streamingAnimation, _tag);
          
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
    


    
    // If we didn't do any streaming above, but we've still got layers to stream
    if(!streamUpdated)
    {
      if(_trackLayerComponent->HaveLayersToSend())
      {
        lastResult = StreamLayers();
      }
    }
    
    // Tick audio engine
    _audioClient->Update();
    
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
  

  void AnimationStreamer::StopTracks(const u8 whichTracks)
  {
    if(whichTracks)
    {
      if(whichTracks & (u8)AnimTrackFlag::HEAD_TRACK)
      {
        RobotInterface::MoveHead msg;
        msg.speed_rad_per_sec = 0;
        RobotInterface::SendMessageToRobot(std::move(msg));
      }
      
      if(whichTracks & (u8)AnimTrackFlag::LIFT_TRACK)
      {
        RobotInterface::MoveLift msg;
        msg.speed_rad_per_sec = 0;
        RobotInterface::SendMessageToRobot(std::move(msg));
      }
      
      if(whichTracks & (u8)AnimTrackFlag::BODY_TRACK)
      {
        RobotInterface::DriveWheels msg;
        msg.lwheel_speed_mmps = 0;
        msg.rwheel_speed_mmps = 0;
        msg.lwheel_accel_mmps2 = 0;
        msg.rwheel_accel_mmps2 = 0;
        RobotInterface::SendMessageToRobot(std::move(msg));
      }
      
      _tracksInUse &= ~whichTracks;
    }
  }
  
  
} // namespace Cozmo
} // namespace Anki

