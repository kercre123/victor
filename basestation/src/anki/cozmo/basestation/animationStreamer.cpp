
#include "anki/cozmo/basestation/animation/animationStreamer.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"

#include "util/logging/logging.h"

#define DEBUG_ANIMATION_STREAMING 0

namespace Anki {
namespace Cozmo {

  const std::string AnimationStreamer::LiveAnimation = "_LIVE_";
  
  AnimationStreamer::AnimationStreamer(CannedAnimationContainer& container)
  : _animationContainer(container)
  , _liveAnimation(LiveAnimation)
  , _idleAnimation(&_liveAnimation)
  , _streamingAnimation(nullptr)
  , _timeSpentIdling_ms(0)
  , _isIdling(false)
  , _numLoops(1)
  , _loopCtr(0)
  , _tagCtr(0)
  , _nextBlink_ms(0)
  , _nextLookAround_ms(0)
  , _bodyMoveDuration_ms(0)
  , _liftMoveDuration_ms(0)
  , _headMoveDuration_ms(0)
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
  
  
  Result AnimationStreamer::UpdateLiveAnimation(Robot& robot)
  {
    Result lastResult = RESULT_OK;
    
    // Live animation parameters:
    // TODO: Get all these constants from config somehow
    const s32 kBlinkCloseTime_ms = 150;
    const s32 kBlinkOpenTime_ms  = 100;
    const s32 kBlinkSpacingMinTime_ms = 3000;
    const s32 kBlinkSpacingMaxTime_ms = 4000;
    const s32 kTimeBeforeWiggleMotions_ms = 1000;
    const s32 kBodyMovementDurationMin_ms = 250;
    const s32 kBodyMovementDurationMax_ms = 1000;
    const s32 kBodyMovementSpeedMinMax_mmps = 10;
    const f32 kBodyMovementStraightFraction = 0.5f;
    const f32 kMaxPupilMovement = 0.5f;
    const s32 kLiftMovementDurationMin_ms = 50;
    const s32 kLiftMovementDurationMax_ms = 500;
    const u8  kLiftHeightMean_mm = 35;
    const u8  kLiftHeightVariability_mm = 8;
    const s32 kHeadMovementDurationMin_ms = 50;
    const s32 kHeadMovementDurationMax_ms = 500;
    const u8  kHeadAngleVariability_deg = 3;
    
    // Use procedural face
    const ProceduralFace& lastFace = robot.GetLastProceduralFace();
    const TimeStamp_t lastTime = lastFace.GetTimeStamp();
    const ProceduralFace& nextFace = robot.GetProceduralFace();
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
    else if(_nextBlink_ms <= 0) { // "time to blink"
      PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.Blink", "");
      ProceduralFace crntFace(nextFace.HasBeenSentToRobot() ? nextFace : lastFace);
      ProceduralFace blinkFace(crntFace);
      blinkFace.Blink();
      
      // Close: interpolate from current face to blink face (eyes closed)
      blinkFace.SetTimeStamp(crntFace.GetTimeStamp() + kBlinkCloseTime_ms);
      lastResult = StreamProceduralFace(robot, crntFace, blinkFace, _liveAnimation);
      if(RESULT_OK != lastResult) {
        return lastResult;
      }
      
      // Open: interpolate from blink face (eyes closed) back to current face
      crntFace.SetTimeStamp(blinkFace.GetTimeStamp() + kBlinkOpenTime_ms);
      lastResult = StreamProceduralFace(robot, blinkFace, crntFace, _liveAnimation);
      if(RESULT_OK != lastResult) {
        return lastResult;
      }
      
      // Pick random next time to blink
      _nextBlink_ms = _rng.RandIntInRange(kBlinkSpacingMinTime_ms, kBlinkSpacingMaxTime_ms);
      faceSent = true;
    }
    

    // Don't start wiggling until we've been idling for a bit and make sure we
    // picking or placing
    if(_timeSpentIdling_ms >= kTimeBeforeWiggleMotions_ms && !robot.IsPickingOrPlacing())
    {
      // If wheels are available, add a little random movement to keep Cozmo looking alive
      if(!robot.IsMoving() && _bodyMoveDuration_ms <= 0 && !robot.AreWheelsLocked())
      {
        _bodyMoveDuration_ms = _rng.RandIntInRange(kBodyMovementDurationMin_ms, kBodyMovementDurationMax_ms);
        s16 speed = _rng.RandIntInRange(-kBodyMovementSpeedMinMax_mmps, kBodyMovementSpeedMinMax_mmps);

        // Drive straight sometimes, turn in place the rest of the time
        s16 curvature = s16_MAX; // drive straight
        if(_rng.RandDblInRange(0., 1.) < kBodyMovementStraightFraction) {
          curvature = 0;
        }
        
        // If we haven't already sent a procedural face, use it to point eyes
        // in direction of motion
        if(!faceSent)
        {
          ProceduralFace crntFace(nextFace.HasBeenSentToRobot() ? nextFace : lastFace);
          
          f32 x = 0, y = 0;
          if(curvature == 0) {
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
        
        PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.BodyTwitch",
                         "Speed=%d, curvature=%d, duration=%d",
                         speed, curvature, _bodyMoveDuration_ms);
        BodyMotionKeyFrame kf(speed, curvature, _bodyMoveDuration_ms);
        kf.SetIsLive(true);
        if(RESULT_OK != _liveAnimation.AddKeyFrame(kf)) {
          PRINT_NAMED_ERROR("AnimationStreamer.UpdateLiveAnimation.AddBodyMotionKeyFrameFailed", "");
          return RESULT_FAIL;
        }
      } else {
        _bodyMoveDuration_ms -= BS_TIME_STEP;
      }
      
      // If lift is available, add a little random movement to keep Cozmo looking alive
      if(!robot.IsLiftMoving() && _liftMoveDuration_ms <= 0 && !robot.IsLiftLocked() && !robot.IsCarryingObject()) {
        _liftMoveDuration_ms = _rng.RandIntInRange(kLiftMovementDurationMin_ms, kLiftMovementDurationMax_ms);

        PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.LiftTwitch",
                         "duration=%d", _liftMoveDuration_ms);

        LiftHeightKeyFrame kf(kLiftHeightMean_mm, kLiftHeightVariability_mm, _liftMoveDuration_ms);
        kf.SetIsLive(true);
        if(RESULT_OK != _liveAnimation.AddKeyFrame(kf)) {
          PRINT_NAMED_ERROR("AnimationStreamer.UpdateLiveAnimation.AddLiftHeightKeyFrameFailed", "");
          return RESULT_FAIL;
        }
      } else {
        _liftMoveDuration_ms -= BS_TIME_STEP;
      }
      
      // If head is available, add a little random movement to keep Cozmo looking alive
      if(!robot.IsHeadMoving() && _headMoveDuration_ms <= 0 && !robot.IsHeadLocked()) {
        _headMoveDuration_ms = _rng.RandIntInRange(kHeadMovementDurationMin_ms, kHeadMovementDurationMax_ms);
        const s8 currentAngle_deg = static_cast<s8>(RAD_TO_DEG(robot.GetHeadAngle()));

        PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.HeadTwitch",
                         "duration=%d", _headMoveDuration_ms);

        HeadAngleKeyFrame kf(currentAngle_deg, kHeadAngleVariability_deg, _headMoveDuration_ms);
        kf.SetIsLive(true);
        if(RESULT_OK != _liveAnimation.AddKeyFrame(kf)) {
          PRINT_NAMED_ERROR("AnimationStreamer.UpdateLiveAnimation.AddHeadAngleKeyFrameFailed", "");
          return RESULT_FAIL;
        }
      } else {
        _headMoveDuration_ms -= BS_TIME_STEP;
      }
      
    } // if(_timeSpentIdling_ms >= kTimeBeforeWiggleMotions_ms)
    
    return lastResult;
  }
  
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

