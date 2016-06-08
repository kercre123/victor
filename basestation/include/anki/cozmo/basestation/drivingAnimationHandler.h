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
#include <vector>

namespace Anki {
  namespace Cozmo {
  
    class Robot;
    
    // Default driving animations
    static const char* kDefaultDrivingStartAnim = "ag_driving01_startDriving";
    static const char* kDefaultDrivingLoopAnim = "ag_driving01_drivingLoop";
    static const char* kDefaultDrivingEndAnim = "ag_driving01_endDriving";
    
    class DrivingAnimationHandler
    {
      public:
      
        // Subscribes to ActionCompleted and SetDrivingAnimations messages
        DrivingAnimationHandler(Robot& robot);
      
        // Container for the various driving animations
        struct DrivingAnimations
        {
          std::string drivingStartAnim;
          std::string drivingLoopAnim;
          std::string drivingEndAnim;
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
      
        // Queues the respective driving animation
        void PlayDrivingStartAnim();
        void PlayDrivingLoopAnim();
        void PlayDrivingEndAnim();
      
        Robot& _robot;
      
        std::vector<DrivingAnimations> _drivingAnimationStack;
        const DrivingAnimations kDefaultDrivingAnimations;
      
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
