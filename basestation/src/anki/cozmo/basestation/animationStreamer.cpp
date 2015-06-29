
#include "anki/cozmo/basestation/animationStreamer.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/util/logging/logging.h"

#define DEBUG_ANIMATION_STREAMING 1

namespace Anki {
namespace Cozmo {

  AnimationStreamer::AnimationStreamer(CannedAnimationContainer& container)
  : _animationContainer(container)
  , _streamingAnimation(nullptr)
  , _numLoops(1)
  {
    
  }

  Result AnimationStreamer::SetStreamingAnimation(const std::string& name, u32 numLoops)
  {
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

  
  
  Result AnimationStreamer::Update(Robot& robot)
  {
    Result lastResult = RESULT_OK;
    
    if(_streamingAnimation != nullptr) {
      if(_streamingAnimation->IsFinished()) {
        
        // Send an end-of-animation keyframe
        MessageAnimKeyFrame_EndOfAnimation endMsg;
        robot.SendMessage(endMsg);
        
        ++_loopCtr;
        
        if(_numLoops == 0 || _loopCtr < _numLoops) {
#         if DEBUG_ANIMATION_STREAMING
          PRINT_NAMED_INFO("CannedAnimationContainer.Update.Looping",
                           "Finished loop %d of %d of '%s' animation. Restarting.\n",
                           _loopCtr, _numLoops,
                           _streamingAnimation->GetName().c_str());
#         endif
          
          // Reset the animation so it can be played again:
          _streamingAnimation->Init();
          
        } else {
#         if DEBUG_ANIMATION_STREAMING
          PRINT_NAMED_INFO("CannedAnimationContainer.Update.FinishedStreaming",
                           "Finished streaming '%s' animation.\n",
                           _streamingAnimation->GetName().c_str());
#         endif
          
          _streamingAnimation = nullptr;
        }
        
      } else {
        lastResult = _streamingAnimation->Update(robot);
      }
    }
    
    return lastResult;
  } // AnimationStreamer::Update()
  
  
} // namespace Cozmo
} // namespace Anki

