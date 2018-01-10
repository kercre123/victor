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

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/utils/timer.h"
#include "cozmoAnim/animation/animationStreamer.h"
//#include "cozmoAnim/animation/trackLayerManagers/faceLayerManager.h"

#include "cozmoAnim/animation/cannedAnimationContainer.h"
#include "cozmoAnim/animation/faceAnimationManager.h"
#include "cozmoAnim/animation/proceduralFaceDrawer.h"
#include "cozmoAnim/animation/trackLayerComponent.h"
#include "cozmoAnim/audio/animationAudioClient.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/animProcessMessages.h"
#include "cozmoAnim/cozmoAnimContext.h"
#include "cozmoAnim/robotDataLoader.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/types/animationTypes.h"
#include "osState/osState.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine_sendAnimToEngine_helper.h"
#include "clad/robotInterface/messageEngineToRobot_sendAnimToRobot_helper.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#define DEBUG_ANIMATION_STREAMING 0
#define DEBUG_ANIMATION_STREAMING_AUDIO 0

namespace Anki {
namespace Cozmo {
  
  namespace{
    
  const char* kLogChannelName = "Animations";

  // Specifies how often to send AnimState message
  static const u32 kAnimStateReportingPeriod_tics = 2;

  // Default time to wait before forcing KeepFaceAlive() after the latest stream has stopped
  const f32 kDefaultLongEnoughSinceLastStreamTimeout_s = 0.5f;
  
  CONSOLE_VAR(bool, kFullAnimationAbortOnAudioTimeout, "AnimationStreamer", false);
  CONSOLE_VAR(u32, kAnimationAudioAllowedBufferTime_ms, "AnimationStreamer", 250);

  // Whether or not to display themal throttling indicator on face
  CONSOLE_VAR(bool, kDisplayThermalThrottling, "AnimationStreamer", true);

  // Overrides whatever faces we're sending with a 3-stripe test pattern
  // (seems more related to the other ProceduralFace console vars, so putting it in that group instead)
  CONSOLE_VAR(bool, kProcFace_DisplayTestPattern, "ProceduralFace", false);
    
  } // namespace
  
  AnimationStreamer::AnimationStreamer(const CozmoAnimContext* context)
  : _context(context)
  , _animationContainer(*(_context->GetDataLoader()->GetCannedAnimations()))
  , _trackLayerComponent(new TrackLayerComponent(context))
  , _lockedTracks(0)
  , _tracksInUse(0)
  , _audioClient( new Audio::AnimationAudioClient(context->GetAudioController()) )
  , _longEnoughSinceLastStreamTimeout_s(kDefaultLongEnoughSinceLastStreamTimeout_s)
  , _numTicsToSendAnimState(kAnimStateReportingPeriod_tics)
  {
    _proceduralAnimation = new Animation(EnumToString(AnimConstants::PROCEDURAL_ANIM));
    _proceduralAnimation->SetIsLive(true);
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
    
    _faceDrawBuf.Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
    _procFaceImg.Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
    _faceImageBinary.Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
    _faceImageRGB565.Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
   

    // TODO: _Might_ need to disable this eventually if there are conflicts
    //       with wake up animations or something on startup. The only reason
    //       this is here is to make sure there's something on the face and
    //       currently there's no face animation that the engine automatically
    //       initiates on startup.
    // 
    // Turn on neutral face by default at startup
    // Otherwise face is blank until an animation is played.
    SetStreamingAnimation(_neutralFaceAnimation, kNotAnimatingTag);

    return RESULT_OK;
  }
  
  
  const Animation* AnimationStreamer::GetCannedAnimation(const std::string& name) const
  {
    return _animationContainer.GetAnimation(name);
  }
  
  AnimationStreamer::~AnimationStreamer()
  {
    Util::SafeDelete(_proceduralAnimation);

    FaceDisplay::removeInstance();
  }
  
