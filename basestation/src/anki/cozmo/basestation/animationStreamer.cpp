
#include "anki/cozmo/basestation/animation/animationStreamer.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/proceduralFaceDrawer.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "anki/cozmo/basestation/utils/hasSettableParameters_impl.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/common/basestation/utils/timer.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>

#define DEBUG_ANIMATION_STREAMING 0

// If 1, plays audio for physical robot on device too.
// If 0, only plays audio meant for sim robot on device.
// Only works if PLAY_ROBOT_AUDIO_ON_DEVICE == 1
#define PLAY_PHYSICAL_ROBOT_AUDIO_ON_DEVICE_TOO 0

namespace Anki {
namespace Cozmo {
  
  const std::string AnimationStreamer::LiveAnimation = "_LIVE_";
  const std::string AnimationStreamer::AnimToolAnimation = "_ANIM_TOOL_";

  const s32 AnimationStreamer::MAX_BYTES_FOR_RELIABLE_TRANSPORT = (1000/2) * BS_TIME_STEP; // Don't send more than 1000 bytes every 2ms

  // This is roughly (2 x ExpectedOneWayLatency_ms + BasestationTick_ms) / AudioSampleLength_ms
  const s32 AnimationStreamer::NUM_AUDIO_FRAMES_LEAD = std::ceil((2.f * 200.f + BS_TIME_STEP) / static_cast<f32>(IKeyFrame::SAMPLE_LENGTH_MS));
  
  AnimationStreamer::AnimationStreamer(IExternalInterface* externalInterface,
                                       CannedAnimationContainer& container,
                                       Audio::RobotAudioClient& audioClient)
  : HasSettableParameters(externalInterface)
  , _animationContainer(container)
  , _liveAnimation(LiveAnimation)
  , _audioClient( audioClient )
  ,_lastProceduralFace(new ProceduralFace)
  {
    _liveAnimation.SetIsLive(true);
  }

  AnimationStreamer::Tag AnimationStreamer::SetStreamingAnimation(Robot& robot, const std::string& name, u32 numLoops, bool interruptRunning)
  {
    // Special case: stop streaming the current animation
    if(name.empty()) {
#     if DEBUG_ANIMATION_STREAMING
      PRINT_NAMED_INFO("AnimationStreamer.SetStreamingAnimation",
                       "Stopping streaming of animation '%s'.\n",
                       GetStreamingAnimationName().c_str());
#     endif

      return SetStreamingAnimation(robot, nullptr);
    }
    
    return SetStreamingAnimation(robot, _animationContainer.GetAnimation(name), numLoops, interruptRunning);
  }
  
