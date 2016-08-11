/**
 * File: drivingAnimationHandler.h
 *
 * Author: Al Chaussee
 * Date:   5/6/2016
 *
 * Description: Handles playing animations while driving
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_DRIVING_ANIMATION_HANDLER_H
#define ANKI_COZMO_DRIVING_ANIMATION_HANDLER_H

#include "anki/common/types.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationKeyFrames.h"
#include "util/signals/simpleSignal_fwd.h"
#include "clad/types/animationTrigger.h"
#include <vector>

namespace Anki {
  namespace Cozmo {
  
    class Robot;
    
    class DrivingAnimationHandler
    {
      public:
      
        // Subscribes to ActionCompleted and SetDrivingAnimations messages
        DrivingAnimationHandler(Robot& robot);
      
        // Container for the various driving animations
        struct DrivingAnimations
        {
          AnimationTrigger drivingStartAnim;
          AnimationTrigger drivingLoopAnim;
          AnimationTrigger drivingEndAnim;
        };
      
        // Sets the driving animations
        void PushDrivingAnimations(const DrivingAnimations& drivingAnimations);
        void PopDrivingAnimations();

        // Resets the driving animations to default
        void ClearAllDrivingAnimations();
      
        // Returns true if the drivingEnd animation is playing
        bool IsPlayingEndAnim() const { return _endAnimStarted && !_endAnimCompleted; }
      
        // Starts playing drivingStart or drivingLoop if drivingStart isn't specified
        void PlayStartAnim(u8 tracksToUnlock);
      
        // Cancels drivingStart and drivingLoop animations and starts playing drivingEnd animation
        bool PlayEndAnim();
      
        // Called when the Driving action is being destroyed
        void ActionIsBeingDestroyed();
      
        bool IsCurrentlyPlayingAnimation() const { return (_startedPlayingAnimation || _endAnimStarted); }
      
      private:
      
        // Listens for driving animations to complete and handles what animation to play next
        void HandleActionCompleted(const ExternalInterface::RobotCompletedAction& msg);
      
        void UpdateCurrDrivingAnimations();

        // Queues the respective driving animation
        void PlayDrivingStartAnim();
        void PlayDrivingLoopAnim();
        void PlayDrivingEndAnim();
      
        Robot& _robot;
      
        std::vector<DrivingAnimations> _drivingAnimationStack;
        DrivingAnimations _currDrivingAnimations;
      
        const DrivingAnimations kDefaultDrivingAnimations;
        const DrivingAnimations kAngryDrivingAnimations;
      
      
        bool _startedPlayingAnimation = false;
      
        u8 _tracksToUnlock = (u8)AnimTrackFlag::NO_TRACKS;
      
        std::vector<Signal::SmartHandle> _signalHandles;
        bool _endAnimStarted = false;
        bool _endAnimCompleted = false;
        u32 _drivingStartAnimTag = ActionConstants::INVALID_TAG;
        u32 _drivingLoopAnimTag = ActionConstants::INVALID_TAG;
        u32 _drivingEndAnimTag = ActionConstants::INVALID_TAG;
    };
  }
}

#endif
