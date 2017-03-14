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
#include "anki/cozmo/basestation/animation/animationStreamer.h"
#include "anki/cozmo/basestation/animationContainers/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/animationGroup/animationGroupContainer.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/proceduralFaceDrawer.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/utils/hasSettableParameters_impl.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/animationKeyFrames.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"

#define DEBUG_ANIMATION_STREAMING 0
#define DEBUG_ANIMATION_STREAMING_AUDIO 0

namespace Anki {
namespace Cozmo {
  
  const AnimationTrigger AnimationStreamer::NeutralFaceTrigger = AnimationTrigger::NeutralFace;

  const s32 AnimationStreamer::MAX_BYTES_FOR_RELIABLE_TRANSPORT = (1000/2) * BS_TIME_STEP; // Don't send more than 1000 bytes every 2ms

  // This is roughly (2 x ExpectedOneWayLatency_ms + BasestationTick_ms) / AudioSampleLength_ms
  const s32 AnimationStreamer::NUM_AUDIO_FRAMES_LEAD = std::ceil((2.f * 200.f + BS_TIME_STEP) / static_cast<f32>(IKeyFrame::SAMPLE_LENGTH_MS));
  
  CONSOLE_VAR(bool, kFullAnimationAbortOnAudioTimeout, "AnimationStreamer", false);
  CONSOLE_VAR(u32, kAnimationAudioAllowedBufferTime_ms, "AnimationStreamer", 250);
  CONSOLE_VAR(f32, kMaxBlinkSpacingTimeForScreenProtection_ms, "AnimationStreamer", 30000);
  
  AnimationStreamer::AnimationStreamer(const CozmoContext* context,
                                       Audio::RobotAudioClient& audioClient)
  : HasSettableParameters(context->GetExternalInterface())
  , _context(context)
  , _animationContainer(_context->GetRobotManager()->GetCannedAnimations())
  , _animationGroups(_context->GetRobotManager()->GetAnimationGroups())
  , _rng(*_context->GetRandom())
  , _liveAnimation(EnumToString(AnimationTrigger::ProceduralLive))
  , _audioClient( audioClient )
  , _lastProceduralFace(new ProceduralFace())
  {
    _liveAnimation.SetIsLive(true);
    
    // TODO: Provide a default idle animation via Json config? COZMO-1662
    // For now, default the idle to be doing nothing, since that what it used to be.
    _idleAnimationNameStack.push_back(AnimationTrigger::Count);
    
    DEV_ASSERT(nullptr != _context, "AnimationStreamer.Constructor.NullContext");
    
    SetupHandlers(_context->GetExternalInterface());
    
    // Set up the neutral face to use when resetting procedural animations
    const std::string neutralFaceAnimGroupName = _context->GetRobotManager()->GetAnimationForTrigger(NeutralFaceTrigger);
    const AnimationGroup* group = _animationGroups.GetAnimationGroup(neutralFaceAnimGroupName);
    if(group == nullptr || group->IsEmpty()) {
      PRINT_NAMED_ERROR("AnimationStreamer.Constructor.BadNeutralAnimGroup",
                        "Neutral animation group %s for trigger %s is empty or null",
                        neutralFaceAnimGroupName.c_str(), EnumToString(NeutralFaceTrigger));
    }
    else
    {
      if(group->GetNumAnimations() > 1)
      {
        PRINT_NAMED_WARNING("AnimationStreamer.Constructor.MultipleNeutralFaceAnimations",
                            "Neutral face animation group %s has %zu animations instead of one. "
                            "Using first.", neutralFaceAnimGroupName.c_str(),
                            group->GetNumAnimations());
      }
      
      const std::string neutralFaceAnimName = group->GetFirstAnimationName();
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
    }
    
    // Do this after the ProceduralFace class has set to use the right neutral face
    _lastProceduralFace->Reset();
  }
  
  void AnimationStreamer::SetupHandlers(IExternalInterface* extIntFace)
  {
    if(nullptr == extIntFace) {
      return;
    }
      
    using namespace ExternalInterface;
    using GameToEngineEvent = AnkiEvent<MessageGameToEngine>;
    
    _eventHandlers.push_back(extIntFace->Subscribe(MessageGameToEngineTag::SetIdleAnimation,
                                                   [this](const GameToEngineEvent& msg) {
                                                     SetIdleAnimation(msg.GetData().Get_SetIdleAnimation().animTrigger);
                                                   }));
    
    _eventHandlers.push_back(extIntFace->Subscribe(MessageGameToEngineTag::PushIdleAnimation,
                                                   [this](const GameToEngineEvent& msg) {
                                                     PushIdleAnimation(msg.GetData().Get_PushIdleAnimation().animTrigger);
                                                   }));
    
    _eventHandlers.push_back(extIntFace->Subscribe(MessageGameToEngineTag::PopIdleAnimation,
                                                   [this](const GameToEngineEvent&) {
                                                     PopIdleAnimation();
                                                   }));
    
    _eventHandlers.push_back(extIntFace->Subscribe(MessageGameToEngineTag::ReplayLastAnimation,
                                                   [this](const GameToEngineEvent& msg) {
                                                     SetStreamingAnimation(_lastPlayedAnimationId,
                                                                           msg.GetData().Get_ReplayLastAnimation().numLoops);
                                                   }));

  } // SetupHandlers()

  const std::string& AnimationStreamer::GetAnimationNameFromGroup(const std::string& name, const Robot& robot) const 
  {
    const AnimationGroup* group = _animationGroups.GetAnimationGroup(name);
    if(group != nullptr && !group->IsEmpty()) {
      return group->GetAnimationName(robot.GetMoodManager(), _animationGroups, robot.GetHeadAngle());
    }
    static const std::string empty("");
    return empty;
  }
  
  const Animation* AnimationStreamer::GetCannedAnimation(const std::string& name) const
  {
    return _animationContainer.GetAnimation(name);
  }
  
  AnimationStreamer::~AnimationStreamer()
  {
    ClearSendBuffer();
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
#if ANKI_DEV_CHEATS
    _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::DebugAnimationString>(anim == nullptr ? "NONE" : anim->GetName());
#endif
    
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
    while(_tagCtr == AnimationStreamer::NotAnimatingTag ||
          _tagCtr == AnimationStreamer::IdleAnimationTag)
    {
      ++_tagCtr;
    }
  }
  
  void AnimationStreamer::IncrementLayerTagCtr()
  {
    // Increment the tag counter and keep it from being the "special"
    // value used to indicate "not animating" or any existing
    // layer tag in use
    ++_layerTagCtr;
    while(_layerTagCtr == AnimationStreamer::NotAnimatingTag ||
          _faceLayers.find(_layerTagCtr) != _faceLayers.end())
    {
      ++_layerTagCtr;
    }
  }
  
  // Private helper to construct string of unsent message tags
  static std::string ToString(const std::list<RobotInterface::EngineToRobot*> sendBuffer)
  {
    std::string s;
    for (const auto * item : sendBuffer)
    {
      if (item != sendBuffer.front()) {
        s.append(",");
      }
      s.append(EngineToRobotTagToString(item->GetTag()));
    }
    return s;
  }
  
