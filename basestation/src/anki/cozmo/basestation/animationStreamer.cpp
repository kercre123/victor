
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
  , _isIdling(false)
  , _numLoops(1)
  , _loopCtr(0)
  {
    
  }

  Result AnimationStreamer::SetStreamingAnimation(const std::string& name, u32 numLoops)
  {
    // Special case: stop streaming the current animation
    if(name.empty()) {
#     if DEBUG_ANIMATION_STREAMING
      PRINT_NAMED_INFO("AnimationStreamer.SetStreamingAnimation",
                       "Stopping streaming of animation '%s'.\n",
                       GetStreamingAnimationName().c_str());
#     endif
      
      _streamingAnimation = nullptr;
      return RESULT_OK;
    }
    
    _streamingAnimation = _animationContainer.GetAnimation(name);
    if(_streamingAnimation == nullptr) {
      return RESULT_FAIL;
    } else {
      
      // Get the animation ready to play
      _streamingAnimation->Init();
      
      _numLoops = numLoops;
      _loopCtr = 0;
      
#     if DEBUG_ANIMATION_STREAMING
      PRINT_NAMED_INFO("AnimationStreamer.SetStreamingAnimation",
                       "Will start streaming '%s' animation %d times.\n",
                       name.c_str(), numLoops);
#     endif
      return RESULT_OK;
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
          _streamingAnimation->Init();
          
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
        _idleAnimation->Init();
        _isIdling = true;
      }
      
      _idleAnimation->Update(robot);
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
    proceduralFace.SetTimeStamp(lastInterpTime);

    while(proceduralFace.GetTimeStamp() < nextTime)
    {
      // Increment interpolation time
      proceduralFace.SetTimeStamp(proceduralFace.GetTimeStamp() + IKeyFrame::SAMPLE_LENGTH_MS);
      
      // Interpolate based on time
      const f32 blendFraction = std::min(1.f, (static_cast<f32>(proceduralFace.GetTimeStamp() - lastInterpTime) /
                                               static_cast<f32>(nextTime - lastInterpTime)));
      
      const bool useSaccades = true;
      proceduralFace.Interpolate(lastFace, nextFace, blendFraction, useSaccades);
      
      // Add this procedural face as a keyframe in the live animation
      ProceduralFaceKeyFrame kf(proceduralFace);
      kf.SetIsLive(true);
      if(RESULT_OK != liveAnimation.AddKeyFrame(kf)) {
        PRINT_NAMED_ERROR("AnimationStreamer.UpdateLiveAnimation.AddFrameFaile", "");
        return RESULT_FAIL;
      }
    }
    
    return RESULT_OK;
  }
  
  Result AnimationStreamer::UpdateLiveAnimation(Robot& robot)
  {
    Result lastResult = RESULT_OK;
    
    // Use procedural face
    const ProceduralFace& lastFace = robot.GetLastProceduralFace();
    const TimeStamp_t lastTime = lastFace.GetTimeStamp();
    const ProceduralFace& nextFace = robot.GetProceduralFace();
    const TimeStamp_t nextTime = nextFace.GetTimeStamp();
    
    _nextBlink_ms -= BS_TIME_STEP;
    
    if(nextFace.HasBeenSentToRobot() == false &&
       lastFace.HasBeenSentToRobot() == true &&
       nextTime > lastTime)
    {
      lastResult = StreamProceduralFace(robot, lastFace, nextFace, _liveAnimation);
      if(RESULT_OK != lastResult) {
        return lastResult;
      }
      
      robot.MarkProceduralFaceAsSent();
    }
    else if(_nextBlink_ms <= 0) { // "time to blink"
      PRINT_NAMED_INFO("AnimationStreamer.UpdateLiveAnimation.Blink", "");
      ProceduralFace crntFace(nextFace.HasBeenSentToRobot() ? nextFace : lastFace);
      ProceduralFace blinkFace(crntFace);
      blinkFace.Blink();
      
      // Close: interpolate from current face to blink face (eyes closed)
      blinkFace.SetTimeStamp(crntFace.GetTimeStamp() + 150);
      lastResult = StreamProceduralFace(robot, crntFace, blinkFace, _liveAnimation);
      if(RESULT_OK != lastResult) {
        return lastResult;
      }
      
      // Open: interpolate from blink face (eyes closed) back to current face
      crntFace.SetTimeStamp(blinkFace.GetTimeStamp() + 100);
      lastResult = StreamProceduralFace(robot, blinkFace, crntFace, _liveAnimation);
      if(RESULT_OK != lastResult) {
        return lastResult;
      }
      
      _nextBlink_ms = _rng.RandIntInRange(3000, 4000); // Pick random next time to blink
      
    }

    
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

