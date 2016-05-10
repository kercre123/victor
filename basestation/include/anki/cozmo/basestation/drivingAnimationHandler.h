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
    static const char* kDefaultDrivingStartAnim = "";
    static const char* kDefaultDrivingLoopAnim = "";
    static const char* kDefaultDrivingEndAnim = "";
    
    class DrivingAnimationHandler
    {
      public:
      
        // Subscribes to ActionCompleted and SetDrivingAnimations messages
        DrivingAnimationHandler(Robot& robot);
      
        // Sets the driving animations
        void SetDrivingAnimations(const std::string& drivingStartAnim,
                                  const std::string& drivingLoopAnim,
                                  const std::string& drivingEndAnim);
      
        // Returns true if the drivingEnd animation is playing
        bool IsPlayingEndAnim() const { return _endAnimStarted && !_endAnimCompleted; }
      
        // Starts playing drivingStart or drivingLoop if drivingStart isn't specified
        void PlayStartAnim(u8 tracksToUnlock);
      
        // Cancels drivingStart and drivingLoop animations and starts playing drivingEnd animation
        bool PlayEndAnim();
      
        // Called when the Driving action is being destroyed
        void ActionIsBeingDestroyed();
      
      private:
      
        // Listens for driving animations to complete and handles what animation to play next
        void HandleActionCompleted(const ExternalInterface::RobotCompletedAction& msg);
      
        // Queues the respective driving animation
        void PlayDrivingStartAnim();
        void PlayDrivingLoopAnim();
        void PlayDrivingEndAnim();
      
        Robot& _robot;
      
        bool _startedPlayingAnimation = false;
      
        u8 _tracksToUnlock = (u8)AnimTrackFlag::NO_TRACKS;
      
        std::vector<Signal::SmartHandle> _signalHandles;
        bool _endAnimStarted = false;
        bool _endAnimCompleted = false;
        std::string _drivingStartAnim = kDefaultDrivingStartAnim;
        std::string _drivingLoopAnim = kDefaultDrivingLoopAnim;
        std::string _drivingEndAnim = kDefaultDrivingEndAnim;
        int _drivingStartAnimTag = -1;
        int _drivingLoopAnimTag = -1;
        int _drivingEndAnimTag = -1;
    };
  }
}

#endif