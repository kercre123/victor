
#include "anki/cozmo/basestation/animation/animationStreamer.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "anki/cozmo/basestation/utils/hasSettableParameters_impl.h"

#include "util/logging/logging.h"

#define DEBUG_ANIMATION_STREAMING 0

namespace Anki {
namespace Cozmo {

  const std::string AnimationStreamer::LiveAnimation = "_LIVE_";
  const std::string AnimationStreamer::AnimToolAnimation = "_ANIM_TOOL_";
  
  AnimationStreamer::AnimationStreamer(IExternalInterface* externalInterface,
                                       CannedAnimationContainer& container)
  : HasSettableParameters(externalInterface)
  , _animationContainer(container)
  , _liveAnimation(LiveAnimation)
  , _idleAnimation(nullptr)
  , _streamingAnimation(nullptr)
  , _timeSpentIdling_ms(0)
  , _isIdling(false)
  , _numLoops(1)
  , _loopCtr(0)
  , _tagCtr(0)
  , _isLiveTwitchEnabled(false)
  , _nextBlink_ms(0)
  , _nextLookAround_ms(0)
  , _bodyMoveDuration_ms(0)
  , _liftMoveDuration_ms(0)
  , _headMoveDuration_ms(0)
  , _bodyMoveSpacing_ms(0)
  , _liftMoveSpacing_ms(0)
  , _headMoveSpacing_ms(0)
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
      _streamingAnimation->Init(_tagCtr);
      
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
    if(name.empty()) {
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
  
  
  Result AnimationStreamer::Update(Robot& robot)
  {
    Result lastResult = RESULT_OK;
    
    if(_streamingAnimation != nullptr) {
      _timeSpentIdling_ms = 0;
      
      if(_streamingAnimation->IsFinished()) {
        
        ++_loopCtr;
        
        if(_numLoops == 0 || _loopCtr < _numLoops) {
#         if DEBUG_ANIMATION_STREAMING
          PRINT_NAMED_INFO("AnimationStreamer.Update.Looping",
                           "Finished loop %d of %d of '%s' animation. Restarting.\n",
                           _loopCtr, _numLoops,
                           _streamingAnimation->GetName().c_str());
#         endif
          
          // Reset the animation so it can be played again:
          _streamingAnimation->Init(_tagCtr);
          
        } else {
#         if DEBUG_ANIMATION_STREAMING
          PRINT_NAMED_INFO("AnimationStreamer.Update.FinishedStreaming",
                           "Finished streaming '%s' animation.\n",
                           _streamingAnimation->GetName().c_str());
#         endif
          
          _streamingAnimation = nullptr;
        }
        
      } else {
        lastResult = _streamingAnimation->Update(robot);
        _isIdling = false;
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
      
      if((!robot.IsAnimating() && _idleAnimation->IsFinished()) || !_isIdling) {
#       if DEBUG_ANIMATION_STREAMING
        PRINT_NAMED_INFO("AnimationStreamer.Update.IdleAnimInit",
                         "(Re-)Initializing idle animation: '%s'.\n",
                         _idleAnimation->GetName().c_str());
#       endif
        
        // Just finished playing a loop, or we weren't just idling. Either way,
        // (re-)init the animation so it can be played (again)
        _idleAnimation->Init(IdleAnimationTag);
        _isIdling = true;
        //InitIdleAnimation();
      }
      
      _idleAnimation->Update(robot);
      _timeSpentIdling_ms += BS_TIME_STEP;
    }
    
    return lastResult;
  } // AnimationStreamer::Update()
  
  
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
      proceduralFace.Interpolate(lastFace, nextFace, blendFraction, useSaccades);
      
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
#   define SET_DEFAULT(__NAME__, __VALUE__) SetParam(LiveIdleAnimationParameter::__NAME__,  static_cast<f32>(__VALUE__))
    
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
    SET_DEFAULT(MaxPupilMovement, 0.5f);
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
    SET_DEFAULT(DockSquintEyeHeight, -0.4f);
    SET_DEFAULT(DockSquintEyebrowHeight, -0.4f);
    
#    undef SET_DEFAULT
  }
  
  Result AnimationStreamer::UpdateLiveAnimation(Robot& robot)
  {
#   define GET_PARAM(__TYPE__, __NAME__) GetParam<__TYPE__>(LiveIdleAnimationParameter::__NAME__)
    
    Result lastResult = RESULT_OK;
    
    // Use procedural face
    const ProceduralFace& lastFace = robot.GetLastProceduralFace();
    const TimeStamp_t lastTime = lastFace.GetTimeStamp();
    //const ProceduralFace& nextFace = robot.GetProceduralFace();
    ProceduralFace nextFace(robot.GetProceduralFace());
    
    // Squint the current face while picking/placing to show concentration:
    if(robot.IsPickingOrPlacing()) {
      for(auto whichEye : {ProceduralFace::Left, ProceduralFace::Right}) {
        nextFace.SetParameter(whichEye, ProceduralFace::Parameter::EyeHeight,
                              GET_PARAM(f32, DockSquintEyeHeight));
        nextFace.SetParameter(whichEye, ProceduralFace::Parameter::BrowCenY,
                              GET_PARAM(f32, DockSquintEyebrowHeight));
      }
      // Make sure squinting face gets displayed:
      if(nextFace.GetTimeStamp() < lastFace.GetTimeStamp()+IKeyFrame::SAMPLE_LENGTH_MS) {
        nextFace.SetTimeStamp(nextFace.GetTimeStamp() + IKeyFrame::SAMPLE_LENGTH_MS);
      }
      nextFace.MarkAsSentToRobot(false);
    }
    
    const TimeStamp_t nextTime = nextFace.GetTimeStamp();
    
    _nextBlink_ms -= BS_TIME_STEP;
    _nextLookAround_ms -= BS_TIME_STEP;
    
    bool faceSent = false;
    if(nextFace.HasBeenSentToRobot() == false &&
       lastFace.HasBeenSentToRobot() == true &&
       nextTime >= (lastTime + IKeyFrame::SAMPLE_LENGTH_MS))
    {
      lastResult = StreamProceduralFace(robot, lastFace, nextFace, _liveAnimation);
      if(RESULT_OK != lastResult) {
        return lastResult;
      }
      
      robot.MarkProceduralFaceAsSent();
      faceSent = true;
    }
    else if(_isLiveTwitchEnabled && _nextBlink_ms <= 0) { // "time to blink"
#     if DEBUG_ANIMATION_STREAMING
      PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.Blink", "");
#     endif
      ProceduralFace crntFace(nextFace.HasBeenSentToRobot() ? nextFace : lastFace);
      ProceduralFace blinkFace(crntFace);
      blinkFace.Blink();
      
      // Close: interpolate from current face to blink face (eyes closed)
      blinkFace.SetTimeStamp(crntFace.GetTimeStamp() + GET_PARAM(s32, BlinkCloseTime_ms));
      lastResult = StreamProceduralFace(robot, crntFace, blinkFace, _liveAnimation);
      if(RESULT_OK != lastResult) {
        return lastResult;
      }
      
      // Open: interpolate from blink face (eyes closed) back to current face
      crntFace.SetTimeStamp(blinkFace.GetTimeStamp() + GET_PARAM(s32, BlinkOpenTime_ms));
      lastResult = StreamProceduralFace(robot, blinkFace, crntFace, _liveAnimation);
      if(RESULT_OK != lastResult) {
        return lastResult;
      }
      
      // Pick random next time to blink
      _nextBlink_ms = _rng.RandIntInRange(GET_PARAM(s32, BlinkSpacingMinTime_ms),
                                          GET_PARAM(s32, BlinkSpacingMaxTime_ms));
      faceSent = true;
    }
    

    // Don't start wiggling until we've been idling for a bit and make sure we
    // picking or placing
    if(_isLiveTwitchEnabled &&
       _timeSpentIdling_ms >= GET_PARAM(s32, TimeBeforeWiggleMotions_ms) &&
       !robot.IsPickingOrPlacing())
    {
      // If wheels are available, add a little random movement to keep Cozmo looking alive
      if(!robot.IsMoving() && (_bodyMoveDuration_ms+_bodyMoveSpacing_ms) <= 0 && !robot.AreWheelsLocked())
      {
        _bodyMoveDuration_ms = _rng.RandIntInRange(GET_PARAM(s32, BodyMovementDurationMin_ms),
                                                   GET_PARAM(s32, BodyMovementDurationMax_ms));
        s16 speed = _rng.RandIntInRange(-GET_PARAM(s32, BodyMovementSpeedMinMax_mmps),
                                         GET_PARAM(s32, BodyMovementSpeedMinMax_mmps));

        // Drive straight sometimes, turn in place the rest of the time
        s16 curvature = s16_MAX; // drive straight
        if(_rng.RandDblInRange(0., 1.) > GET_PARAM(f32, BodyMovementStraightFraction)) {
          curvature = 0;
        }
        
        // If we haven't already sent a procedural face, use it to point eyes
        // in direction of motion
        if(!faceSent)
        {
          ProceduralFace crntFace(nextFace.HasBeenSentToRobot() ? nextFace : lastFace);
          
          f32 x = 0, y = 0;
          if(curvature == 0) {
            const f32 kMaxPupilMovement = GET_PARAM(f32, MaxPupilMovement);
            x = (speed < 0 ? _rng.RandDblInRange(-kMaxPupilMovement, 0.) : _rng.RandDblInRange(0., kMaxPupilMovement));
            y = _rng.RandDblInRange(-kMaxPupilMovement, kMaxPupilMovement);
          }
          for(auto whichEye : {ProceduralFace::Left, ProceduralFace::Right})
          {
            crntFace.SetParameter(whichEye, ProceduralFace::Parameter::PupilCenX, x);
            crntFace.SetParameter(whichEye, ProceduralFace::Parameter::PupilCenY, y);
          }
          
          ProceduralFaceKeyFrame kf(crntFace);
          kf.SetIsLive(true);
          if(RESULT_OK != _liveAnimation.AddKeyFrame(kf)) {
            PRINT_NAMED_ERROR("AnimationStreamer.UpdateLiveAnimation.AddTurnLookProcFaceFrameFailed", "");
            return RESULT_FAIL;
          }
        } // if(!faceSent)
        
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
        
        _bodyMoveSpacing_ms = _rng.RandIntInRange(GET_PARAM(s32, BodyMovementSpacingMin_ms),
                                                  GET_PARAM(s32, BodyMovementSpacingMax_ms));
        
      } else {
        _bodyMoveDuration_ms -= BS_TIME_STEP;
      }
      
      // If lift is available, add a little random movement to keep Cozmo looking alive
      if(!robot.IsLiftMoving() && (_liftMoveDuration_ms + _liftMoveSpacing_ms) <= 0 && !robot.IsLiftLocked() && !robot.IsCarryingObject()) {
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
        
        _liftMoveSpacing_ms = _rng.RandIntInRange(GET_PARAM(s32, LiftMovementSpacingMin_ms),
                                                  GET_PARAM(s32, LiftMovementSpacingMax_ms));
        
      } else {
        _liftMoveDuration_ms -= BS_TIME_STEP;
      }
      
      // If head is available, add a little random movement to keep Cozmo looking alive
      if(!robot.IsHeadMoving() && (_headMoveDuration_ms+_headMoveSpacing_ms) <= 0 && !robot.IsHeadLocked()) {
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
  

  
} // namespace Cozmo
} // namespace Anki