  Result AnimationStreamer::SetStreamingAnimation(const std::string& name, Tag tag, u32 numLoops, bool interruptRunning)
  {
    // Special case: stop streaming the current animation
    if(name.empty()) {
      if(DEBUG_ANIMATION_STREAMING) {
        PRINT_CH_DEBUG(kLogChannelName, "AnimationStreamer.SetStreamingAnimation.StoppingCurrent",
                       "Stopping streaming of animation '%s'.",
                       GetStreamingAnimationName().c_str());
      }

      Abort();
      return RESULT_OK;
    }
    
    return SetStreamingAnimation(_animationContainer.GetAnimation(name), tag, numLoops, interruptRunning, false);
  }
  
  Result AnimationStreamer::SetStreamingAnimation(Animation* anim, Tag tag, u32 numLoops, bool interruptRunning, bool isInternalAnim)
  {
    if(DEBUG_ANIMATION_STREAMING)
    {
      PRINT_CH_DEBUG(kLogChannelName, 
                     "AnimationStreamer.SetStreamingAnimation", "Name:%s Tag:%d NumLoops:%d",
                     anim != nullptr ? anim->GetName().c_str() : "NULL", tag, numLoops);
    }
    
    const bool wasStreamingSomething = nullptr != _streamingAnimation;
    
    if(wasStreamingSomething)
    {
      if(nullptr != anim && !interruptRunning) {
        PRINT_CH_INFO(kLogChannelName,
                      "AnimationStreamer.SetStreamingAnimation.NotInterrupting",
                      "Already streaming %s, will not interrupt with %s",
                      _streamingAnimation->GetName().c_str(),
                      anim->GetName().c_str());
        return RESULT_FAIL;
      }
      
      PRINT_NAMED_WARNING("AnimationStreamer.SetStreamingAnimation.Aborting",
                          "Animation %s is interrupting animation %s",
                          anim != nullptr ? anim->GetName().c_str() : "NULL",
                          _streamingAnimation->GetName().c_str());

      Abort();                          
    }
    
    _streamingAnimation = anim;
    if(_streamingAnimation == nullptr) {
      return RESULT_OK;
    }

    _wasAnimationInterruptedWithNothing = false;
  
    // Get the animation ready to play
    InitStream(_streamingAnimation, tag);
    
    _numLoops = numLoops;
    _loopCtr = 0;

    _playingInternalAnim = isInternalAnim;
    
    if(DEBUG_ANIMATION_STREAMING) {
      PRINT_CH_DEBUG(kLogChannelName, 
                     "AnimationStreamer.SetStreamingAnimation",
                     "Will start streaming '%s' animation %d times with tag=%d.",
                     _streamingAnimation->GetName().c_str(), numLoops, tag);
    }
    
    return RESULT_OK;

  }
  
  Result AnimationStreamer::SetProceduralFace(const ProceduralFace& face, u32 duration_ms)
  {
    DEV_ASSERT(nullptr != _proceduralAnimation, "AnimationStreamer.SetProceduralFace.NullProceduralAnimation");
    
    // Always add one keyframe
    ProceduralFaceKeyFrame keyframe(face);
    Result result = _proceduralAnimation->AddKeyFrameToBack(keyframe);
    
    // Add a second one later to interpolate to, if duration is longer than one keyframe
    if(RESULT_OK == result && duration_ms > ANIM_TIME_STEP_MS)
    {
      keyframe.SetTriggerTime(duration_ms-ANIM_TIME_STEP_MS);
      result = _proceduralAnimation->AddKeyFrameToBack(keyframe);
    }

    if(!(ANKI_VERIFY(RESULT_OK == result, "AnimationStreamer.SetProceduralFace.FailedToCreateAnim", "")))
    {
      return result;
    }
    
    // ProceduralFace is always played as an "internal" animation since it's not considered 
    // a regular animation by the engine so we don't need to send AnimStarted and AnimEnded
    // messages for it.
    result = SetStreamingAnimation(_proceduralAnimation, 0);
    
    return result;
  }

