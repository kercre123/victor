
#include "anki/cozmo/basestation/animation/animationStreamer.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/animationKeyFrames.h"
#include "anki/cozmo/basestation/utils/hasSettableParameters_impl.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "util/logging/logging.h"
#include "anki/common/basestation/utils/timer.h"

#define DEBUG_ANIMATION_STREAMING 0


namespace Anki {
namespace Cozmo {
  
  const std::string AnimationStreamer::LiveAnimation = "_LIVE_";
  const std::string AnimationStreamer::AnimToolAnimation = "_ANIM_TOOL_";

  const s32 AnimationStreamer::MAX_BYTES_FOR_RELIABLE_TRANSPORT = (1000/2) * BS_TIME_STEP; // Don't send more than 1000 bytes every 2ms
  
  AnimationStreamer::AnimationStreamer(IExternalInterface* externalInterface,
                                       CannedAnimationContainer& container,
                                       Audio::RobotAudioClient& audioClient)
  : HasSettableParameters(externalInterface)
  , _animationContainer(container)
  , _liveAnimation(LiveAnimation)
  , _audioClient( audioClient )
  {

  }

  u8 AnimationStreamer::SetStreamingAnimation(const std::string& name, u32 numLoops)
  {
    // Special case: stop streaming the current animation
    if(name.empty()) {
#     if DEBUG_ANIMATION_STREAMING
      PRINT_NAMED_INFO("AnimationStreamer.SetStreamingAnimation",
                       "Stopping streaming of animation '%s'.\n",
                       GetStreamingAnimationName().c_str());
#     endif
      
      _streamingAnimation = nullptr;
      return 0;
    }
    
    _streamingAnimation = _animationContainer.GetAnimation(name);
    if(_streamingAnimation == nullptr) {
      return 0;
    } else {
      
      // Incrememnt the tag counter and keep it from being one of the "special"
      // values used to indicate "not animating" or "idle animation"
      ++_tagCtr;
      while(_tagCtr == 0 || _tagCtr == IdleAnimationTag) {
        ++_tagCtr;
      }
    
      // Get the animation ready to play
      InitStream(_streamingAnimation, _tagCtr);
      
      _numLoops = numLoops;
      _loopCtr = 0;
      
#     if DEBUG_ANIMATION_STREAMING
      PRINT_NAMED_INFO("AnimationStreamer.SetStreamingAnimation",
                       "Will start streaming '%s' animation %d times with tag=%d.\n",
                       name.c_str(), numLoops, _tagCtr);
#     endif
    
      return _tagCtr;
    }
  }

  
  Result AnimationStreamer::SetIdleAnimation(const std::string &name)
  {
    // Special cases for disabling, animation tool, or "live"
    if(name.empty() || name == "NONE") {
#     if DEBUG_ANIMATION_STREAMING
      PRINT_NAMED_INFO("AnimationStreamer.SetIdleAnimation",
                       "Disabling idle animation.\n");
#     endif
      _idleAnimation = nullptr;
      return RESULT_OK;
    } else if(name == LiveAnimation) {
      _idleAnimation = &_liveAnimation;
      _isLiveTwitchEnabled = true;
      return RESULT_OK;
    } else if(name == AnimToolAnimation) {
      _idleAnimation = &_liveAnimation;
      _isLiveTwitchEnabled = false;
      return RESULT_OK;
    }
    
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

    return RESULT_OK;
  }
  