  void AnimationStreamer::Abort()
  {
    if (_tag != NotAnimatingTag) {
      using namespace ExternalInterface;
      _context->GetExternalInterface()->Broadcast(MessageEngineToGame(AnimationAborted(_tag)));
    }
    
    if (nullptr != _streamingAnimation || nullptr != _idleAnimation)
    {
      // Log streamer state for diagnostics
      const auto * anim = (_streamingAnimation != nullptr ? _streamingAnimation : _idleAnimation);
      const char * type = (_streamingAnimation != nullptr ? "Streaming" : "Idle");
      
      PRINT_NAMED_INFO("AnimationStreamer.Abort",
                       "Tag=%d %s:%s hasFramesLeft=%d audioComplete=%d startSent=%d endSent=%d sendBuffer=%s",
                       _tag,
                       type,
                       anim->GetName().c_str(),
                       anim->HasFramesLeft(),
                       _audioClient.AnimationIsComplete(),
                       _startOfAnimationSent,
                       _endOfAnimationSent,
                       ToString(_sendBuffer).c_str());
      
      // Reset streamer state
      _startOfAnimationSent = false;
      _endOfAnimationSent = false;

      if (_audioClient.HasAnimation()) {
        _audioClient.GetCurrentAnimation()->AbortAnimation();
      }
      _audioClient.ClearCurrentAnimation();
    }
  } // Abort()
  
  
  Result AnimationStreamer::SetIdleAnimation(AnimationTrigger name)
  {
    if(!_idleAnimationNameStack.empty()) {
      PRINT_NAMED_INFO("AnimationStreamer.SetIdleAnimation.ClearingIdleStack",
                       "Had %zu idles in the stack",
                       _idleAnimationNameStack.size());
      _idleAnimationNameStack.clear();
    }

    return PushIdleAnimation(name);
    
  } // SetIdleAnimation()
  
  Result AnimationStreamer::PushIdleAnimation(AnimationTrigger name)
  {
    if(name == AnimationTrigger::Count) {
      if(DEBUG_ANIMATION_STREAMING) {
        PRINT_NAMED_DEBUG("AnimationStreamer.PushIdleAnimation.Disabling",
                          "Disabling idle animation.");
      }
      _idleAnimation = nullptr;
      _isIdling = false;
    }
    _idleAnimationNameStack.push_back(name);
    
    if(DEBUG_ANIMATION_STREAMING) {
      PRINT_NAMED_DEBUG("AnimationStreamer.PushIdleAnimation",
                        "Setting idle animation to '%s'.",
                        EnumToString(name));
    }
    
    return RESULT_OK;
  } // PushIdleAnimation()
  
  
  Result AnimationStreamer::PopIdleAnimation()
  {
    if(_idleAnimationNameStack.size() == 1) {
      PRINT_NAMED_INFO("AnimationStreamer.PopIdleAnimation.WillNotPopLast",
                       "Refusing to pop last idle animation '%s'",
                       EnumToString(_idleAnimationNameStack.back()));
      return RESULT_OK;
    }
    
    if( _idleAnimationNameStack.empty() ) {
      // This shouldn't really happen because we generally want something always
      // in the idle stack
      PRINT_NAMED_WARNING("AnimationStreamer.PopIdleAnimation.NoStack",
                          "Trying to pop an idle animation, but the stack is empty");
      return RESULT_FAIL;
    }

    _idleAnimationNameStack.pop_back();
    
    DEV_ASSERT(!_idleAnimationNameStack.empty(), "AnimationStreamer.PopIdleAnimation.EmptyIdleStack");
    
    // If what's left on the stack is "no idle", make sure to make idle null and isIdling = false:
    if(_idleAnimationNameStack.back() == AnimationTrigger::Count) {
      // If we were just idling and there's nothing streaming, we don't want to leave
      // the face in a weird spot, so stream the neutral face, just in case
      if(_idleAnimation != nullptr && _streamingAnimation == nullptr) {
        // Send the neutral face
        SetStreamingAnimation(_neutralFaceAnimation);
      }

      _idleAnimation = nullptr;
      _isIdling = false;
    }
    
    return RESULT_OK;
  }

  const std::string& AnimationStreamer::GetIdleAnimationName() const
  {
    if( nullptr == _idleAnimation ) {
      static const std::string empty("");
      return empty;
    }

    return _idleAnimation->GetName();
  }
  
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
      ProceduralFaceDrawer::SwitchInterlacing();
      FaceAnimationManager::SwitchInterlacing();
      
      _tag = withTag;
      
      _startTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      
      // Initialize "fake" streaming time to the same start time so we can compare
      // to it for determining when its time to stream out a keyframe
      _streamingTime_ms = _startTime_ms;
      
      if(!_sendBuffer.empty()) {
        PRINT_NAMED_WARNING("Animation.Init.SendBufferNotEmpty",
                            "Expecting SendBuffer to be empty. Will clear.");
        ClearSendBuffer();
      }
      
      // If this is an empty (e.g. live) animation, there is no need to
      // send end of animation keyframe until we actually send a keyframe
      _endOfAnimationSent = anim->IsEmpty();
      _startOfAnimationSent = false;
      
      // Prep sound
      _audioClient.CreateAudioAnimation( anim );
      _audioBufferingTime_ms = 0;
      