  void AnimationStreamer::Process_displayFaceImageChunk(const Anki::Cozmo::RobotInterface::DisplayFaceImageBinaryChunk& msg) 
  {
    // Expand the bit-packed msg.faceData (every bit == 1 pixel) to byte array (every byte == 1 pixel)
    static const u32 kExpectedNumPixels = FACE_DISPLAY_NUM_PIXELS/2;
    static const u32 kDataLength = sizeof(msg.faceData);
    static_assert(8 * kDataLength == kExpectedNumPixels, "Mismatched face image and bit image sizes");

    if (msg.imageId != _faceImageId) {
      if (_faceImageChunksReceivedBitMask != 0) {
        PRINT_NAMED_WARNING("AnimationStreamer.Process_displayFaceImageChunk.UnfinishedFace", 
                            "Overwriting ID %d with ID %d", 
                            _faceImageId, msg.imageId);
      }
      _faceImageId = msg.imageId;
      _faceImageChunksReceivedBitMask = 1 << msg.chunkIndex;
    } else {
      _faceImageChunksReceivedBitMask |= 1 << msg.chunkIndex;
    }
    
    uint8_t* imageData_i = _faceImageBinary.GetDataPointer();

    uint32_t destI = msg.chunkIndex * kExpectedNumPixels;
    
    for (int i = 0; i < kDataLength; ++i)
    {
      uint8_t currentByte = msg.faceData[i];
        
      for (uint8_t bit = 0; bit < 8; ++bit)
      {
        imageData_i[destI] = ((currentByte & 0x80) > 0) ? 255 : 0;
        ++destI;
        currentByte = (uint8_t)(currentByte << 1);
      }
    }
    assert(destI == kExpectedNumPixels * (1+msg.chunkIndex));
    
    if (_faceImageChunksReceivedBitMask == kAllFaceImageChunksReceivedMask) {
      //PRINT_CH_DEBUG(kLogChannelName, "AnimationStreamer.Process_displayFaceImageChunk.CompleteFaceReceived", "");
      SetFaceImage(_faceImageBinary, msg.duration_ms);
      _faceImageId = 0;
      _faceImageChunksReceivedBitMask = 0;
    }
  }

  void AnimationStreamer::Process_displayFaceImageChunk(const Anki::Cozmo::RobotInterface::DisplayFaceImageRGBChunk& msg) 
  {
    if (msg.imageId != _faceImageRGBId) {
      if (_faceImageRGBChunksReceivedBitMask != 0) {
        PRINT_NAMED_WARNING("AnimationStreamer.Process_displayFaceImageRGBChunk.UnfinishedFace", 
                            "Overwriting ID %d with ID %d", 
                            _faceImageRGBId, msg.imageId);
      }
      _faceImageRGBId = msg.imageId;
      _faceImageRGBChunksReceivedBitMask = 1 << msg.chunkIndex;
    } else {
      _faceImageRGBChunksReceivedBitMask |= 1 << msg.chunkIndex;
    }
    
    static const u16 kMaxNumPixelsPerChunk = sizeof(msg.faceData) / sizeof(msg.faceData[0]);
    const auto numPixels = std::min(msg.numPixels, kMaxNumPixelsPerChunk);
    std::copy_n(msg.faceData, numPixels, _faceImageRGB565.GetRawDataPointer() + (msg.chunkIndex * kMaxNumPixelsPerChunk) );
    
    if (_faceImageRGBChunksReceivedBitMask == kAllFaceImageRGBChunksReceivedMask) {
      //PRINT_CH_DEBUG(kLogChannelName, "AnimationStreamer.Process_displayFaceImageRGBChunk.CompleteFaceReceived", "");
      SetFaceImage(_faceImageRGB565, msg.duration_ms);
      _faceImageRGBId = 0;
      _faceImageRGBChunksReceivedBitMask = 0;
    }
  }

  Result AnimationStreamer::SetFaceImage(const Vision::Image& img, u32 duration_ms)
  {
    // Create RGB565 image from grey scale
    // TODO: Set to some default or user-defined color?
    Vision::ImageRGB565 imgRGB565(img);

    return SetFaceImage(imgRGB565, duration_ms);
  }