  Result AnimationStreamer::InitStream(Animation* anim, u8 withTag)
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
      
#     if PLAY_ROBOT_AUDIO_ON_DEVICE
      // This prevents us from replaying the same keyframe
      _playedRobotAudio_ms = 0;
      _onDeviceRobotAudioKeyFrameQueue.clear();
      _lastPlayedOnDeviceRobotAudioKeyFrame = nullptr;
#     endif
    }
    return lastResult;
  }
  
  Result AnimationStreamer::AddFaceLayer(const FaceTrack& faceTrack, TimeStamp_t delay_ms)
  {
    Result lastResult = RESULT_OK;
    
    FaceLayer newLayer;
    newLayer.track = faceTrack; // COPY the track in
    newLayer.track.Init();
    newLayer.startTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp() + delay_ms;
    newLayer.streamTime_ms = newLayer.startTime_ms;
    newLayer.isLooping = false;
    
    _faceLayers.emplace_back(std::move(newLayer));
    
    return lastResult;
  }
  
  u32 AnimationStreamer::AddLoopingFaceLayer(const FaceTrack& faceTrack)
  {
    static u32 TagCtr = 0;

    FaceLayer newLayer;
    newLayer.tag = TagCtr++;
    newLayer.track = faceTrack;
    newLayer.track.Init();
    newLayer.startTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    newLayer.streamTime_ms = newLayer.startTime_ms;
    newLayer.isLooping = true;
    newLayer.sentOnce = false;
    
    PRINT_NAMED_INFO("AnimationStreamer.AddLoopingFaceLayer",
                     "Tag = %d", newLayer.tag);
    
    const u32 returnTag = newLayer.tag;
    _faceLayers.emplace_back(std::move(newLayer));
    
    return returnTag;
  }
  
  void AnimationStreamer::RemoveLoopingFaceLayer(u32 tag)
  {
    for(auto layerIter = _faceLayers.begin(); layerIter != _faceLayers.end(); ++layerIter) {
      if(layerIter->tag == tag) {
        PRINT_NAMED_INFO("AnimationStreamer.RemoveLoopingFaceLayer",
                         "Tag = %d", layerIter->tag);
        
        _faceLayers.erase(layerIter);
        return; // once found, stop looking immediately (there should be no more layers w/ this tag)
      }
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
      PRINT_NAMED_INFO("Animation.SendBufferedMessages",
                       "Send buffer length=%lu.", _sendBuffer.size());
#     endif
      
      msg = _sendBuffer.front();
      const size_t numBytesRequired = msg->Size();
      if(numBytesRequired <= _numBytesToSend) {
        Result sendResult = robot.SendMessage(*msg);
        if(sendResult != RESULT_OK) {
          return sendResult;
        }
        
#       if DEBUG_ANIMATION_STREAMING
        ++numSent;
#       endif
        
        _numBytesToSend -= numBytesRequired;
        
        // Increment total number of bytes streamed to robot
        robot.IncrementNumAnimationBytesStreamed((int32_t)numBytesRequired);
        
        _sendBuffer.pop_front();
        delete msg;
      } else {
        // Out of bytes to send, continue on next Update()
#       if DEBUG_ANIMATION_STREAMING
        PRINT_NAMED_INFO("Animation.SendBufferedMessages",
                         "Sent %d messages, but ran out of bytes to send from "
                         "buffer. %lu remain, so will continue next Update().",
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

  Result AnimationStreamer::BufferAudioToSend(bool sendSilence)
  {
    if ( _audioClient.IsPlugInActive() ) {
      if ( _audioClient.HasKeyFrameAudioSample() ) {
        // Give audio sample memory ownership to engine message
        AnimKeyFrame::AudioSample* audioSample = _audioClient.PopAudioSample();
        BufferMessageToSend( new RobotInterface::EngineToRobot( std::move( *audioSample ) ) );
      }
    }

    else if(sendSilence) {
      // No audio sample available, so send silence
      BufferMessageToSend(new RobotInterface::EngineToRobot(AnimKeyFrame::AudioSilence()));
    }
       
    return RESULT_OK;
  } // BufferAudioToSend()
  

  inline static void SetFaceParams(ProceduralFaceParams& faceParams,
                                   ProceduralFaceParams&& interpolatedFrame,
                                   bool shouldReplace)
  {
    if (shouldReplace)
    {
      faceParams = interpolatedFrame;
    }
    else
    {
      faceParams.Combine(interpolatedFrame);
    }
  }
  
  bool AnimationStreamer::GetFaceHelper(Animations::Track<ProceduralFaceKeyFrame>& track,
                                        TimeStamp_t startTime_ms, TimeStamp_t currTime_ms,
                                        ProceduralFaceParams& faceParams,
                                        bool shouldReplace)
  {
    bool paramsSet = false;
    
    if(track.HasFramesLeft()) {
      ProceduralFaceKeyFrame& currentKeyFrame = track.GetCurrentKeyFrame();
      if(currentKeyFrame.IsTimeToPlay(startTime_ms, currTime_ms))
      {
        ProceduralFaceParams interpolatedParams;
        
        ProceduralFaceKeyFrame* nextFrame = track.GetNextKeyFrame();
        if (nextFrame != nullptr) {
          if (nextFrame->IsTimeToPlay(startTime_ms, currTime_ms)) {
            // If it's time to play the next frame, then the current frame is done,
            // so move the track forward
            
            if(nextFrame->GetTriggerTime() - currentKeyFrame.GetTriggerTime() <= IKeyFrame::SAMPLE_LENGTH_MS)
            {
              // Special case: the current frame is within one sample length of the
              // next frame, meaning it became time to play both of them at the same
              // time. So we want to make sure we actually send the current frame
              // before moving ahead.
              interpolatedParams = currentKeyFrame.GetFace().GetParams();
              paramsSet = true;
            }
            
            track.MoveToNextKeyFrame();
          } else {
            // We're on the way to the next frame: interpolate.
            interpolatedParams = currentKeyFrame.GetInterpolatedFaceParams(*nextFrame, currTime_ms - startTime_ms);
            paramsSet = true;
          }
        } else {
          // There's no next frame to interpolate towards: just send this keyframe
          // and move forward
          interpolatedParams = currentKeyFrame.GetFace().GetParams();
          track.MoveToNextKeyFrame();
          paramsSet = true;
        }
        
        if(paramsSet) {
          SetFaceParams(faceParams, std::move(interpolatedParams), shouldReplace);
        }
      } // if(nextFrame != nullptr
    } // if(track.HasFramesLeft())
    
    return paramsSet;
  } // GetFaceHelper()
  
  
  void AnimationStreamer::KeepFaceAlive(Robot& robot)
  {
    using Param = LiveIdleAnimationParameter;
    
    _nextBlink_ms   -= BS_TIME_STEP;
    _nextEyeDart_ms -= BS_TIME_STEP;
    
    // Eye darts
    if(_nextEyeDart_ms <= 0)
    {
      // Shift the eyes slightly by a random amount
      const s32 xShift = _rng.RandIntInRange(0, GetParam<s32>(Param::EyeDartMaxDistance_pix));
      const s32 yShift = _rng.RandIntInRange(0, GetParam<s32>(Param::EyeDartMaxDistance_pix));
      
      robot.ShiftEyes(xShift, yShift, IKeyFrame::SAMPLE_LENGTH_MS);
      
      _nextEyeDart_ms = _rng.RandIntInRange(GetParam<s32>(Param::EyeDartSpacingMinTime_ms),
                                            GetParam<s32>(Param::EyeDartSpacingMaxTime_ms));
    }
    
    // Blinks
    if(_nextBlink_ms <= 0)
    {
      ProceduralFace blinkFace(robot.GetProceduralFace());
      
      // Now we clear out the current face params so that the layer generated
      // below starts as the nominal face
      blinkFace.SetParams(ProceduralFaceParams());
      
      FaceTrack faceTrack;
      TimeStamp_t totalOffset = 0;
      bool moreBlinkFrames = false;
      do {
        TimeStamp_t timeInc;
        moreBlinkFrames = blinkFace.GetNextBlinkFrame(timeInc);
        totalOffset += timeInc;
        
        ProceduralFaceKeyFrame kf(blinkFace, totalOffset);
        kf.SetIsLive(true);
        
        faceTrack.AddKeyFrame(std::move(kf));
      } while(moreBlinkFrames);
      
      AddFaceLayer(std::move(faceTrack));
      
      _nextBlink_ms = _rng.RandIntInRange(GetParam<s32>(Param::BlinkSpacingMinTime_ms),
                                          GetParam<s32>(Param::BlinkSpacingMaxTime_ms));
    }
    
  } // KeepFaceAlive()
  
  
  void AnimationStreamer::UpdateFace(Robot& robot, Animation* anim, bool storeFace)
  {
    bool faceUpdated = false;
    
    // Combine the robot's current face with anything the currently-streaming
    // animation does to the face, plus anything present in any face "layers".
    ProceduralFaceParams faceParams = robot.GetProceduralFace().GetParams();
    
    if(nullptr != anim) {
      // Note that shouldReplace==true in this case because the animation frames
      // actually replace what's on the face.
      faceUpdated = GetFaceHelper(anim->GetTrack<ProceduralFaceKeyFrame>(), _startTime_ms, _streamingTime_ms, faceParams, true);
    }
    
    for(auto faceLayerIter = _faceLayers.begin(); faceLayerIter != _faceLayers.end(); )
    {
      // Note that shouldReplace==false here because face layers do not replace
      // what's on the face, by defintion, they layer on top of what's already there.
      faceUpdated |= GetFaceHelper(faceLayerIter->track, faceLayerIter->startTime_ms,
                                   faceLayerIter->streamTime_ms, faceParams, false);
      
      faceLayerIter->streamTime_ms += RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
      
      if(!faceLayerIter->track.HasFramesLeft()) {
        
        // This layer is done...
        if(faceLayerIter->isLooping) {
          //...but is marked to loop, so restart it
          faceLayerIter->track.Init();
          faceLayerIter->sentOnce = true; // mark that it has been sent at least once
          ++faceLayerIter;
        } else {
          //...and is not looping, so delete it
          faceLayerIter = _faceLayers.erase(faceLayerIter);
        }
      } else {
        ++faceLayerIter;
      }
    }
    
    // If we actually made changes to the face...
    if(faceUpdated) {
      // ...turn the final procedural face into an RLE-encoded image suitable for
      // streaming to the robot
      AnimKeyFrame::FaceImage faceImageMsg;
      ProceduralFace procFace(robot.GetProceduralFace());
      procFace.SetParams(faceParams);
      Result rleResult = FaceAnimationManager::CompressRLE(procFace.GetFace(), faceImageMsg.image);
      
      if(RESULT_OK != rleResult) {
        PRINT_NAMED_ERROR("ProceduralFaceKeyFrame.GetStreamMesssageHelper",
                          "Failed to get RLE frame from procedural face.");
      } else {
#       if DEBUG_ANIMATION_STREAMING
        PRINT_NAMED_INFO("AnimationStreamer.UpdateFace",
                         "Streaming ProceduralFaceKeyFrame at t=%dms.",
                         _streamingTime_ms - _startTime_ms);
#       endif
        BufferMessageToSend(new RobotInterface::EngineToRobot(std::move(faceImageMsg)));
      }
      
      if (storeFace)
      {
        // Also store the updated face in the robot
        robot.SetProceduralFace(procFace);
      }
    }
  } // UpdateFace()
  
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
    
    // Increment running total of bytes streamed
    robot.IncrementNumAnimationBytesStreamed((int32_t)endMsgSize);
    
    return lastResult;
  } // SendEndOfAnimation()
  
  bool AnimationStreamer::HaveNonLoopingFaceLayersToSend()
  {
    if(_faceLayers.empty()) {
      return false;
    } else {
      // There are face layers, but we want to ignore any that are looping that
      // have already been sent once
      for(auto & layer : _faceLayers) {
        if(!layer.isLooping || !layer.sentOnce) {
          // There's at least one non-looping face layer, or a looping face layer
          // that has not been sent once: return that there are
          // face layers to send
          return true;
        }
      }
      // All face layers are looping ones that have been sent, so no need to keep sending them
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
    
    UpdateNumBytesToSend(robot);
    
    // Send anything still left in the buffer after last Update()
    lastResult = SendBufferedMessages(robot);
    if(RESULT_OK != lastResult) {
      PRINT_NAMED_ERROR("AnimationStreamer.Update.SendBufferedMessagesFailed", "");
      return lastResult;
    }
    
    // Add more stuff to send buffer from face layers
    while(_sendBuffer.empty() &&
          ( HaveNonLoopingFaceLayersToSend() ||
           _audioClient.IsPlugInActive() ))
    {
      // If we have face layers to send, we _do_ want BufferAudioToSend to
      // buffer audio silence keyframes to keep the clock ticking. If not, we
      // don't need to send audio silence.
      const bool sendSilence = (_faceLayers.empty() ? false : true);
      BufferAudioToSend(sendSilence);
      
      // Increment fake "streaming" time, so we can evaluate below whether
      // it's time to stream out any of the other tracks. Note that it is still
      // relative to the same start time.
      _streamingTime_ms += RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
      
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
    }
    
    // If we just finished buffering all the face layers, send an end of animation message
    if(_startOfAnimationSent &&
       !HaveNonLoopingFaceLayersToSend() &&
       _sendBuffer.empty() &&
       !_endOfAnimationSent &&
       !_audioClient.IsPlugInActive()) {
      lastResult = SendEndOfAnimation(robot);
    }
    
    return lastResult;
  }// StreamFaceLayers()
    
  Result AnimationStreamer::UpdateStream(Robot& robot, Animation* anim, bool storeFace)
  {
    Result lastResult = RESULT_OK;
    
    if(!anim->IsInitialized()) {
      PRINT_NAMED_ERROR("Animation.Update", "Animation must be initialized before it can be played/updated.");
      return RESULT_FAIL;
    }
    
    const TimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    
    //#   if DEBUG_ANIMATIONS
    //    PRINT_NAMED_INFO("Animation.Update", "Current time = %dms", currTime_ms);
    //#   endif
    
    // Grab references to all the tracks
    auto & deviceAudioTrack = anim->GetTrack<DeviceAudioKeyFrame>();
    auto & robotAudioTrack  = anim->GetTrack<RobotAudioKeyFrame>();
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
    
    UpdateNumBytesToSend(robot);
    
    // Send anything still left in the buffer after last Update()
    lastResult = SendBufferedMessages(robot);
    if(RESULT_OK != lastResult) {
      PRINT_NAMED_ERROR("Animation.Update.SendBufferedMessagesFailed", "");
      return lastResult;
    }
    
    // Add more stuff to the send buffer. Note that we are not counting individual
    // keyframes here, but instead _audio_ keyframes (with which we will buffer
    // any co-timed keyframes from other tracks).
    while(_sendBuffer.empty() && anim->HasFramesLeft())
    {
#     if DEBUG_ANIMATIONS
      //PRINT_NAMED_INFO("Animation.Update", "%d bytes left to send this Update.",
      //                 numBytesToSend);
#     endif
      
#     if USE_SOUND_MANAGER_FOR_ROBOT_AUDIO
      // Have to always send an audio frame to keep time, whether that's the next
      // audio sample or a silent frame. This increments "streamingTime"
      // NOTE: Audio frame must be first!
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
      
      if(robotAudioTrack.HasFramesLeft())
      {
        RobotAudioKeyFrame& audioKF = robotAudioTrack.GetCurrentKeyFrame();
        if(audioKF.IsTimeToPlay(_startTime_ms, _streamingTime_ms)) {
          // Tell the audio manager to play the sound indicated by this track
//          auto & audioRef = audioKF.GetAudioRef();
          if (nullptr != _audioClient) {
            _audioClient->PostCozmoEvent(Anki::Cozmo::Audio::EventType::PLAY_VO_COZ_PLAYFUL); // audioRef.audioEvent );
            //  AudioManager::PlayEvent(audioRef.audioEvent, audioRef.volume);
          }
          
#         if PLAY_ROBOT_AUDIO_ON_DEVICE && !defined(ANKI_IOS_BUILD)
          // Queue up audio frame for playing locally if
          // it's not already in the queued and it wasn't already played.
          if ((&audioKF != _lastPlayedOnDeviceRobotAudioKeyFrame) &&
              (_onDeviceRobotAudioKeyFrameQueue.empty() || &audioKF != _onDeviceRobotAudioKeyFrameQueue.back()))
          {
            _onDeviceRobotAudioKeyFrameQueue.push_back(&audioKF);
          }
#         endif
          
          robotAudioTrack.MoveToNextKeyFrame();
        }
      }
      
      // Stream a single audio sample from the audio manager (or silence if there isn't one)
      // (Have to *always* send an audio frame to keep time, whether that's the next
      // audio sample or a silent frame.)
      // NOTE: Audio frame must be first!
      BufferAudioToSend(true);
      
#     endif // USE_SOUND_MANAGER_FOR_ROBOT_AUDIO
      
      // Increment fake "streaming" time, so we can evaluate below whether
      // it's time to stream out any of the other tracks. Note that it is still
      // relative to the same start time.
      _streamingTime_ms += RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
      
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
      
    } // while(buffering frames)
    
#   if USE_SOUND_MANAGER_FOR_ROBOT_AUDIO
#   if PLAY_ROBOT_AUDIO_ON_DEVICE && !defined(ANKI_IOS_BUILD)
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
#   endif
#   endif // USE_SOUND_MANAGER_FOR_ROBOT_AUDIO
    
    // Send an end-of-animation keyframe when done
    if(!anim->HasFramesLeft() && _sendBuffer.empty() &&
       _startOfAnimationSent && !_endOfAnimationSent)
    {
      lastResult = SendEndOfAnimation(robot);
    }
    
    return lastResult;
  } // UpdateStream()
  
  
  Result AnimationStreamer::Update(Robot& robot)
  {
    Result lastResult = RESULT_OK;
    
    bool streamUpdated = false;
    
    // Always keep face alive, unless we have a streaming animation, since we rely on it
    // to do all face updating and we don't want to step on it's hand-designed toes.
    // Wait a 1/2 second before running after we finish the last streaming animation
    // to help reduce stepping on the next animation's toes when we have things
    // sequenced.
    if(_streamingAnimation == nullptr &&
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
          InitStream(_streamingAnimation, _tagCtr);
          
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
        InitStream(_idleAnimation, IdleAnimationTag);
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
    if(!streamUpdated && (HaveNonLoopingFaceLayersToSend() || _audioClient.HasKeyFrameAudioSample()))
    {
      lastResult = StreamFaceLayersOrAudio(robot);
    }
    
    return lastResult;
  } // AnimationStreamer::Update()
  
  
  void AnimationStreamer::UpdateNumBytesToSend(Robot& robot)
  {
    // Compute number of bytes free in robot animation buffer.
    // This is a lower bound since this is computed from a delayed measure
    // of the number of animation bytes already played on the robot.
    s32 totalNumBytesStreamed = robot.GetNumAnimationBytesStreamed();
    s32 totalNumBytesPlayed = robot.GetNumAnimationBytesPlayed();
    bool overflow = (totalNumBytesStreamed < 0) && (totalNumBytesPlayed > 0);
    assert((totalNumBytesStreamed >= totalNumBytesPlayed) || overflow);
    
    s32 minBytesFreeInRobotBuffer = static_cast<size_t>(AnimConstants::KEYFRAME_BUFFER_SIZE) - (totalNumBytesStreamed - totalNumBytesPlayed);
    if (overflow) {
      // Computation for minBytesFreeInRobotBuffer still works out in overflow case
      PRINT_NAMED_INFO("Animation.Update.BytesStreamedOverflow",
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
    
  } // UpdateNumBytesToSend()

  
  Result StreamProceduralFace(Robot& robot,
                              const ProceduralFace& lastFace,
                              const ProceduralFace& nextFace,
                              Animation& liveAnimation)
  {
    const TimeStamp_t lastTime = lastFace.GetTimeStamp();
    const TimeStamp_t nextTime = nextFace.GetTimeStamp();
    
    // Either interpolate from the last procedural face's timestamp if it's not too
    // old, or for a fixed max duration so we get a smooth change but don't
    // queue up tons of frames right now trying to get to the current face
    // (which would cause an unwanted delay).
    const TimeStamp_t maxDuration = 4*IKeyFrame::SAMPLE_LENGTH_MS;
    TimeStamp_t lastInterpTime = lastTime;
    if(nextTime > maxDuration) {
      lastInterpTime = std::max(lastTime, nextTime - maxDuration);
    }
    
    ProceduralFace proceduralFace;
    proceduralFace.SetTimeStamp(lastInterpTime + IKeyFrame::SAMPLE_LENGTH_MS);

    while(proceduralFace.GetTimeStamp() <= nextTime)
    {
      // Calculate next interpolation time
      auto nextInterpFrameTime = proceduralFace.GetTimeStamp() + IKeyFrame::SAMPLE_LENGTH_MS;
      
      // Interpolate based on time
      f32 blendFraction = 1.f;
      // If there are more blending frames after this one actually calculate the blend. Otherwise this is the last
      // frame and we should finish the interpolation
      if (nextInterpFrameTime <= nextTime)
      {
        blendFraction = std::min(1.f, (static_cast<f32>(proceduralFace.GetTimeStamp() - lastInterpTime) /
                                       static_cast<f32>(nextTime - lastInterpTime)));
      }
      
      const bool useSaccades = true;
      proceduralFace.GetParams().Interpolate(lastFace.GetParams(), nextFace.GetParams(), blendFraction, useSaccades);
      
      // Add this procedural face as a keyframe in the live animation
      ProceduralFaceKeyFrame kf(proceduralFace);
      kf.SetIsLive(true);
      if(RESULT_OK != liveAnimation.AddKeyFrame(kf)) {
        PRINT_NAMED_ERROR("AnimationStreamer.StreamProceduralFace.AddFrameFailed", "");
        return RESULT_FAIL;
      }
      
      // Increment the procedural face time for the next interpolated frame
      proceduralFace.SetTimeStamp(nextInterpFrameTime);
    }
    
    return RESULT_OK;
  } // StreamProceduralFace()
  

  
  void AnimationStreamer::SetDefaultParams()
  {
#   define SET_DEFAULT(__NAME__, __VALUE__) \
    SetParam(LiveIdleAnimationParameter::__NAME__,  static_cast<f32>(__VALUE__))
    
    SET_DEFAULT(BlinkCloseTime_ms, 150);
    SET_DEFAULT(BlinkOpenTime_ms, 150);
    
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
    SET_DEFAULT(EyeDartMaxDistance_pix, 3);
    
#   undef SET_DEFAULT
  } // SetDefaultParams()
  
  Result AnimationStreamer::UpdateLiveAnimation(Robot& robot)
  {
#   define GET_PARAM(__TYPE__, __NAME__) GetParam<__TYPE__>(LiveIdleAnimationParameter::__NAME__)
    
    Result lastResult = RESULT_OK;
    
    bool anyFramesAdded = false;
    
    // Use procedural face
    const ProceduralFace& lastFace = robot.GetLastProceduralFace();
//    const TimeStamp_t lastTime = lastFace.GetTimeStamp();
    //const ProceduralFace& nextFace = robot.GetProceduralFace();
    ProceduralFace nextFace(robot.GetProceduralFace());
        
    _nextLookAround_ms -= BS_TIME_STEP;
    
    bool faceSent = false;
    if(nextFace.HasBeenSentToRobot() == false &&
       lastFace.HasBeenSentToRobot() == true)
    {
      lastResult = StreamProceduralFace(robot, lastFace, nextFace, _liveAnimation);
      if(RESULT_OK != lastResult) {
        return lastResult;
      }
      
      robot.MarkProceduralFaceAsSent();
      faceSent = true;
    }
    
    anyFramesAdded = faceSent;

    // Don't start wiggling until we've been idling for a bit and make sure we
    // picking or placing
    if(_isLiveTwitchEnabled &&
       _timeSpentIdling_ms >= GET_PARAM(s32, TimeBeforeWiggleMotions_ms) &&
       !robot.IsPickingOrPlacing())
    {
      // If wheels are available, add a little random movement to keep Cozmo looking alive
      if(!robot.IsMoving() && (_bodyMoveDuration_ms+_bodyMoveSpacing_ms) <= 0 && !robot.GetMoveComponent().AreWheelsLocked())
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
          robot.ShiftEyes(x, y, IKeyFrame::SAMPLE_LENGTH_MS);
        }
        
#       if DEBUG_ANIMATION_STREAMING
        PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.BodyTwitch",
                         "Speed=%d, curvature=%d, duration=%d",
                         speed, curvature, _bodyMoveDuration_ms);
#       endif
        BodyMotionKeyFrame kf(speed, curvature, _bodyMoveDuration_ms);
        kf.SetIsLive(true);
        if(RESULT_OK != _liveAnimation.AddKeyFrame(kf)) {
          PRINT_NAMED_ERROR("AnimationStreamer.UpdateLiveAnimation.AddBodyMotionKeyFrameFailed", "");
          return RESULT_FAIL;
        }
        
        anyFramesAdded = true;
        
        _bodyMoveSpacing_ms = _rng.RandIntInRange(GET_PARAM(s32, BodyMovementSpacingMin_ms),
                                                  GET_PARAM(s32, BodyMovementSpacingMax_ms));
        
      } else {
        _bodyMoveDuration_ms -= BS_TIME_STEP;
      }
      
      // If lift is available, add a little random movement to keep Cozmo looking alive
      if(!robot.IsLiftMoving() && (_liftMoveDuration_ms + _liftMoveSpacing_ms) <= 0 && !robot.GetMoveComponent().IsLiftLocked() && !robot.IsCarryingObject()) {
        _liftMoveDuration_ms = _rng.RandIntInRange(GET_PARAM(s32, LiftMovementDurationMin_ms),
                                                   GET_PARAM(s32, LiftMovementDurationMax_ms));

#       if DEBUG_ANIMATION_STREAMING
        PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.LiftTwitch",
                         "duration=%d", _liftMoveDuration_ms);
#       endif
        LiftHeightKeyFrame kf(GET_PARAM(u8, LiftHeightMean_mm),
                              GET_PARAM(u8, LiftHeightVariability_mm),
                              _liftMoveDuration_ms);
        kf.SetIsLive(true);
        if(RESULT_OK != _liveAnimation.AddKeyFrame(kf)) {
          PRINT_NAMED_ERROR("AnimationStreamer.UpdateLiveAnimation.AddLiftHeightKeyFrameFailed", "");
          return RESULT_FAIL;
        }
        
        anyFramesAdded = true;
        
        _liftMoveSpacing_ms = _rng.RandIntInRange(GET_PARAM(s32, LiftMovementSpacingMin_ms),
                                                  GET_PARAM(s32, LiftMovementSpacingMax_ms));
        
      } else {
        _liftMoveDuration_ms -= BS_TIME_STEP;
      }
      
      // If head is available, add a little random movement to keep Cozmo looking alive
      if(!robot.IsHeadMoving() && (_headMoveDuration_ms+_headMoveSpacing_ms) <= 0 && !robot.GetMoveComponent().IsHeadLocked()) {
        _headMoveDuration_ms = _rng.RandIntInRange(GET_PARAM(s32, HeadMovementDurationMin_ms),
                                                   GET_PARAM(s32, HeadMovementDurationMax_ms));
        const s8 currentAngle_deg = static_cast<s8>(RAD_TO_DEG(robot.GetHeadAngle()));

#       if DEBUG_ANIMATION_STREAMING
        PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.HeadTwitch",
                         "duration=%d", _headMoveDuration_ms);
#       endif
        HeadAngleKeyFrame kf(currentAngle_deg, GET_PARAM(u8, HeadAngleVariability_deg), _headMoveDuration_ms);
        kf.SetIsLive(true);
        if(RESULT_OK != _liveAnimation.AddKeyFrame(kf)) {
          PRINT_NAMED_ERROR("AnimationStreamer.UpdateLiveAnimation.AddHeadAngleKeyFrameFailed", "");
          return RESULT_FAIL;
        }
        
        anyFramesAdded = true;
        
        _headMoveSpacing_ms = _rng.RandIntInRange(GET_PARAM(s32, HeadMovementSpacingMin_ms),
                                                  GET_PARAM(s32, HeadMovementSpacingMax_ms));
        
      } else {
        _headMoveDuration_ms -= BS_TIME_STEP;
      }
      
    } // if(_isLiveTwitchEnabled && _timeSpentIdling_ms >= kTimeBeforeWiggleMotions_ms)
    
    if(anyFramesAdded) {
      // If we add a keyframe after initialization (at which time this animation
      // could have been empty), make sure to mark that we haven't yet sent
      // end of animation.
      _endOfAnimationSent = false;
    }
    
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