      // Make sure any eye dart (which is persistent) gets removed so it doesn't
      // affect the animation we are about to start streaming. Give it a little
      // duration so it doesn't pop.
      // (Special case: allow KeepAlive to play on top of the "Live" idle.)
      if(anim != &_liveAnimation) {
        RemoveKeepAliveEyeDart(3*IKeyFrame::SAMPLE_LENGTH_MS);
      }
    }
    return lastResult;
  }
  
  
  Result AnimationStreamer::AddFaceLayer(const std::string& name, FaceTrack&& faceTrack, TimeStamp_t delay_ms)
  {
    Result lastResult = RESULT_OK;
    
    faceTrack.SetIsLive(true);
    
    IncrementLayerTagCtr();
    
    FaceLayer newLayer;
    newLayer.tag = _layerTagCtr;
    newLayer.track = faceTrack; // COPY the track in
    newLayer.track.Init();
    newLayer.startTime_ms = delay_ms;
    newLayer.streamTime_ms = 0;
    newLayer.isPersistent = false;
    newLayer.sentOnce = false;
    newLayer.name = name;
    
    if(DEBUG_ANIMATION_STREAMING) {
      PRINT_NAMED_DEBUG("AnimationStreamer.AddFaceLayer",
                        "%s, Tag = %d (Total layers=%lu)",
                        newLayer.name.c_str(), newLayer.tag, (unsigned long)_faceLayers.size()+1);
    }
    
    _faceLayers[_layerTagCtr] = std::move(newLayer);
    
    return lastResult;
  }
  
  AnimationStreamer::Tag AnimationStreamer::AddPersistentFaceLayer(const std::string& name, FaceTrack&& faceTrack)
  {
    faceTrack.SetIsLive(false); // don't want keyframes to delete as they play
    
    IncrementLayerTagCtr();
    
    FaceLayer newLayer;
    newLayer.tag = _layerTagCtr;
    newLayer.track = faceTrack;
    newLayer.track.Init();
    newLayer.startTime_ms = 0;
    newLayer.streamTime_ms = 0;
    newLayer.isPersistent = true;
    newLayer.sentOnce = false;
    newLayer.name = name;

    if(DEBUG_ANIMATION_STREAMING){
      PRINT_NAMED_DEBUG("AnimationStreamer.AddPersistentFaceLayer",
                        "%s, Tag = %d (Total layers=%lu)",
                        newLayer.name.c_str(), newLayer.tag, (unsigned long)_faceLayers.size());
    }
    
    _faceLayers[_layerTagCtr] = std::move(newLayer);
    
    return _layerTagCtr;
  }
  
  void AnimationStreamer::RemovePersistentFaceLayer(Tag tag, s32 duration_ms)
  {
    auto layerIter = _faceLayers.find(tag);
    if(layerIter != _faceLayers.end()) {
      PRINT_NAMED_INFO("AnimationStreamer.RemovePersistentFaceLayer",
                       "%s, Tag = %d (Layers remaining=%lu)",
                       layerIter->second.name.c_str(), layerIter->first, (unsigned long)_faceLayers.size()-1);
      

      // Add a layer that takes us back from where this persistent frame leaves
      // off to no adjustment at all.
      FaceTrack faceTrack;
      faceTrack.SetIsLive(true);
      if(duration_ms > 0)
      {
        ProceduralFaceKeyFrame firstFrame(layerIter->second.track.GetCurrentKeyFrame());
        firstFrame.SetTriggerTime(0);
        faceTrack.AddKeyFrameToBack(std::move(firstFrame));
      }
      ProceduralFaceKeyFrame lastFrame;
      lastFrame.SetTriggerTime(duration_ms);
      faceTrack.AddKeyFrameToBack(std::move(lastFrame));
      
      AddFaceLayer("Remove" + layerIter->second.name, std::move(faceTrack));
      
      _faceLayers.erase(layerIter);
    }
  }
  
  void AnimationStreamer::AddToPersistentFaceLayer(Tag tag, ProceduralFaceKeyFrame&& keyframe)
  {
    auto layerIter = _faceLayers.find(tag);
    if(layerIter != _faceLayers.end()) {
      auto & track = layerIter->second.track;
      assert(nullptr != track.GetLastKeyFrame());
      // Make keyframe trigger one sample length (plus any internal delay) past
      // the last keyframe's trigger time
      keyframe.SetTriggerTime(track.GetLastKeyFrame()->GetTriggerTime() +
                              IKeyFrame::SAMPLE_LENGTH_MS +
                              keyframe.GetTriggerTime());
      track.AddKeyFrameToBack(keyframe);
      layerIter->second.sentOnce = false;
    }
  }
  
  bool AnimationStreamer::BufferMessageToSend(RobotInterface::EngineToRobot* msg)
  {
    if(msg != nullptr) {
      _sendBuffer.push_back(msg);
      return true;
    }
    return false;
  }
  
  Result AnimationStreamer::SendBufferedMessages(Robot& robot)
  {
#   if DEBUG_ANIMATION_STREAMING
    s32 numSent = 0;
#   endif
    
    // Empty out anything waiting in the send buffer:
    RobotInterface::EngineToRobot* msg = nullptr;
    while(!_sendBuffer.empty()) {
      if(DEBUG_ANIMATION_STREAMING) {
        PRINT_NAMED_DEBUG("AnimationStreamer.SendBufferedMessages",
                          "Send buffer length=%lu.", (unsigned long)_sendBuffer.size());
      }
      
      // Compute number of bytes required and whether the robot is accepting any
      // further audio frames yet
      msg = _sendBuffer.front();
      const size_t numBytesRequired = msg->Size();
      
      s32 numAudioFramesRequired = 0;
      if(RobotInterface::EngineToRobotTag::animAudioSample == msg->GetTag() ||
         RobotInterface::EngineToRobotTag::animAudioSilence == msg->GetTag())
      {
        ++numAudioFramesRequired;
      }
      
      if(numBytesRequired <= _numBytesToSend &&
         numAudioFramesRequired <= _numAudioFramesToSend)
      {
        Result sendResult = robot.SendMessage(*msg);
        if(sendResult != RESULT_OK) {
          return sendResult;
        }
        
#       if DEBUG_ANIMATION_STREAMING
        ++numSent;
#       endif
        
        _numBytesToSend -= numBytesRequired;
        _numAudioFramesToSend -= numAudioFramesRequired;
        
        // Increment total number of bytes and audio frames streamed to robot
        robot.IncrementNumAnimationBytesStreamed((int32_t)numBytesRequired);
        robot.IncrementNumAnimationAudioFramesStreamed(numAudioFramesRequired);
        
        _sendBuffer.pop_front();
        delete msg;
      } else {
        // Out of bytes or audio frames to send, continue on next Update()
#       if DEBUG_ANIMATION_STREAMING
        PRINT_NAMED_DEBUG("Animation.SendBufferedMessages.SentMax",
                          "Sent %d messages, but ran out of bytes or audio frames to "
                          "send from buffer. %lu bytes remain, so will continue next Update().",
                          numSent, (unsigned long)_sendBuffer.size());
#       endif
        return RESULT_OK;
      }
    }
    
    // Sanity check
    // If we got here, we've finished streaming out everything in the send
    // buffer -- i.e., all the frames associated with the last audio keyframe
    assert(_numBytesToSend >= 0);
    assert(_sendBuffer.empty());
    
#   if DEBUG_ANIMATION_STREAMING
    if(numSent > 0) {
      PRINT_NAMED_DEBUG("Animation.SendBufferedMessages.Sent",
                        "Sent %d messages, %lu remain in buffer.",
                        numSent, (unsigned long)_sendBuffer.size());
    }
#   endif
    
    return RESULT_OK;
  } // SendBufferedMessages()

  Result AnimationStreamer::BufferAudioToSend(bool sendSilence, TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms)
  {
    using namespace Audio;
    
    if ( _audioClient.HasAnimation() ) {
      
      RobotAudioAnimation* audioAnimation = _audioClient.GetCurrentAnimation();
      
      RobotInterface::EngineToRobot* audioMsg = nullptr;
      audioAnimation->PopRobotAudioMessage( audioMsg, startTime_ms, streamingTime_ms );
      if ( nullptr != audioMsg ) {
        // Add key frame
        BufferMessageToSend( audioMsg );
        
        if (DEBUG_ANIMATION_STREAMING_AUDIO) {
          PRINT_NAMED_INFO("AnimationStreamer.BufferAudioToSend",
                           "Has Animation and Audio Message");
        }
      }
      else {
        // Insert Silence
        BufferMessageToSend(new RobotInterface::EngineToRobot(AnimKeyFrame::AudioSilence()));
        
        if (DEBUG_ANIMATION_STREAMING_AUDIO) {
          PRINT_NAMED_INFO("AnimationStreamer.BufferAudioToSend",
                           "Has Animation Insert Silence");
        }
      }
    }
    else if ( sendSilence ) {
      // No audio sample available, so send silence
      BufferMessageToSend(new RobotInterface::EngineToRobot(AnimKeyFrame::AudioSilence()));
      
      if (DEBUG_ANIMATION_STREAMING_AUDIO) {
        PRINT_NAMED_INFO("AnimationStreamer.BufferAudioToSend",
                         "NO Animation Insert Silence");
      }
    }
  
    return RESULT_OK;
  } // BufferAudioToSend()
  
  void AnimationStreamer::ClearSendBuffer()
  {
    for (auto& engineToRobotMsg : _sendBuffer ) {
      PRINT_NAMED_DEBUG("AnimationStreamer.ClearSendBuffer",
                        "Clearing unsent %s",
                        EngineToRobotTagToString(engineToRobotMsg->GetTag()));
      Util::SafeDelete(engineToRobotMsg);
    }
    _sendBuffer.clear();
  }

  bool AnimationStreamer::GetFaceHelper(Animations::Track<ProceduralFaceKeyFrame>& track,
                                        TimeStamp_t startTime_ms, TimeStamp_t currTime_ms,
                                        ProceduralFace& procFace,
                                        bool shouldReplace)
  {
    bool paramsSet = false;
    
    const TimeStamp_t currStreamTime = currTime_ms - startTime_ms;
    if(track.HasFramesLeft()) {
      ProceduralFaceKeyFrame& currentKeyFrame = track.GetCurrentKeyFrame();
      if(currentKeyFrame.IsTimeToPlay(startTime_ms, currTime_ms))
      {
        ProceduralFace interpolatedFace;
        
        const ProceduralFaceKeyFrame* nextFrame = track.GetNextKeyFrame();
        if (nextFrame != nullptr) {
          if (nextFrame->IsTimeToPlay(startTime_ms, currTime_ms)) {
            // If it's time to play the next frame and the current frame at the same time, something's wrong!
            PRINT_NAMED_WARNING("AnimationStreamer.GetFaceHelper.FramesTooClose",
                                "currentFrameTriggerTime: %d ms, nextFrameTriggerTime: %d, StreamTime: %d",
                                currentKeyFrame.GetTriggerTime(), nextFrame->GetTriggerTime(), currStreamTime);
            
            // Something is wrong. Just move to next frame...
            track.MoveToNextKeyFrame();
            
          } else {
            /*
            // If we're within one sample period following the currFrame, just play the current frame
            if (currStreamTime - currentKeyFrame.GetTriggerTime() < IKeyFrame::SAMPLE_LENGTH_MS) {
              interpolatedParams = currentKeyFrame.GetFace().GetParams();
              paramsSet = true;
            }
            // We're on the way to the next frame, but not too close to it: interpolate.
            else if (nextFrame->GetTriggerTime() - currStreamTime >= IKeyFrame::SAMPLE_LENGTH_MS) {
             */
              interpolatedFace = currentKeyFrame.GetInterpolatedFace(*nextFrame, currTime_ms - startTime_ms);
              paramsSet = true;
            //}
            
            if (nextFrame->IsTimeToPlay(startTime_ms, currTime_ms + IKeyFrame::SAMPLE_LENGTH_MS)) {
              track.MoveToNextKeyFrame();
            }
            
          }
        } else {
          // There's no next frame to interpolate towards: just send this keyframe
          // and move forward
          interpolatedFace = currentKeyFrame.GetFace();
          track.MoveToNextKeyFrame();
          paramsSet = true;
        }
        
        if(paramsSet) {
          if(DEBUG_ANIMATION_STREAMING) {
            const Point2f& facePosition = interpolatedFace.GetFacePosition();
            PRINT_NAMED_DEBUG("AnimationStreamer.GetFaceHelper.EyeShift",
                              "Applying eye shift from face layer of (%.1f,%.1f)",
                              facePosition.x(), facePosition.y());
          }
          
          if (shouldReplace)
          {
            procFace = interpolatedFace;
          }
          else
          {
            procFace.Combine(interpolatedFace);
          }
        }
      } // if(nextFrame != nullptr
    } // if(track.HasFramesLeft())
    
    return paramsSet;
  } // GetFaceHelper()
  
  void AnimationStreamer::RemoveKeepAliveEyeDart(s32 duration_ms)
  {
    if(NotAnimatingTag != _eyeDartTag) {
      RemovePersistentFaceLayer(_eyeDartTag, duration_ms);
      _eyeDartTag = NotAnimatingTag;
    }
  }
  
  void AnimationStreamer::SetParam(LiveIdleAnimationParameter whichParam, float newValue)
  {
    if(LiveIdleAnimationParameter::BlinkSpacingMaxTime_ms == whichParam)
    {
      if( newValue > kMaxBlinkSpacingTimeForScreenProtection_ms)
      {
        PRINT_NAMED_WARNING("AnimationStreamer.SetParam.MaxBlinkSpacingTooLong",
                            "Clamping max blink spacing to %.1fms to avoid screen burn-in",
                            kMaxBlinkSpacingTimeForScreenProtection_ms);
        
        newValue = kMaxBlinkSpacingTimeForScreenProtection_ms;
      }
    }
    
    // Call base class SetParam()
    HasSettableParameters<LiveIdleAnimationParameter, ExternalInterface::MessageGameToEngineTag::SetLiveIdleAnimationParameters, f32>::SetParam(whichParam, newValue);
  }
  
  void AnimationStreamer::KeepFaceAlive(Robot& robot)
  {
    using Param = LiveIdleAnimationParameter;
    
    // If we were interrupted from streaming an animation and we've met all the
    // conditions to even be in this function, then we should make sure we've
    // got neutral face back on the screen
    if(_wasAnimationInterruptedWithNothing) {
      SetStreamingAnimation(_neutralFaceAnimation);
      _wasAnimationInterruptedWithNothing = false;
    }
    
    _nextBlink_ms   -= BS_TIME_STEP;
    _nextEyeDart_ms -= BS_TIME_STEP;
    
    // Eye darts
    const f32 MaxDist = GetParam<f32>(Param::EyeDartMaxDistance_pix);
    if(_nextEyeDart_ms <= 0 && MaxDist > 0.f)
    {
      const bool noOtherFaceLayers = (_faceLayers.empty() ||
                                      (_faceLayers.size()==1 && _faceLayers.find(_eyeDartTag) != _faceLayers.end()));
      
      // If there's no other face layer active right now, do the dart. Otherwise,
      // skip it
      if(noOtherFaceLayers)
      {
        const f32 xDart = _rng.RandIntInRange(-MaxDist, MaxDist);
        const f32 yDart = _rng.RandIntInRange(-MaxDist, MaxDist);
        
        // Randomly choose how long the shift should take
        const s32 duration = _rng.RandIntInRange(GetParam<s32>(Param::EyeDartMinDuration_ms),
                                                 GetParam<s32>(Param::EyeDartMaxDuration_ms));
        
        //PRINT_NAMED_DEBUG("AnimationStreamer.KeepFaceAlive.EyeDart",
        //                  "shift=(%.1f,%.1f)", xDart, yDart);
        
        const f32 normDist = 5.f;
        ProceduralFace procFace;
        procFace.LookAt(xDart, yDart, normDist, normDist,
                        GetParam<f32>(Param::EyeDartUpMaxScale),
                        GetParam<f32>(Param::EyeDartDownMinScale),
                        GetParam<f32>(Param::EyeDartOuterEyeScaleIncrease));
        
        if(_eyeDartTag == NotAnimatingTag) {
          FaceTrack faceTrack;
          faceTrack.AddKeyFrameToBack(ProceduralFaceKeyFrame(procFace, duration));
          _eyeDartTag = AddPersistentFaceLayer("KeepAliveEyeDart", std::move(faceTrack));
        } else {
          AddToPersistentFaceLayer(_eyeDartTag, ProceduralFaceKeyFrame(procFace, duration));
        }
        
        _nextEyeDart_ms = _rng.RandIntInRange(GetParam<s32>(Param::EyeDartSpacingMinTime_ms),
                                              GetParam<s32>(Param::EyeDartSpacingMaxTime_ms));
      }
    }
    
    // Blinks
    if(_nextBlink_ms <= 0)
    {
      ProceduralFace blinkFace;
      
      FaceTrack faceTrack;
      TimeStamp_t totalOffset = 0;
      bool moreBlinkFrames = false;
      do {
        TimeStamp_t timeInc;
        moreBlinkFrames = ProceduralFaceDrawer::GetNextBlinkFrame(blinkFace, timeInc);
        totalOffset += timeInc;
        faceTrack.AddKeyFrameToBack(ProceduralFaceKeyFrame(blinkFace, totalOffset));
      } while(moreBlinkFrames);

      if(DEBUG_ANIMATION_STREAMING)
      {
        // Sanity checkt: we should never command two blinks at the same time
        bool alreadyBlinking = false;
        for(auto & layer : _faceLayers) {
          if(layer.second.name == "Blink") {
            PRINT_NAMED_WARNING("AnimationStreamer.KeepFaceAlive.DuplicateBlink",
                                "Seems like there's already a blink layer. Skipping this blink.");
            alreadyBlinking = true;
          }
        }
        
        if(!alreadyBlinking) {
          AddFaceLayer("Blink", std::move(faceTrack));
        }
      } else {
        AddFaceLayer("Blink", std::move(faceTrack));
      } // DEBUG_ANIMATION_STREAMING
      
      s32 blinkSpaceMin_ms = GetParam<s32>(Param::BlinkSpacingMinTime_ms);
      s32 blinkSpaceMax_ms = GetParam<s32>(Param::BlinkSpacingMaxTime_ms);
      if(blinkSpaceMax_ms <= blinkSpaceMin_ms)
      {
        PRINT_NAMED_WARNING("AnimationStreamer.KeepFaceAlive.BadBlinkSpacingParams",
                            "Max (%d) must be greater than min (%d)",
                            blinkSpaceMax_ms, blinkSpaceMin_ms);
        blinkSpaceMin_ms = kMaxBlinkSpacingTimeForScreenProtection_ms * .25f;
        blinkSpaceMax_ms = kMaxBlinkSpacingTimeForScreenProtection_ms;
      }
      _nextBlink_ms = _rng.RandIntInRange(blinkSpaceMin_ms, blinkSpaceMax_ms);
      
    }
    
  } // KeepFaceAlive()
  
  
  void AnimationStreamer::UpdateFace(Robot& robot, Animation* anim, bool storeFace)
  {
    bool faceUpdated = false;
    
    // Combine the robot's current face with anything the currently-streaming
    // animation does to the face, plus anything present in any face "layers".
    ProceduralFace procFace = *_lastProceduralFace;
    
    if(nullptr != anim) {
      // Note that shouldReplace==true in this case because the animation frames
      // actually replace what's on the face.
      faceUpdated = GetFaceHelper(anim->GetTrack<ProceduralFaceKeyFrame>(), _startTime_ms, _streamingTime_ms, procFace, true);
      
      if (faceUpdated && storeFace)
      {
        *_lastProceduralFace = procFace;
      }
    }
    
    if(DEBUG_ANIMATION_STREAMING) {
      if(!_faceLayers.empty()) {
        PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace.ApplyingFaceLayers",
                          "NumLayers=%lu", (unsigned long)_faceLayers.size());
      }
    }
    
    std::list<Tag> tagsToErase;
    
    for(auto faceLayerIter = _faceLayers.begin(); faceLayerIter != _faceLayers.end(); ++faceLayerIter)
    {
      auto & faceLayer = faceLayerIter->second;
      
      if(DEBUG_ANIMATION_STREAMING) {
        PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace.ApplyFaceLayer",
                          "%slayer %s with tag %d",
                          faceLayer.isPersistent ? "Persistent" : "",
                          faceLayer.name.c_str(), faceLayer.tag);
      }
      
      // Note that shouldReplace==false here because face layers do not replace
      // what's on the face, by definition, they layer on top of what's already there.
      faceUpdated |= GetFaceHelper(faceLayer.track, faceLayer.startTime_ms,
                                   faceLayer.streamTime_ms, procFace, false);

      faceLayer.streamTime_ms += RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
      
      if(!faceLayer.track.HasFramesLeft()) {
        
        // This layer is done...
        if(faceLayer.isPersistent) {
          if(faceLayer.track.IsEmpty()) {
            PRINT_NAMED_WARNING("AnimationStreamer.UpdateFace.EmptyPersistentLayer",
                                "Persistent face layer is empty - perhaps live frames were "
                                "used? (tag=%d)", faceLayer.tag);
            faceLayer.isPersistent = false;
          } else {
            //...but is marked persistent, so keep applying last frame
            faceLayer.track.MoveToPrevKeyFrame(); // so we're not at end() anymore
            faceLayer.streamTime_ms -= RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
            if(DEBUG_ANIMATION_STREAMING) {
              PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace.HoldingLayer",
                                "Holding last frame of face layer %s with tag %d",
                                faceLayer.name.c_str(), faceLayer.tag);
            }
            faceLayer.sentOnce = true; // mark that it has been sent at least once
            
            // We no longer need anything but the last frame (which should now be
            // "current")
            faceLayer.track.ClearUpToCurrent();
          }
        } else {
          //...and is not persistent, so delete it
          if(DEBUG_ANIMATION_STREAMING) {
            PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace.RemovingFaceLayer",
                              "%s, Tag = %d (Layers remaining=%lu)",
                              faceLayer.name.c_str(), faceLayer.tag, (unsigned long)_faceLayers.size()-1);
          }
          tagsToErase.push_back(faceLayerIter->first);
        }
      }
    }
    
    // Actually erase elements from the map
    for(auto tag : tagsToErase) {
      _faceLayers.erase(tag);
    }
    
    // If we actually made changes to the face...
    if(faceUpdated) {
      // ...turn the final procedural face into an RLE-encoded image suitable for
      // streaming to the robot
      
      if(DEBUG_ANIMATION_STREAMING)
      {
        using Param = ProceduralEyeParameter;
        const ProceduralFace::WhichEye L = ProceduralFace::WhichEye::Left;
        const ProceduralFace::WhichEye R = ProceduralFace::WhichEye::Right;

        PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace.CombinedFace",
                          "Face: shift=(%.2f,%.2f) scale=(%.2f,%.2f), "
                          "Left: shift=(%.2f,%.2f) scale=(%.2f,%.2f), "
                          "Right: shift=(%.2f,%.2f) scale=(%.2f,%.2f)",
                          procFace.GetFacePosition().x(), procFace.GetFacePosition().y(),
                          procFace.GetFaceScale().x(), procFace.GetFaceScale().y(),
                          procFace.GetParameter(L, Param::EyeCenterX),
                          procFace.GetParameter(L, Param::EyeCenterY),
                          procFace.GetParameter(L, Param::EyeScaleX),
                          procFace.GetParameter(L, Param::EyeScaleY),
                          procFace.GetParameter(R, Param::EyeCenterX),
                          procFace.GetParameter(R, Param::EyeCenterY),
                          procFace.GetParameter(R, Param::EyeScaleX),
                          procFace.GetParameter(R, Param::EyeScaleY));
      }
      
      BufferFaceToSend(procFace);
    }
  } // UpdateFace()
  
  
  void AnimationStreamer::BufferFaceToSend(const ProceduralFace& procFace)
  {
    AnimKeyFrame::FaceImage faceImageMsg;
    Result rleResult = FaceAnimationManager::CompressRLE(ProceduralFaceDrawer::DrawFace(procFace), faceImageMsg.image);
    
    if(RESULT_OK != rleResult) {
      PRINT_NAMED_ERROR("ProceduralFaceKeyFrame.GetStreamMesssageHelper",
                        "Failed to get RLE frame from procedural face.");
    } else {
      if(DEBUG_ANIMATION_STREAMING) {
        PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace",
                          "Streaming ProceduralFaceKeyFrame at t=%dms.",
                          _streamingTime_ms - _startTime_ms);
      }
      BufferMessageToSend(new RobotInterface::EngineToRobot(std::move(faceImageMsg)));
    }

  }
  
  
  Result AnimationStreamer::SendStartOfAnimation()
  {
    if(DEBUG_ANIMATION_STREAMING) {
      PRINT_NAMED_DEBUG("AnimationStreamer.SendStartOfAnimation.BufferedStartOfAnimation", "Tag=%d", _tag);
    }
    BufferMessageToSend(new RobotInterface::EngineToRobot(AnimKeyFrame::StartOfAnimation(_tag)));
    _startOfAnimationSent = true;
    _endOfAnimationSent = false;
    return RESULT_OK;
  }
  
  Result AnimationStreamer::SendEndOfAnimation(Robot& robot)
  {
    Result lastResult = RESULT_OK;
    
    DEV_ASSERT(_startOfAnimationSent,
               "Should not be sending end of animation without having first sent start of animation.");
    
    if(DEBUG_ANIMATION_STREAMING) {
      PRINT_NAMED_INFO("AnimationStreamer.SendEndOfAnimation", "Streaming EndOfAnimation at t=%dms.",
                       _streamingTime_ms - _startTime_ms);
      
      /* no longer necessary to check with the assert above?
       static TimeStamp_t lastEndOfAnimTime = 0;
       if(_streamingTime_ms - _startTime_ms == lastEndOfAnimTime) {
       PRINT_NAMED_INFO("AnimationStreamer.SendEndOfAnimation", "Already sent end of animation at t=%dms.",
       lastEndOfAnimTime);
       }
       lastEndOfAnimTime = _streamingTime_ms - _startTime_ms;
       */
    }
    
    RobotInterface::EngineToRobot endMsg{AnimKeyFrame::EndOfAnimation()};
    size_t endMsgSize = endMsg.Size();
    lastResult = robot.SendMessage(endMsg);
    if(lastResult != RESULT_OK) { return lastResult; }
    _endOfAnimationSent = true;
    _startOfAnimationSent = false;
    
    // Increment running total of bytes and "audio frames" streamed (we consider
    // end of anim to be "audio" for flow control counting)
    robot.IncrementNumAnimationBytesStreamed((int32_t)endMsgSize);
    robot.IncrementNumAnimationAudioFramesStreamed(1);
    
    return lastResult;
  } // SendEndOfAnimation()
  
  bool AnimationStreamer::HaveFaceLayersToSend()
  {
    if(_faceLayers.empty()) {
      return false;
    } else {
      // There are face layers, but we want to ignore any that are persistent that
      // have already been sent once
      for(auto & layer : _faceLayers) {
        if(!layer.second.isPersistent || !layer.second.sentOnce) {
          // There's at least one non-persistent face layer, or a persistent face layer
          // that has not been sent in its entirety at least once: return that there
          // are still face layers to send
          return true;
        }
      }
      // All face layers are persistent ones that have been sent, so no need to keep sending them
      // by themselves. They only need to be re-applied while there's something
      // else being sent
      return false;
    }
  }
  
  Result AnimationStreamer::StreamFaceLayers(Robot& robot)
  {
    Result lastResult = RESULT_OK;
    
    if(DEBUG_ANIMATION_STREAMING) {
      PRINT_NAMED_DEBUG("AnimationStreamer.StreamFaceLayers",
                        "Have %lu face layers to stream", (unsigned long)_faceLayers.size());
    }
    
    // There is no idle/streaming animation playing, but we haven't finished
    // streaming the face layers. Do so now.
    
    UpdateAmountToSend(robot);
    
    // Send anything still left in the buffer after last Update()
    lastResult = SendBufferedMessages(robot);
    if(RESULT_OK != lastResult) {
      return lastResult;
    }
    
    // Add more stuff to send buffer from face layers
    while(_sendBuffer.empty() && HaveFaceLayersToSend())
    {
      // If we have face layers to send with silent audio frames
      BufferMessageToSend(new RobotInterface::EngineToRobot(AnimKeyFrame::AudioSilence()));
      
      if(!_startOfAnimationSent) {
        IncrementTagCtr();
        _tag = _tagCtr;
        SendStartOfAnimation();
      }
      
      // Because we are updating the face with layers only, don't save face to the robot
      UpdateFace(robot, nullptr, false);
      
      // Send as much as we can of what we just buffered
      lastResult = SendBufferedMessages(robot);
      if(RESULT_OK != lastResult) {
        break;
      }
      
      // Increment fake "streaming" time, so we can evaluate below whether
      // it's time to stream out any of the other tracks. Note that it is still
      // relative to the same start time.
      _streamingTime_ms += RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
    }
    
    // If we just finished buffering all the face layers, send an end of animation message
    if(_startOfAnimationSent &&
       !HaveFaceLayersToSend() &&
       _sendBuffer.empty() &&
       !_endOfAnimationSent) {
      lastResult = SendEndOfAnimation(robot);
    }
    
    return lastResult;
  }// StreamFaceLayers()
  
  
  Result AnimationStreamer::UpdateStream(Robot& robot, Animation* anim, bool storeFace)
  {
    Result lastResult = RESULT_OK;
    
    if(!anim->IsInitialized()) {
      PRINT_NAMED_ERROR("Animation.Update", "%s: Animation must be initialized before it can be played/updated.",
                        anim != nullptr ? anim->GetName().c_str() : "<NULL>");
      return RESULT_FAIL;
    }
    
    const TimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    
    //#   if DEBUG_ANIMATIONS
    //    PRINT_NAMED_INFO("Animation.Update", "Current time = %dms", currTime_ms);
    //#   endif
    
    // Grab references to all the tracks
    auto & deviceAudioTrack = anim->GetTrack<DeviceAudioKeyFrame>();
    auto & headTrack        = anim->GetTrack<HeadAngleKeyFrame>();
    auto & liftTrack        = anim->GetTrack<LiftHeightKeyFrame>();
    auto & bodyTrack        = anim->GetTrack<BodyMotionKeyFrame>();
    auto & eventTrack       = anim->GetTrack<EventKeyFrame>();
    auto & faceAnimTrack    = anim->GetTrack<FaceAnimationKeyFrame>();
    auto & backpackLedTrack = anim->GetTrack<BackpackLightsKeyFrame>();
    
    // Is it time to play device audio? (using actual basestation time)
    if(deviceAudioTrack.HasFramesLeft() &&
       deviceAudioTrack.GetCurrentKeyFrame().IsTimeToPlay(_startTime_ms, currTime_ms)) {
       deviceAudioTrack.GetCurrentKeyFrame().PlayOnDevice();
       deviceAudioTrack.MoveToNextKeyFrame();
    }
    
    UpdateAmountToSend(robot);
    
    // Send anything still left in the buffer after last Update()
    lastResult = SendBufferedMessages(robot);
    if(RESULT_OK != lastResult) {
      return lastResult;
    }
    
    // Add more stuff to the send buffer. Note that we are not counting individual
    // keyframes here, but instead _audio_ keyframes (with which we will buffer
    // any co-timed keyframes from other tracks).
    
    while ( ShouldProcessAnimationFrame(anim, _startTime_ms, _streamingTime_ms) )
    {
      
      if(DEBUG_ANIMATION_STREAMING) {
        // Very verbose!
        //PRINT_NAMED_INFO("Animation.Update", "%d bytes left to send this Update.",
        //                 numBytesToSend);
      }
      
      // Stream a single audio sample from the audio manager (or silence if there isn't one)
      // (Have to *always* send an audio frame to keep time, whether that's the next
      // audio sample or a silent frame.)
      // NOTE: Audio frame must be first!
      BufferAudioToSend(true, _startTime_ms, _streamingTime_ms);
      
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
      
      if(BufferMessageToSend(headTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("HeadAngle");
      }
      
      if(BufferMessageToSend(liftTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("LiftHeight");
      }
      
      if(BufferMessageToSend(eventTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("Event");
      }
      
      if(BufferMessageToSend(faceAnimTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("FaceAnimation");
      }
      else if(!faceAnimTrack.HasFramesLeft()) {
        UpdateFace(robot, anim, storeFace);
      }
      
      if(BufferMessageToSend(backpackLedTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("BackpackLights");
      }
      
      if(BufferMessageToSend(bodyTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("BodyMotion");
      }
      
#     undef DEBUG_STREAM_KEYFRAME_MESSAGE
        
      // Send out as much as we can from the send buffer. If we manage to send
      // the entire buffer out, we will proceed with putting more into the buffer
      // the next time through this while loop. Otherwise, we will exit the while
      // loop and continue trying to empty the buffer on the next Update().
      // Doing this guarantees we don't try to buffer another message pointer from
      // the same frame before sending the last one (which is important because we
      // re-use message structs inside the keyframes and don't want pointers
      // getting reassigned before they get sent out!)
      lastResult = SendBufferedMessages(robot);
      if(RESULT_OK != lastResult) {
        return lastResult;
      }
      
      // Increment fake "streaming" time, so we can evaluate below whether
      // it's time to stream out any of the other tracks. Note that it is still
      // relative to the same start time.
      _streamingTime_ms += RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
      
    } // while(buffering frames)
    
    // Send an end-of-animation keyframe when done
    if( !anim->HasFramesLeft() &&
        _audioClient.AnimationIsComplete() &&
        _sendBuffer.empty() &&
        _startOfAnimationSent &&
        !_endOfAnimationSent)
    {      
       _audioClient.ClearCurrentAnimation();
      lastResult = SendEndOfAnimation(robot);
    }
    
    return lastResult;
  } // UpdateStream()
  
  
  bool AnimationStreamer::ShouldProcessAnimationFrame( Animation* anim, TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms )
  {
    bool result = false;
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
    
    return result;
  }

  
  Result AnimationStreamer::Update(Robot& robot)
  {
    ANKI_CPU_PROFILE("AnimationStreamer::Update");
    
    Result lastResult = RESULT_OK;
    
    bool streamUpdated = false;
    
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
    const bool longEnoughSinceStream  = (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - _lastStreamTime) > 0.5f;
    if(!haveStreamingAnimation &&
       haveStreamedAnything &&
       (usingLiveIdle || (!haveIdleAnimation && longEnoughSinceStream)))
    {
      KeepFaceAlive(robot);
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
        lastResult = UpdateStream(robot, _streamingAnimation, true);
        _isIdling = false;
        streamUpdated = true;
        _lastStreamTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      }
    } // if(_streamingAnimation != nullptr)
    else if(!_idleAnimationNameStack.empty() && _idleAnimationNameStack.back() != AnimationTrigger::Count) {
      
      AnimationTrigger idleAnimationGroupTrigger = _idleAnimationNameStack.back();
      
      Animation* oldIdleAnimation = _idleAnimation;
      if(idleAnimationGroupTrigger == AnimationTrigger::ProceduralLive) {
        // Make sure these are set, in case we just popped an idle and came back to idle
        _idleAnimation = &_liveAnimation;
        _isLiveTwitchEnabled = true;

        // Update the live animation's keyframes
        lastResult = UpdateLiveAnimation(robot);
        if(RESULT_OK != lastResult) {
          PRINT_NAMED_ERROR("AnimationStreamer.Update.LiveUpdateFailed",
                            "Failed updating live animation from current robot state.");
          return lastResult;
        }
      } // if(idleAnimationGroupTrigger == AnimationTrigger::ProceduralLive)
      else if(_idleAnimation == nullptr || IsFinished(_idleAnimation) || !_isIdling) {
        // We aren't doing "live" idle and the robot is either done with the last
        // idle animation or hasn't started idling yet. So it's time to select the
        // next idle from the group.
        RobotManager* robot_mgr = robot.GetContext()->GetRobotManager();
        if(robot_mgr->HasAnimationForTrigger(idleAnimationGroupTrigger) )
        {
          std::string idleAnimationGroupName = robot_mgr->GetAnimationForTrigger(idleAnimationGroupTrigger);
          const std::string animName = GetAnimationNameFromGroup(idleAnimationGroupName, robot);
          if(animName.empty()) {
            PRINT_NAMED_ERROR("AnimationStreamer.Update.EmptyIdleAnimationFromGroup",
                              "Returned empty animation name for group %s. Popping the group.",
                              EnumToString(idleAnimationGroupTrigger));
            PopIdleAnimation();
            _idleAnimation = nullptr;
            return RESULT_FAIL;
          }
          _idleAnimation = _animationContainer.GetAnimation(animName);
          if(_idleAnimation == nullptr) {
            PRINT_NAMED_ERROR("AnimationStreamer.Update.InvalidIdleAnimationFromGroup",
                              "Returned null animation for name '%s' from group '%s'. Popping the group.",
                              animName.c_str(), idleAnimationGroupName.c_str());
            PopIdleAnimation();
            return RESULT_FAIL;
          }
          
          PRINT_NAMED_DEBUG("AnimationStreamer.Update.SelectedNewIdle",
                            "Selected idle animation '%s' from group '%s'",
                            animName.c_str(), idleAnimationGroupName.c_str());
        } // if(!idleAnimationGroupName.empty())
      } // else if(_idleAnimation == nullptr || IsFinished(_idleAnimation) || !_isIdling)
      
      DEV_ASSERT(_idleAnimation != nullptr, "AnimationStreamer.Update.NullIdleAnimation");
      
      if( oldIdleAnimation != _idleAnimation ) {
        // We just changed idle animations, so we need to force re-init.
        _isIdling = false;
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
        lastResult = UpdateStream(robot, _idleAnimation, false);
        streamUpdated = true;
        _lastStreamTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      }
      _timeSpentIdling_ms += BS_TIME_STEP;
    } // else if(!_idleAnimationNameStack.empty() && _idleAnimationNameStack.back() != AnimationTrigger::Count)
    
    // If we didn't do any streaming above, but we've still got face layers to stream
    if(!streamUpdated)
    {
      if(HaveFaceLayersToSend()) {
        lastResult = StreamFaceLayers(robot);
      } else if(!_sendBuffer.empty()) {
        PRINT_NAMED_WARNING("AnimationStreamer.Update.SendBufferNotEmpty",
                            "Expect send buffer to be emptied by UpdateStream or StreamFaceLayers "
                            "by now, has %zu messages",
                            _sendBuffer.size());
        
        // Stream anything left in the send buffer. This can happen sometimes when it's taking awhile  
        UpdateAmountToSend(robot);
        lastResult = SendBufferedMessages(robot);
        if(RESULT_OK != lastResult) {
          PRINT_NAMED_WARNING("AnimationStreamer.Update.SendBufferedMessagesFailed",
                              "Could not send %zu remaining messages in send buffer",
                              _sendBuffer.size());
        }
        
        // If we've finished sending whatever messages
        if(_startOfAnimationSent &&
           _sendBuffer.empty() &&
           !_endOfAnimationSent) {
          lastResult = SendEndOfAnimation(robot);
        }
      }
    }
    
    return lastResult;
  } // AnimationStreamer::Update()
  
  
  void AnimationStreamer::UpdateAmountToSend(Robot& robot)
  {
    // Compute number of bytes free in robot animation buffer.
    // This is a lower bound since this is computed from a delayed measure
    // of the number of animation bytes already played on the robot.
    s32 totalNumBytesStreamed = robot.GetNumAnimationBytesStreamed();
    s32 totalNumBytesPlayed = robot.GetNumAnimationBytesPlayed();
    s32 totalNumAudioFramesStreamed = robot.GetNumAnimationAudioFramesStreamed();
    s32 totalNumAudioFramesPlayed = robot.GetNumAnimationAudioFramesPlayed();
    
    bool overflow = (totalNumBytesStreamed < 0) && (totalNumBytesPlayed > 0);
    DEV_ASSERT_MSG((totalNumBytesStreamed >= totalNumBytesPlayed) || overflow,
                   "AnimationStreamer.UpdateAmountToSend.BytesPlayedExceedsStreamed",
                   "totalNumBytesStreamed: %d, totalNumBytesPlayed: %d, overflow: %s",
                   totalNumBytesStreamed,
                   totalNumBytesPlayed,
                   (overflow ? "Yes" : "No"));
    
    s32 minBytesFreeInRobotBuffer = static_cast<size_t>(AnimConstants::KEYFRAME_BUFFER_SIZE) - (totalNumBytesStreamed - totalNumBytesPlayed);
    if (overflow) {
      // Computation for minBytesFreeInRobotBuffer still works out in overflow case
      PRINT_NAMED_INFO("AnimationStreamer.UpdateAmountToSend.BytesStreamedOverflow",
                       "free %d (streamed = %d, played %d)",
                       minBytesFreeInRobotBuffer, totalNumBytesStreamed, totalNumBytesPlayed);
    }
    
    if(minBytesFreeInRobotBuffer < 0) {
      PRINT_NAMED_WARNING("AnimationStreamer.UpdateAmountToSend.NegativeMinBytesFreeInRobot",
                          "minBytesFree:%d numBytesStreamed:%d numBytesPlayed:%d",
                          minBytesFreeInRobotBuffer, totalNumBytesStreamed, totalNumBytesPlayed);
      minBytesFreeInRobotBuffer = 0;
    }
    
    // Reset the number of bytes we can send each Update() as a form of
    // flow control: Don't send frames if robot has no space for them, and be
    // careful not to overwhelm reliable transport either, in terms of bytes or
    // sheer number of messages. These get decremented on each call to
    // SendBufferedMessages() below
    _numBytesToSend = std::min(MAX_BYTES_FOR_RELIABLE_TRANSPORT,
                               minBytesFreeInRobotBuffer);
    
    s32 audioFramesInBuffer = totalNumAudioFramesStreamed - totalNumAudioFramesPlayed;

    if(DEBUG_ANIMATION_STREAMING) {
      PRINT_NAMED_INFO("AnimationStreamer.UpdateAmountToSend", "Streamed= %d, Played=%d, InBuffer=%d",
                       totalNumAudioFramesStreamed, totalNumAudioFramesPlayed, audioFramesInBuffer);
    }
    _numAudioFramesToSend = std::max(0, NUM_AUDIO_FRAMES_LEAD-audioFramesInBuffer);
    
  } // UpdateAmountToSend()
  

  
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
  
  Result AnimationStreamer::UpdateLiveAnimation(Robot& robot)
  {
#   define GET_PARAM(__TYPE__, __NAME__) GetParam<__TYPE__>(LiveIdleAnimationParameter::__NAME__)
    
    Result lastResult = RESULT_OK;
    
    // Don't start wiggling until we've been idling for a bit and make sure we
    // are not picking or placing
    if(_isLiveTwitchEnabled &&
       _timeSpentIdling_ms >= GET_PARAM(s32, TimeBeforeWiggleMotions_ms) &&
       !robot.IsPickingOrPlacing())
    {
      // If wheels are available, add a little random movement to keep Cozmo looking alive
      const bool wheelsAvailable = (!robot.GetMoveComponent().IsMoving() &&
                                    !robot.GetMoveComponent().AreAnyTracksLocked((u8)AnimTrackFlag::BODY_TRACK));
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
          
          ProceduralFace procFace;
          procFace.LookAt(x, y, ProceduralFace::WIDTH/2, ProceduralFace::HEIGHT/2);
          FaceTrack faceTrack;
          faceTrack.AddKeyFrameToBack(ProceduralFaceKeyFrame(procFace, IKeyFrame::SAMPLE_LENGTH_MS));
          AddFaceLayer("LiveIdleTurn", std::move(faceTrack));
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
        _bodyMoveDuration_ms -= BS_TIME_STEP;
      }
      
      // If lift is available, add a little random movement to keep Cozmo looking alive
      const bool liftIsAvailable = (!robot.GetMoveComponent().IsLiftMoving() &&
                                    !robot.GetMoveComponent().AreAnyTracksLocked((u8)AnimTrackFlag::LIFT_TRACK));
      const bool timeToMoveLIft = (_liftMoveDuration_ms + _liftMoveSpacing_ms) <= 0;
      if(liftIsAvailable && timeToMoveLIft && !robot.IsCarryingObject())
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
        _liftMoveDuration_ms -= BS_TIME_STEP;
      }
      
      // If head is available, add a little random movement to keep Cozmo looking alive
      const bool headIsAvailable = (!robot.GetMoveComponent().IsHeadMoving() &&
                                    !robot.GetMoveComponent().AreAnyTracksLocked((u8)AnimTrackFlag::HEAD_TRACK));
      const bool timeToMoveHead = (_headMoveDuration_ms+_headMoveSpacing_ms) <= 0;
      if(headIsAvailable && timeToMoveHead)
      {
        _headMoveDuration_ms = _rng.RandIntInRange(GET_PARAM(s32, HeadMovementDurationMin_ms),
                                                   GET_PARAM(s32, HeadMovementDurationMax_ms));
        const s8 currentAngle_deg = static_cast<s8>(RAD_TO_DEG(robot.GetHeadAngle()));

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
        _headMoveDuration_ms -= BS_TIME_STEP;
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
    return _endOfAnimationSent && !anim->HasFramesLeft() && _sendBuffer.empty();
  }

  
} // namespace Cozmo
} // namespace Anki