  Result AnimationStreamer::SetFaceImage(const Vision::ImageRGB565& imgRGB565, u32 duration_ms)
  {
    DEV_ASSERT(nullptr != _proceduralAnimation, "AnimationStreamer.SetFaceImage.NullProceduralAnimation");
    DEV_ASSERT(imgRGB565.IsContinuous(), "AnimationStreamer.SetFaceImage.ImageIsNotContinuous");
    
    FaceAnimationManager* faceAnimMgr = FaceAnimationManager::getInstance();
    
    // Is proceduralAnimation already playing a FaceAnimationKeyFrame?
    auto& faceAnimTrack = _proceduralAnimation->GetTrack<FaceAnimationKeyFrame>();
    bool hasFaceAnimKeyFrame = !faceAnimTrack.IsEmpty();

    // Clear FaceAnimationMAnager if not already playing
    if (!hasFaceAnimKeyFrame) {
      faceAnimMgr->ClearAnimation(FaceAnimationManager::ProceduralAnimName);
    }

    Result result = faceAnimMgr->AddImage(FaceAnimationManager::ProceduralAnimName, imgRGB565, duration_ms);
    if(!(ANKI_VERIFY(RESULT_OK == result, "AnimationStreamer.SetFaceImage.AddImageFailed", "")))
    {
      return result;
    }

    // Add keyframe if one isn't already there
    if (!hasFaceAnimKeyFrame) {
      // Trigger time of keyframe is 0 since we want it to start playing immediately
      result = _proceduralAnimation->AddKeyFrameToBack(FaceAnimationKeyFrame(FaceAnimationManager::ProceduralAnimName));
      if(!(ANKI_VERIFY(RESULT_OK == result, "AnimationStreamer.SetFaceImage.FailedToAddKeyFrame", "")))
      {
        return result;
      }
    }
    
    if (_streamingAnimation != _proceduralAnimation) {
      result = SetStreamingAnimation(_proceduralAnimation, 0);
    }
    return result;
    
  }
  
  void AnimationStreamer::Abort()
  {
    if (nullptr != _streamingAnimation)
    {
      PRINT_CH_INFO(kLogChannelName,
                    "AnimationStreamer.Abort",
                    "Tag=%d %s hasFramesLeft=%d startSent=%d endSent=%d",
                    _tag,
                    _streamingAnimation->GetName().c_str(),
                    _streamingAnimation->HasFramesLeft(),
                    _startOfAnimationSent,
                    _endOfAnimationSent);
      
      if (_startOfAnimationSent) {
        SendEndOfAnimation(true);
      }

      EnableBackpackAnimationLayer(false);

      _audioClient->StopCozmoEvent();

      if (_streamingAnimation == _proceduralAnimation) {
        _proceduralAnimation->Clear();
        FaceAnimationManager::getInstance()->ClearAnimation(FaceAnimationManager::ProceduralAnimName);
      }

      // Reset the current FaceAnimationKeyFrame if there is one.
      // Note: This is currently the only keyframe that modifies a variable
      // as it's read and needs to be reset before the next time it's read,
      // which is why we're not resetting all tracks in the same way
      auto & faceAnimTrack = _streamingAnimation->GetTrack<FaceAnimationKeyFrame>();
      if (faceAnimTrack.HasFramesLeft()) {
        auto & faceKeyFrame = faceAnimTrack.GetCurrentKeyFrame();
        faceKeyFrame.Reset();
      }

      // Reset animation pointer
      _streamingAnimation = nullptr;

      // If we get to KeepFaceAlive with this flag set, we'll stream neutral face for safety.
      _wasAnimationInterruptedWithNothing = true;
    }
  } // Abort()

  
  