  AnimationStreamer::Tag AnimationStreamer::SetStreamingAnimation(Robot& robot, Animation* anim, u32 numLoops, bool interruptRunning)
  {
    if(nullptr != _streamingAnimation)
    {
      if(nullptr != anim && !interruptRunning) {
        PRINT_NAMED_INFO("AnimationStreamer.SetStreamingAnimation.NotInterrupting",
                         "Already streaming %s, will not interrupt with %s",
                         _streamingAnimation->GetName().c_str(),
                         anim->GetName().c_str());
        return NotAnimatingTag;
      }
      
      using namespace ExternalInterface;
      PRINT_NAMED_WARNING("AnimationStreamer.SetStreamingAnimation.Aborting",
                          "Animation %s is interrupting animation %s",
                          anim != nullptr ? anim->GetName().c_str() : "NULL",
                          _streamingAnimation->GetName().c_str());
      robot.GetExternalInterface()->Broadcast(MessageEngineToGame(AnimationAborted(_tag)));
    }
    
    // If there's something already streaming or we're purposefully clearing the buffer, abort
    if(nullptr != _streamingAnimation || nullptr == anim)
    {
      Abort(robot);
    }
    
    // If we're cancelling a current anim with no replacement, make use of the live animation track to insert a
    // face keyframe from the end of the animation being cancelled
    if (nullptr != _streamingAnimation && nullptr == anim)
    {
      bool streamingHasProcFace = !_streamingAnimation->GetTrack<ProceduralFaceKeyFrame>().IsEmpty();
      
      // Only bother if the streaming animation has some kind of face track
      if (streamingHasProcFace ||
          !_streamingAnimation->GetTrack<FaceAnimationKeyFrame>().IsEmpty())
      {
        anim = &_liveAnimation;
        
        if (streamingHasProcFace)
        {
          // Create a copy of the last procedural face frame of the streaming animation with the trigger time defaulted to 0
          auto lastFrame = _streamingAnimation->GetTrack<ProceduralFaceKeyFrame>().GetLastKeyFrame();
          ProceduralFaceKeyFrame frameCopy(lastFrame->GetFace());
          anim->AddKeyFrameToBack(frameCopy);
        }
        else
        {
          // Create a copy of the last animating face frame of the streaming animation with the trigger time defaulted to 0
          auto lastFrame = _streamingAnimation->GetTrack<FaceAnimationKeyFrame>().GetLastKeyFrame();
          FaceAnimationKeyFrame frameCopy(lastFrame->GetFaceImage(), lastFrame->GetName());
          anim->AddKeyFrameToBack(frameCopy);
        }
      }
    }
    
    _streamingAnimation = anim;
    
    if(_streamingAnimation == nullptr) {
      return NotAnimatingTag;
    } else {
      IncrementTagCtr();
    
      // Get the animation ready to play
      InitStream(robot, _streamingAnimation, _tagCtr);
      
      _numLoops = numLoops;
      _loopCtr = 0;
      
#     if DEBUG_ANIMATION_STREAMING
      PRINT_NAMED_INFO("AnimationStreamer.SetStreamingAnimation",
                       "Will start streaming '%s' animation %d times with tag=%d.\n",
                       _streamingAnimation->GetName().c_str(), numLoops, _tagCtr);
#     endif
      Anki::Util::sEvent("robot.play_animation", {}, _streamingAnimation->GetName().c_str());
      
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
          _tagCtr == AnimationStreamer::IdleAnimationTag ||
          _faceLayers.find(_tagCtr) != _faceLayers.end())
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
  
  void AnimationStreamer::Abort(Robot& robot)
  {
    if(nullptr != _streamingAnimation)
    {
      // Tell the robot to abort
      robot.AbortAnimation();
      _startOfAnimationSent = false;
      _endOfAnimationSent = false;
      _audioClient.AbortAnimation();
    }
  } // Abort()
  
  
  Result AnimationStreamer::SetIdleAnimation(const std::string &name)
  {

    Animation* oldIdleAnimation = _idleAnimation;
    
    // Special cases for disabling, animation tool, or "live"
    if(name.empty() || name == "NONE") {
#     if DEBUG_ANIMATION_STREAMING
      PRINT_NAMED_INFO("AnimationStreamer.SetIdleAnimation",
                       "Disabling idle animation.\n");
#     endif
      _idleAnimation = nullptr;
    } else if(name == LiveAnimation) {
      _idleAnimation = &_liveAnimation;
      _isLiveTwitchEnabled = true;
    } else if(name == AnimToolAnimation) {
      _idleAnimation = &_liveAnimation;
      _isLiveTwitchEnabled = false;
    }
    else {
    
      // Otherwise find the specified name and use that as the idle
      _idleAnimation = _animationContainer.GetAnimation(name);
      if(_idleAnimation == nullptr) {
        return RESULT_FAIL;
      }
    
#   if DEBUG_ANIMATION_STREAMING
      PRINT_NAMED_INFO("AnimationStreamer.SetIdleAnimation",
                       "Setting idle animation to '%s'.\n",
                       name.c_str());
#   endif
    }

    if( oldIdleAnimation != _idleAnimation ) {
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
  
  Result AnimationStreamer::InitStream(Robot& robot, Animation* anim, Tag withTag)
  {
    Result lastResult = anim->Init();
    if(lastResult == RESULT_OK)
    {
      _tag = withTag;
      
      _startTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      
      // Initialize "fake" streaming time to the same start time so we can compare
      // to it for determining when its time to stream out a keyframe
      _streamingTime_ms = _startTime_ms;
      
      if(!_sendBuffer.empty()) {
        PRINT_NAMED_WARNING("Animation.Init", "Expecting SendBuffer to be empty. Will clear.");
        _sendBuffer.clear();
      }
      
      // If this is an empty (e.g. live) animation, there is no need to
      // send end of animation keyframe until we actually send a keyframe
      _endOfAnimationSent = anim->IsEmpty();
      _startOfAnimationSent = false;
      
#     if !USE_SOUND_MANAGER_FOR_ROBOT_AUDIO
      // Prep sound
      _audioClient.LoadAnimationAudio(anim, robot.IsPhysical());
      // Set Pre Buffer Key Frame Size
      _audioClient.SetPreBufferRobotAudioMessageCount(0);
#     endif
      
#     if PLAY_ROBOT_AUDIO_ON_DEVICE
      // This prevents us from replaying the same keyframe
      _playedRobotAudio_ms = 0;
      _onDeviceRobotAudioKeyFrameQueue.clear();
      _lastPlayedOnDeviceRobotAudioKeyFrame = nullptr;
#     endif
      
      // Make sure any eye dart (which is persistent) gets removed so it doesn't
      // affect the animation we are about to start streaming. Give it a little
      // duration so it doesn't pop.
      // (Special case: allow KeepAlive to play on top of the "Live" idle.)
      if(anim != &_liveAnimation) {
        if(NotAnimatingTag != _eyeDartTag) {
          RemovePersistentFaceLayer(_eyeDartTag, 3*IKeyFrame::SAMPLE_LENGTH_MS);
          _eyeDartTag = NotAnimatingTag;
        }
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
    
#   if DEBUG_ANIMATION_STREAMING
    PRINT_NAMED_INFO("AnimationStreamer.AddFaceLayer",
                     "%s, Tag = %d (Total layers=%lu)",
                     newLayer.name.c_str(), newLayer.tag, _faceLayers.size()+1);
#   endif
    
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

#   if DEBUG_ANIMATION_STREAMING
    PRINT_NAMED_INFO("AnimationStreamer.AddPersistentFaceLayer",
                     "%s, Tag = %d (Total layers=%lu)",
                     newLayer.name.c_str(), newLayer.tag, _faceLayers.size());
#   endif
    
    _faceLayers[_layerTagCtr] = std::move(newLayer);
    
    return _layerTagCtr;
  }
  
  void AnimationStreamer::RemovePersistentFaceLayer(Tag tag, s32 duration_ms)
  {
    auto layerIter = _faceLayers.find(tag);
    if(layerIter != _faceLayers.end()) {
      PRINT_NAMED_INFO("AnimationStreamer.RemovePersistentFaceLayer",
                       "%s, Tag = %d (Layers remaining=%lu)",
                       layerIter->second.name.c_str(), layerIter->first, _faceLayers.size()-1);
      

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
#     if DEBUG_ANIMATION_STREAMING
      PRINT_NAMED_INFO("AnimationStreamer.SendBufferedMessages",
                       "Send buffer length=%lu.", _sendBuffer.size());
#     endif
      
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
        PRINT_NAMED_INFO("Animation.SendBufferedMessages.SentMax",
                         "Sent %d messages, but ran out of bytes or audio frames to "
                         "send from buffer. %lu bytes remain, so will continue next Update().",
                         numSent, _sendBuffer.size());
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
      PRINT_NAMED_INFO("Animation.SendBufferedMessages.Sent",
                       "Sent %d messages, %lu remain in buffer.",
                       numSent, _sendBuffer.size());
    }
#   endif
    
    return RESULT_OK;
  } // SendBufferedMessages()

  Result AnimationStreamer::BufferAudioToSend(bool sendSilence, TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms)
  {
    if ( _audioClient.IsPlayingAnimation() ) {
      // Get Audio Key Frame
      RobotInterface::EngineToRobot* audioMsg = nullptr;
      bool shouldSendFrame = _audioClient.PopRobotAudioMessage(audioMsg, startTime_ms, streamingTime_ms);
      if (shouldSendFrame) {
        if ( nullptr != audioMsg ) {
          // Add key frame
          BufferMessageToSend( audioMsg );
        }
        else {
          // Insert Silence
          BufferMessageToSend(new RobotInterface::EngineToRobot(AnimKeyFrame::AudioSilence()));
        }
      }
      else {
        ASSERT_NAMED( false, "Should not fall into this condition, _audioClient.PopRobotAudioMessage() should always return true" );
      }
    } // if ( _audioClient.IsPlayingAnimation() )
    else if(sendSilence) {
      // No audio sample available, so send silence
      BufferMessageToSend(new RobotInterface::EngineToRobot(AnimKeyFrame::AudioSilence()));
    }
       
    return RESULT_OK;
  } // BufferAudioToSend()
  

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
#         if DEBUG_ANIMATION_STREAMING
          const Point2f& facePosition = interpolatedFace.GetFacePosition();
          PRINT_NAMED_INFO("AnimationStreamer.GetFaceHelper.EyeShift",
                           "Applying eye shift from face layer of (%.1f,%.1f)",
                           facePosition.x(), facePosition.y());
#         endif // if DEBUG_ANIMATION_STREAMING
          
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
  
  void AnimationStreamer::SetParam(LiveIdleAnimationParameter whichParam, float newValue)
  {
    if(LiveIdleAnimationParameter::EnableKeepFaceAlive == whichParam && (bool)newValue==false)
    {
      // Get rid of any existint eye dart in case we are disabling keep face alive
      RemovePersistentFaceLayer(_eyeDartTag, IKeyFrame::SAMPLE_LENGTH_MS);
      _eyeDartTag = NotAnimatingTag;
      
      // NOTE: we can't remove an in-progress blink. It's not persistent and has
      // to complete so we don't leave the eyes in a weird state.
    }
    
    // Call base class SetParam()
    HasSettableParameters<LiveIdleAnimationParameter, ExternalInterface::MessageGameToEngineTag::SetLiveIdleAnimationParameters, f32>::SetParam(whichParam, newValue);
  }
  
  void AnimationStreamer::KeepFaceAlive(Robot& robot)
  {
    using Param = LiveIdleAnimationParameter;
    
    _nextBlink_ms   -= BS_TIME_STEP;
    _nextEyeDart_ms -= BS_TIME_STEP;
    
    // Eye darts
    const f32 MaxDist = GetParam<f32>(Param::EyeDartMaxDistance_pix);
    if(_nextEyeDart_ms <= 0 && MaxDist > 0.f)
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

#     if DEBUG_ANIMATION_STREAMING
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
#     else 
      AddFaceLayer("Blink", std::move(faceTrack));
#     endif // DEBUG_ANIMATION_STREAMING
      
      _nextBlink_ms = _rng.RandIntInRange(GetParam<s32>(Param::BlinkSpacingMinTime_ms),
                                          GetParam<s32>(Param::BlinkSpacingMaxTime_ms));
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
    
#   if DEBUG_ANIMATION_STREAMING
    if(!_faceLayers.empty()) {
      PRINT_NAMED_INFO("AnimationStreamer.UpdateFace.ApplyingFaceLayers",
                       "NumLayers=%lu", _faceLayers.size());
    }
#   endif
    
    std::list<Tag> tagsToErase;
    
    for(auto faceLayerIter = _faceLayers.begin(); faceLayerIter != _faceLayers.end(); ++faceLayerIter)
    {
      auto & faceLayer = faceLayerIter->second;
      
#     if DEBUG_ANIMATION_STREAMING
      PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace.ApplyFaceLayer",
                        "%slayer %s with tag %d",
                        faceLayer.isPersistent ? "Persistent" : "",
                        faceLayer.name.c_str(), faceLayer.tag);
#     endif
      
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
#           if DEBUG_ANIMATION_STREAMING
            PRINT_NAMED_INFO("AnimationStreamer.UpdateFace.HoldingLayer",
                             "Holding last frame of face layer %s with tag %d",
                             faceLayer.name.c_str(), faceLayer.tag);
#           endif
            faceLayer.sentOnce = true; // mark that it has been sent at least once
            
            // We no longer need anything but the last frame (which should now be
            // "current")
            faceLayer.track.ClearUpToCurrent();
          }
        } else {
          //...and is not persistent, so delete it
#         if DEBUG_ANIMATION_STREAMING
          PRINT_NAMED_INFO("AnimationStreamer.UpdateFace.RemovingFaceLayer",
                           "%s, Tag = %d (Layers remaining=%lu)",
                           faceLayer.name.c_str(), faceLayer.tag, _faceLayers.size()-1);
#         endif
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
      
#     if DEBUG_ANIMATION_STREAMING
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
#     endif
      
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
#     if DEBUG_ANIMATION_STREAMING
      PRINT_NAMED_INFO("AnimationStreamer.UpdateFace",
                       "Streaming ProceduralFaceKeyFrame at t=%dms.",
                       _streamingTime_ms - _startTime_ms);
#     endif
      BufferMessageToSend(new RobotInterface::EngineToRobot(std::move(faceImageMsg)));
    }

  }
  
  
  Result AnimationStreamer::SendStartOfAnimation()
  {
#   if DEBUG_ANIMATION_STREAMING
    PRINT_NAMED_INFO("AnimationStreamer.SendStartOfAnimation.BufferedStartOfAnimation", "Tag=%d", _tag);
#   endif
    BufferMessageToSend(new RobotInterface::EngineToRobot(AnimKeyFrame::StartOfAnimation(_tag)));
    _startOfAnimationSent = true;
    _endOfAnimationSent = false;
    return RESULT_OK;
  }
  
  Result AnimationStreamer::SendEndOfAnimation(Robot& robot)
  {
    Result lastResult = RESULT_OK;
    
    ASSERT_NAMED(_startOfAnimationSent,
                 "Should not be sending end of animation without having first sent start of animation.");
    
#   if DEBUG_ANIMATION_STREAMING
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
#   endif
    
    RobotInterface::EngineToRobot endMsg{AnimKeyFrame::EndOfAnimation()};
    size_t endMsgSize = endMsg.Size();
    lastResult = robot.SendMessage(std::move(endMsg));
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
  
  Result AnimationStreamer::StreamFaceLayersOrAudio(Robot& robot)
  {
    Result lastResult = RESULT_OK;
    
#   if DEBUG_ANIMATION_STREAMING
    PRINT_NAMED_INFO("AnimationStreamer.StreamFaceLayers",
                     "Have %lu face layers to stream", _faceLayers.size());
#   endif
    
    // There is no idle/streaming animation playing, but we haven't finished
    // streaming the face layers. Do so now.
    
    UpdateAmountToSend(robot);
    
    // Send anything still left in the buffer after last Update()
    lastResult = SendBufferedMessages(robot);
    if(RESULT_OK != lastResult) {
      PRINT_NAMED_ERROR("AnimationStreamer.Update.SendBufferedMessagesFailed", "");
      return lastResult;
    }
    
    // Add more stuff to send buffer from face layers
    while(_sendBuffer.empty() && HaveFaceLayersToSend())
    {
      // If we have face layers to send, we _do_ want BufferAudioToSend to
      // buffer audio silence keyframes to keep the clock ticking. If not, we
      // don't need to send audio silence.
      const bool sendSilence = (_faceLayers.empty() ? false : true);
      BufferAudioToSend( sendSilence, _startTime_ms, _streamingTime_ms );
      
      if(!_startOfAnimationSent) {
        SendStartOfAnimation();
      }
      
      // Because we are updating the face with layers only, don't save face to the robot
      UpdateFace(robot, nullptr, false);
      
      // Send as much as we can of what we just buffered
      lastResult = SendBufferedMessages(robot);
      if(RESULT_OK != lastResult) {
        PRINT_NAMED_ERROR("AnimationStreamer.Update.SendBufferedMessagesFailed", "");
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
    auto & facePosTrack     = anim->GetTrack<FacePositionKeyFrame>();
    auto & faceAnimTrack    = anim->GetTrack<FaceAnimationKeyFrame>();
    auto & blinkTrack       = anim->GetTrack<BlinkKeyFrame>();
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
      PRINT_NAMED_ERROR("Animation.Update.SendBufferedMessagesFailed", "");
      return lastResult;
    }
    
    // Add more stuff to the send buffer. Note that we are not counting individual
    // keyframes here, but instead _audio_ keyframes (with which we will buffer
    // any co-timed keyframes from other tracks).
    while( _sendBuffer.empty() &&
          ( (anim->HasFramesLeft() && !_audioClient.IsPlayingAnimation() ) ||
            (_audioClient.UpdateFirstBuffer() && _audioClient.PrepareRobotAudioMessage(_startTime_ms, _streamingTime_ms)) ) )
    {
#     if DEBUG_ANIMATIONS
      //PRINT_NAMED_INFO("Animation.Update", "%d bytes left to send this Update.",
      //                 numBytesToSend);
#     endif
      
#     if USE_SOUND_MANAGER_FOR_ROBOT_AUDIO
      // Have to always send an audio frame to keep time, whether that's the next
      // audio sample or a silent frame. This increments "streamingTime"
      // NOTE: Audio frame must be first!
      auto & robotAudioTrack  = anim->GetTrack<RobotAudioKeyFrame>();
      
      if(robotAudioTrack.HasFramesLeft() &&
         robotAudioTrack.GetCurrentKeyFrame().IsTimeToPlay(_startTime_ms, _streamingTime_ms))
      {
#       if PLAY_ROBOT_AUDIO_ON_DEVICE && !defined(ANKI_IOS_BUILD)
        // Queue up audio frame for playing locally if
        // it's not already in the queued and it wasn't already played.
        const RobotAudioKeyFrame* audioKF = &robotAudioTrack.GetCurrentKeyFrame();
        if ((audioKF != _lastPlayedOnDeviceRobotAudioKeyFrame) &&
            (_onDeviceRobotAudioKeyFrameQueue.empty() || audioKF != _onDeviceRobotAudioKeyFrameQueue.back())) {
          _onDeviceRobotAudioKeyFrameQueue.push_back(audioKF);
        }
#       endif // PLAY_ROBOT_AUDIO_ON_DEVICE && !defined(ANKI_IOS_BUILD)
        
        if(!BufferMessageToSend(robotAudioTrack.GetCurrentKeyFrame().GetStreamMessage()))
        {
          // No samples left to send for this keyframe. Move to next keyframe,
          // and for now send silence.
          //PRINT_NAMED_INFO("Animation.Update", "Streaming AudioSilenceKeyFrame.");
          
          robotAudioTrack.MoveToNextKeyFrame();
          BufferMessageToSend(new RobotInterface::EngineToRobot(AnimKeyFrame::AudioSilence()));
        }
      } else {
        // No frames left or not time to play next frame yet, so send silence
        //PRINT_NAMED_INFO("Animation.Update", "Streaming AudioSilenceKeyFrame.");
        BufferMessageToSend(new RobotInterface::EngineToRobot(AnimKeyFrame::AudioSilence()));
      }

#     else // if (!USE_SOUND_MANAGER_FOR_ROBOT_AUDIO)
      
      // Stream a single audio sample from the audio manager (or silence if there isn't one)
      // (Have to *always* send an audio frame to keep time, whether that's the next
      // audio sample or a silent frame.)
      // NOTE: Audio frame must be first!
      BufferAudioToSend(true, _startTime_ms, _streamingTime_ms);
      
#     endif // USE_SOUND_MANAGER_FOR_ROBOT_AUDIO
      
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
      
      if(BufferMessageToSend(facePosTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("FacePosition");
      }
      
      bool streamedFaceAnimImage = false;
      if(BufferMessageToSend(faceAnimTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
        streamedFaceAnimImage = true;
        DEBUG_STREAM_KEYFRAME_MESSAGE("FaceAnimation");
      }
      
      UpdateFace(robot, anim, storeFace);
      
      if(BufferMessageToSend(blinkTrack.GetCurrentStreamingMessage(_startTime_ms, _streamingTime_ms))) {
        DEBUG_STREAM_KEYFRAME_MESSAGE("Blink");
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
        PRINT_NAMED_ERROR("Animation.Update.SendBufferedMessagesFailed", "");
        return lastResult;
      }
      
      // Increment fake "streaming" time, so we can evaluate below whether
      // it's time to stream out any of the other tracks. Note that it is still
      // relative to the same start time.
      _streamingTime_ms += RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
      
    } // while(buffering frames)
    
#   if USE_SOUND_MANAGER_FOR_ROBOT_AUDIO
#   if PLAY_ROBOT_AUDIO_ON_DEVICE && !defined(ANKI_IOS_BUILD)
    if (PLAY_PHYSICAL_ROBOT_AUDIO_ON_DEVICE_TOO || !robot.IsPhysical()) {
      for (auto audioKF : _onDeviceRobotAudioKeyFrameQueue)
      {
        if(!anim->HasFramesLeft() ||   // If all tracks buffered already, then play this now.
           ((_playedRobotAudio_ms < _startTime_ms + audioKF->GetTriggerTime()) &&
            audioKF->IsTimeToPlay(_startTime_ms,  currTime_ms)))
        {
          // TODO: Insert some kind of small delay to simulate latency?
          SoundManager::getInstance()->Play(audioKF->GetSoundName());
          _playedRobotAudio_ms = currTime_ms;
          
          _lastPlayedOnDeviceRobotAudioKeyFrame = audioKF;
          _onDeviceRobotAudioKeyFrameQueue.pop_front();
        }
      }
    }
#   endif // PLAY_ROBOT_AUDIO_ON_DEVICE && !defined(ANKI_IOS_BUILD)
#   endif // USE_SOUND_MANAGER_FOR_ROBOT_AUDIO
    
    // Send an end-of-animation keyframe when done
    if( !anim->HasFramesLeft() &&
        !_audioClient.IsPlayingAnimation() &&
        _sendBuffer.empty() &&
        _startOfAnimationSent &&
        !_endOfAnimationSent)
    {
      lastResult = SendEndOfAnimation(robot);
    }
    
    return lastResult;
  } // UpdateStream()
  
  
  Result AnimationStreamer::Update(Robot& robot)
  {
    Result lastResult = RESULT_OK;
    
    bool streamUpdated = false;
    
    VizManager* vizManager = robot.GetContext()->GetVizManager();
    ASSERT_NAMED(nullptr != vizManager, "Expecting a non-null VizManager");

    // Update name in viz:
    if(nullptr == _streamingAnimation && nullptr == _idleAnimation)
    {
      vizManager->SetText(VizManager::ANIMATION_NAME, NamedColors::WHITE, "Anim: <none>");
    } else if(nullptr != _streamingAnimation) {
      vizManager->SetText(VizManager::ANIMATION_NAME, NamedColors::WHITE, "Anim: %s",
                          _streamingAnimation->GetName().c_str());
    } else if(nullptr != _idleAnimation) {
      vizManager->SetText(VizManager::ANIMATION_NAME, NamedColors::WHITE, "Anim[Idle]: %s",
                          _idleAnimation->GetName().c_str());
    }
    
    // Always keep face alive, unless we have a streaming animation, since we rely on it
    // to do all face updating and we don't want to step on it's hand-designed toes.
    // Wait a 1/2 second before running after we finish the last streaming animation
    // to help reduce stepping on the next animation's toes when we have things
    // sequenced.
    
    // _tagCtr check so we wait until we get one behavior until we start playing
    if(GetParam<bool>(LiveIdleAnimationParameter::EnableKeepFaceAlive) &&
       _tagCtr > 0 && _streamingAnimation == nullptr &&
       (_idleAnimation == &_liveAnimation || (_idleAnimation == nullptr &&
       BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - _lastStreamTime > 0.5f)))
    {
      KeepFaceAlive(robot);
    }
    
    if(_streamingAnimation != nullptr) {
      _timeSpentIdling_ms = 0;
      
      if(IsFinished(_streamingAnimation)) {
        
        ++_loopCtr;
        
        if(_numLoops == 0 || _loopCtr < _numLoops) {
#         if DEBUG_ANIMATION_STREAMING
          PRINT_NAMED_INFO("AnimationStreamer.Update.Looping",
                           "Finished loop %d of %d of '%s' animation. Restarting.\n",
                           _loopCtr, _numLoops,
                           _streamingAnimation->GetName().c_str());
#         endif
          
          // Reset the animation so it can be played again:
          InitStream(robot, _streamingAnimation, _tagCtr);
          
        } else {
#         if DEBUG_ANIMATION_STREAMING
          PRINT_NAMED_INFO("AnimationStreamer.Update.FinishedStreaming",
                           "Finished streaming '%s' animation.\n",
                           _streamingAnimation->GetName().c_str());
#         endif
          
          _streamingAnimation = nullptr;
        }
        
      } else {
        // We do want to store this face to the robot since it's coming from an actual animation
        lastResult = UpdateStream(robot, _streamingAnimation, true);
        _isIdling = false;
        streamUpdated = true;
        _lastStreamTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      }
    } else if(_idleAnimation != nullptr) {
      
      // Update the live animation if we're using it
      if(_idleAnimation == &_liveAnimation)
      {
        lastResult = UpdateLiveAnimation(robot);
        if(RESULT_OK != lastResult) {
          PRINT_NAMED_ERROR("AnimationStreamer.Update.LiveUpdateFailed",
                            "Failed updating live animation from current robot state.");
          return lastResult;
        }
      }
      
      if((!robot.IsAnimating() && IsFinished(_idleAnimation)) || !_isIdling) {
#       if DEBUG_ANIMATION_STREAMING
        PRINT_NAMED_INFO("AnimationStreamer.Update.IdleAnimInit",
                         "(Re-)Initializing idle animation: '%s'.\n",
                         _idleAnimation->GetName().c_str());
#       endif
        
        // Just finished playing a loop, or we weren't just idling. Either way,
        // (re-)init the animation so it can be played (again)
        InitStream(robot, _idleAnimation, IdleAnimationTag);
        _isIdling = true;
        //InitIdleAnimation();
      }
      
      if(_idleAnimation->HasFramesLeft()) {
        // This is just an idle animation, so we don't want to save the face to the robot
        lastResult = UpdateStream(robot, _idleAnimation, false);
        streamUpdated = true;
        _lastStreamTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      }
      _timeSpentIdling_ms += BS_TIME_STEP;
    }
    
    // If we didn't do any streaming above, but we've still got face layers to
    // stream or there's audio waiting to go out, stream those now
    if(!streamUpdated)
    {
      if(HaveFaceLayersToSend()) {
        lastResult = StreamFaceLayersOrAudio(robot);
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
    ASSERT_NAMED((totalNumBytesStreamed >= totalNumBytesPlayed) || overflow, ("AnimationStreamer.UpdateAmountToSend totalNumBytesStreamed: " + std::to_string(totalNumBytesStreamed) + "  totalNumBytesPlayed: " + std::to_string(totalNumBytesPlayed) + " overflow: " + (overflow ? "Yes" : "No")).c_str());

    
    s32 minBytesFreeInRobotBuffer = static_cast<size_t>(AnimConstants::KEYFRAME_BUFFER_SIZE) - (totalNumBytesStreamed - totalNumBytesPlayed);
    if (overflow) {
      // Computation for minBytesFreeInRobotBuffer still works out in overflow case
      PRINT_NAMED_INFO("AnimationStreamer.UpdateAmountToSend.BytesStreamedOverflow",
                       "free %d (streamed = %d, played %d)",
                       minBytesFreeInRobotBuffer, totalNumBytesStreamed, totalNumBytesPlayed);
    }
    assert(minBytesFreeInRobotBuffer >= 0);
    
    // Reset the number of bytes we can send each Update() as a form of
    // flow control: Don't send frames if robot has no space for them, and be
    // careful not to overwhelm reliable transport either, in terms of bytes or
    // sheer number of messages. These get decremenged on each call to
    // SendBufferedMessages() below
    _numBytesToSend = std::min(MAX_BYTES_FOR_RELIABLE_TRANSPORT,
                               minBytesFreeInRobotBuffer);
    
    s32 audioFramesInBuffer = totalNumAudioFramesStreamed - totalNumAudioFramesPlayed;

#   if DEBUG_ANIMATION_STREAMING
    PRINT_NAMED_INFO("AnimationStreamer.UpdateAmountToSend", "Streamed= %d, Played=%d, InBuffer=%d",
                     totalNumAudioFramesStreamed, totalNumAudioFramesPlayed, audioFramesInBuffer);
#   endif
    _numAudioFramesToSend = std::max(0, NUM_AUDIO_FRAMES_LEAD-audioFramesInBuffer);
    
  } // UpdateAmountToSend()
  

  
  void AnimationStreamer::SetDefaultParams()
  {
#   define SET_DEFAULT(__NAME__, __VALUE__) \
    SetParam(LiveIdleAnimationParameter::__NAME__,  static_cast<f32>(__VALUE__))
    
    SET_DEFAULT(EnableKeepFaceAlive, true);
    
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
        s16 curvature = s16_MAX; // drive straight
        if(_rng.RandDblInRange(0., 1.) > GET_PARAM(f32, BodyMovementStraightFraction)) {
          curvature = 0;
          
          // If turning in place, look in the direction of the turn
          const s32 x = (speed < 0 ? -1.f : 1.f) * _rng.RandIntInRange(0, ProceduralFace::WIDTH/6);
          const s32 y = _rng.RandIntInRange(-ProceduralFace::HEIGHT/6, ProceduralFace::HEIGHT/6);
#         if DEBUG_ANIMATION_STREAMING
          PRINT_NAMED_DEBUG("AnimationStreamer.UpdateLiveAnimation.EyeLeadTurn",
                            "Point turn eye shift (%d,%d)", x, y);
#         endif
          
          ProceduralFace procFace;
          procFace.LookAt(x, y, ProceduralFace::WIDTH/2, ProceduralFace::HEIGHT/2);
          FaceTrack faceTrack;
          faceTrack.AddKeyFrameToBack(ProceduralFaceKeyFrame(procFace, IKeyFrame::SAMPLE_LENGTH_MS));
          AddFaceLayer("LiveIdleTurn", std::move(faceTrack));
        }
        
#       if DEBUG_ANIMATION_STREAMING
        PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.BodyTwitch",
                         "Speed=%d, curvature=%d, duration=%d",
                         speed, curvature, _bodyMoveDuration_ms);
#       endif
        
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

#       if DEBUG_ANIMATION_STREAMING
        PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.LiftTwitch",
                         "duration=%d", _liftMoveDuration_ms);
#       endif
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

#       if DEBUG_ANIMATION_STREAMING
        PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.HeadTwitch",
                         "duration=%d", _headMoveDuration_ms);
#       endif
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