  Result AnimationStreamer::InitStream(Animation* anim, Tag withTag)
  {
    Result lastResult = anim->Init();
    if(lastResult == RESULT_OK)
    {
     
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
      _trackLayerComponent->RemoveKeepFaceAlive(3*ANIM_TIME_STEP_MS);
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
            res = AnimProcessMessages::SendAnimToRobot(*msg);
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
    if(kProcFace_DisplayTestPattern)
    {
      // Display three color strips increasing in brightness from left to right
      for(int i=0; i<FACE_DISPLAY_HEIGHT/3; ++i)
      {
        Vision::PixelRGB565* red_i   = _faceDrawBuf.GetRow(i);
        Vision::PixelRGB565* green_i = _faceDrawBuf.GetRow(i + FACE_DISPLAY_HEIGHT/3);
        Vision::PixelRGB565* blue_i  = _faceDrawBuf.GetRow(i + 2*FACE_DISPLAY_HEIGHT/3);
        for(int j=0; j<FACE_DISPLAY_WIDTH; ++j)
        {
          const u8 value = Util::numeric_cast_clamped<u8>(std::round((f32)j/(f32)FACE_DISPLAY_WIDTH * 255.f));
          red_i[j]   = Vision::PixelRGB565(value, 0, 0);
          green_i[j] = Vision::PixelRGB565(0, value, 0);
          blue_i[j]  = Vision::PixelRGB565(0, 0, value);
        }
      }
    }
    else
    {
      DEV_ASSERT(_context != nullptr, "AnimationStreamer.BufferFaceToSend.NoContext");
      DEV_ASSERT(_context->GetRandom() != nullptr, "AnimationStreamer.BufferFaceToSend.NoRNGinContext");
      ProceduralFaceDrawer::DrawFace(procFace, *_context->GetRandom(), _procFaceImg);
      _faceDrawBuf.SetFromImageRGB(_procFaceImg);
    }
    
    BufferFaceToSend(_faceDrawBuf);
  }
  
  void AnimationStreamer::BufferFaceToSend(Vision::ImageRGB565& faceImg565, bool allowOverlay)
  {
    DEV_ASSERT_MSG(faceImg565.GetNumCols() == FACE_DISPLAY_WIDTH &&
                   faceImg565.GetNumRows() == FACE_DISPLAY_HEIGHT,
                   "AnimationStreamer.BufferFaceToSend.InvalidImageSize",
                   "Got %d x %d. Expected %d x %d",
                   faceImg565.GetNumCols(), faceImg565.GetNumRows(),
                   FACE_DISPLAY_WIDTH, FACE_DISPLAY_HEIGHT);
    

    // Draw red square in corner of face if thermal throttling
    if (kDisplayThermalThrottling && 
        allowOverlay &&
        OSState::getInstance()->IsThermalThrottling()) {

      const Rectangle<f32> rect( 0, 0, 20, 20);
      const ColorRGBA pixel(1.f, 0.f, 0.f);
      faceImg565.DrawFilledRect(rect, pixel);
    }

    FaceDisplay::getInstance()->DrawToFace(faceImg565);
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

    if (!RobotInterface::SendAnimToRobot(msg)) {
      return RESULT_FAIL;
    }

    return RESULT_OK;
  }
  
  Result AnimationStreamer::SendStartOfAnimation()
  {
    DEV_ASSERT(!_startOfAnimationSent, "AnimationStreamer.SendStartOfAnimation.AlreadySent");
    DEV_ASSERT(_streamingAnimation != nullptr, "AnimationStreamer.SendStartOfAnimation.NullAnim");
    const std::string& streamingAnimName = _streamingAnimation->GetName();

    if(DEBUG_ANIMATION_STREAMING) {
      PRINT_CH_DEBUG(kLogChannelName,
                     "AnimationStreamer.SendStartOfAnimation", "Tag=%d, Name=%s, loopCtr=%d",
                     _tag, streamingAnimName.c_str(), _loopCtr);
    }

    if (_loopCtr == 0) {
      // Don't actually send start message for proceduralFace or neutralFace anims since
      // they weren't requested by engine
      if (!_playingInternalAnim) {
        AnimationStarted startMsg;
        memcpy(startMsg.animName, streamingAnimName.c_str(), streamingAnimName.length());
        startMsg.animName_length = streamingAnimName.length();
        startMsg.tag = _tag;
        if (!RobotInterface::SendAnimToEngine(startMsg)) {
          return RESULT_FAIL;
        }
      }
    }
  
    _startOfAnimationSent = true;
    _endOfAnimationSent = false;

    return RESULT_OK;
  }
  
  // TODO: Is this actually being called at the right time?
  //       Need to call this after triggerTime+durationTime of last keyframe has expired.
  Result AnimationStreamer::SendEndOfAnimation(bool abortingAnim)
  {
    DEV_ASSERT(_startOfAnimationSent && !_endOfAnimationSent, "AnimationStreamer.SendEndOfAnimation.StartNotSentOrEndAlreadySent");
    DEV_ASSERT(_streamingAnimation != nullptr, "AnimationStreamer.SendStartOfAnimation.NullAnim");
    const std::string& streamingAnimName = _streamingAnimation->GetName();

    if(DEBUG_ANIMATION_STREAMING) {
      PRINT_CH_INFO(kLogChannelName,
                    "AnimationStreamer.SendEndOfAnimation", "Tag=%d, Name=%s, t=%dms, loopCtr=%d, numLoops=%d",
                    _tag, streamingAnimName.c_str(), _streamingTime_ms - _startTime_ms, _loopCtr, _numLoops);
    }
    
    if (abortingAnim || (_loopCtr == _numLoops - 1)) {
      // Don't actually send end message for proceduralFace or neutralFace anims since
      // they weren't requested by engine
      if (!_playingInternalAnim) {
        AnimationEnded endMsg;
        memcpy(endMsg.animName, streamingAnimName.c_str(), streamingAnimName.length());
        endMsg.animName_length = streamingAnimName.length();
        endMsg.tag = _tag;
        endMsg.wasAborted = abortingAnim;
        if (!RobotInterface::SendAnimToEngine(endMsg)) {
          return RESULT_FAIL;
        }
      }
    }
    
    _endOfAnimationSent = true;
    _startOfAnimationSent = false;
    
    // Every time we end an animation we should also re-enable BPL_USER layer on robot
    EnableBackpackAnimationLayer(false);
    
    return RESULT_OK;
  } // SendEndOfAnimation()


  Result AnimationStreamer::StreamLayers()
  {
    Result lastResult = RESULT_OK;
    
    // Add more stuff to send buffer from various layers
    if(_trackLayerComponent->HaveLayersToSend())
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
      _streamingTime_ms += ANIM_TIME_STEP_MS;
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
      _audioClient->PlayAudioKeyFrame( robotAudioTrack.GetCurrentKeyFrame() );
      robotAudioTrack.MoveToNextKeyFrame();
    }

    // Send keyframes at appropriates times
    if (anim->HasFramesLeft())
    {
      
      if(DEBUG_ANIMATION_STREAMING) {
        // Very verbose!
        //PRINT_CH_INFO(kLogChannelName,
        //              "Animation.Update", "%d bytes left to send this Update.",
        //              numBytesToSend);
      }
      
      // Apply any track layers to the animation
      TrackLayerComponent::LayeredKeyFrames layeredKeyFrames;
      _trackLayerComponent->ApplyLayersToAnim(anim,
                                              _startTime_ms,
                                              _streamingTime_ms,
                                              layeredKeyFrames,
                                              storeFace);
            
#     if DEBUG_ANIMATION_STREAMING
#       define DEBUG_STREAM_KEYFRAME_MESSAGE(__KF_NAME__) \
                  PRINT_CH_INFO(kLogChannelName, 
                                "AnimationStreamer.UpdateStream", \
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

        AnimationEvent eventMsg;
        eventMsg.event_id = eventKeyFrame.GetAnimEvent();
        eventMsg.timestamp = currTime_ms;
        eventMsg.tag = _tag;
        RobotInterface::SendAnimToEngine(eventMsg);

        eventTrack.MoveToNextKeyFrame();
      }
      
      // Non-procedural faces (raw pixel data/images) take precedence over procedural faces (parameterized faces
      // like idles, keep alive, or normal animated faces)
      if(faceAnimTrack.HasFramesLeft() && !IsTrackLocked((u8)AnimTrackFlag::FACE_IMAGE_TRACK)) {
        auto & faceKeyFrame = faceAnimTrack.GetCurrentKeyFrame();
        if(faceKeyFrame.IsTimeToPlay(_streamingTime_ms - _startTime_ms))
        {
          const bool gotImage = faceKeyFrame.GetFaceImage(_faceDrawBuf);
          if (gotImage) {
            DEBUG_STREAM_KEYFRAME_MESSAGE("FaceAnimation");
            BufferFaceToSend(_faceDrawBuf, false);  // Don't overwrite FaceAnimationKeyFrame images
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
      _streamingTime_ms += ANIM_TIME_STEP_MS;
      
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
    const bool longEnoughSinceStream  = (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - _lastStreamTime) > _longEnoughSinceLastStreamTimeout_s;
    
    if(!haveStreamingAnimation &&
       haveStreamedAnything &&
       longEnoughSinceStream)
    {
      // If we were interrupted from streaming an animation and we've met all the
      // conditions to even be in this function, then we should make sure we've
      // got neutral face back on the screen
      if(_wasAnimationInterruptedWithNothing) {
        SetStreamingAnimation(_neutralFaceAnimation, kNotAnimatingTag);
        _wasAnimationInterruptedWithNothing = false;
      }
      
      _trackLayerComponent->KeepFaceAlive(_liveAnimParams);
    }
    
    if(_streamingAnimation != nullptr) {
      
      if(IsFinished(_streamingAnimation)) {
        
        ++_loopCtr;
        
        if(_numLoops == 0 || _loopCtr < _numLoops) {
         if(DEBUG_ANIMATION_STREAMING) {
           PRINT_CH_INFO(kLogChannelName,
                         "AnimationStreamer.Update.Looping",
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
            PRINT_CH_INFO(kLogChannelName,
                          "AnimationStreamer.Update.FinishedStreaming",
                          "Finished streaming '%s' animation.",
                          _streamingAnimation->GetName().c_str());
          }
          
          _streamingAnimation = nullptr;
        }
        
      }
      else {
        // We do want to store this face to the robot since it's coming from an actual animation
        lastResult = UpdateStream(_streamingAnimation, true);
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
    

    // Send animState message
    if (--_numTicsToSendAnimState == 0) {
      AnimationState msg;
      msg.numProcAnimFaceKeyframes = FaceAnimationManager::getInstance()->GetNumFrames(FaceAnimationManager::ProceduralAnimName);
      msg.lockedTracks             = _lockedTracks;
      msg.tracksInUse              = _tracksInUse;

      RobotInterface::SendAnimToEngine(msg);
      _numTicsToSendAnimState = kAnimStateReportingPeriod_tics;
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
        RobotInterface::SendAnimToRobot(std::move(msg));
      }
      
      if(whichTracks & (u8)AnimTrackFlag::LIFT_TRACK)
      {
        RobotInterface::MoveLift msg;
        msg.speed_rad_per_sec = 0;
        RobotInterface::SendAnimToRobot(std::move(msg));
      }
      
      if(whichTracks & (u8)AnimTrackFlag::BODY_TRACK)
      {
        RobotInterface::DriveWheels msg;
        msg.lwheel_speed_mmps = 0;
        msg.rwheel_speed_mmps = 0;
        msg.lwheel_accel_mmps2 = 0;
        msg.rwheel_accel_mmps2 = 0;
        RobotInterface::SendAnimToRobot(std::move(msg));
      }
      
      _tracksInUse &= ~whichTracks;
    }
  }
  
  
} // namespace Cozmo
} // namespace Anki

