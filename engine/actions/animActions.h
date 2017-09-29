/**
 * File: animActions.h
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements animation and audio cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_ANIM_ACTIONS_H
#define ANKI_COZMO_ANIM_ACTIONS_H

#include "engine/actions/actionInterface.h"
#include "engine/actions/compoundActions.h"
#include "engine/animations/animationStreamer.h"
#include "engine/components/animationComponent.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "clad/types/actionTypes.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/types/animationTrigger.h"


namespace Anki {
  
  namespace Cozmo {

    class PlayAnimationAction : public IAction
    {
    public:
      PlayAnimationAction(Robot& robot,
                          const std::string& animName,
                          u32 numLoops = 1,
                          bool interruptRunning = true,
                          u8 tracksToLock = (u8)AnimTrackFlag::NO_TRACKS,
                          float timeout_sec = _kDefaultTimeout_sec);
      // Constructor for playing an Animation object (e.g. a "live" one created dynamically)
      // Caller owns the animation -- it will not be deleted by this action.
      // Numloops 0 causes the action to loop forever
      // tracksToLock indicates tracks of the animation which should not play
      PlayAnimationAction(Robot& robot,
                          Animation* animation,
                          u32 numLoops = 1,
                          bool interruptRunning = true,
                          u8 tracksToLock = (u8)AnimTrackFlag::NO_TRACKS,
                          float timeout_sec = _kDefaultTimeout_sec);
      
      virtual ~PlayAnimationAction();
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
      virtual f32 GetTimeoutInSeconds() const override { return _timeout_sec; }
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      std::string               _animName;
      u32                       _numLoopsRemaining;
      bool                      _stoppedPlaying;
      bool                      _wasAborted;
      bool                      _interruptRunning;
      Animation*                _animPointer = nullptr;
      float                     _timeout_sec = _kDefaultTimeout_sec;
      
      static constexpr float _kDefaultTimeout_sec = 60.f;
      static constexpr float _kDefaultTimeoutForInfiniteLoops_sec = std::numeric_limits<f32>::max();

    }; // class PlayAnimationAction


    class TriggerAnimationAction : public PlayAnimationAction
    {
    public:
      // Preferred constructor, used by the factory CreatePlayAnimationAction
      // Numloops 0 causes the action to loop forever
      explicit TriggerAnimationAction(Robot& robot,
                                      AnimationTrigger animEvent,
                                      u32 numLoops = 1,
                                      bool interruptRunning = true,
                                      u8 tracksToLock = (u8)AnimTrackFlag::NO_TRACKS,
                                      float timeout_sec = _kDefaultTimeout_sec);
      
    protected:
      virtual ActionResult Init() override;

      void SetAnimGroupFromTrigger(AnimationTrigger animTrigger);

      bool HasAnimTrigger() const { return _animTrigger != AnimationTrigger::Count; }

    private:
      AnimationTrigger _animTrigger;
      std::string _animGroupName;
      
    }; // class TriggerAnimationAction
    
    
    // A special subclass of TriggerAnimationAction which checks to see
    // if the robot is holding a cube and locks the tracks
    class TriggerLiftSafeAnimationAction : public TriggerAnimationAction
    {
    public:
      // Preferred constructor, used by the factory CreatePlayAnimationAction
      // Numloops 0 causes the action to loop forever
      explicit TriggerLiftSafeAnimationAction(Robot& robot,
                                      AnimationTrigger animEvent,
                                      u32 numLoops = 1,
                                      bool interruptRunning = true,
                                      u8 tracksToLock = (u8)AnimTrackFlag::NO_TRACKS);
      static u8 TracksToLock(Robot& robot, u8 tracksCurrentlyLocked);
      
      
    };

    #pragma mark ---- DeviceAudioAction ----
    // TODO: JIRA VIC-29 - Reimplement DeviceAudioAction
    
    #pragma mark ---- RobotAudioAction ----
    // TODO: VIC-30 - Implement RobotAudioAction

    
    class TriggerCubeAnimationAction : public IAction
    {
    private:
      // This action has a private constructor to prevent usage from within engine code. This is because
      // this action plays a cube light anim on the User/Game layer instead of the Engine layer
      // Note: If you want to control cube lights from within engine use PlayLightAnim() in cubeLightComponent
      friend IActionRunner* GetPlayCubeAnimationHelper(Robot& robot,
                                                       const ExternalInterface::PlayCubeAnimationTrigger& msg);
      
      // Plays a light animation on an object. The action will complete when the animation finishes
      TriggerCubeAnimationAction(Robot& robot,
                                 const ObjectID& objectID,
                                 const CubeAnimationTrigger& trigger);
      virtual ~TriggerCubeAnimationAction();
      
    protected:
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
    private:
      ObjectID _objectID;
      CubeAnimationTrigger _trigger = CubeAnimationTrigger::Count;
      bool _animEnded = false;
    };

    // if Cozmo is expressing a severe need, this action will automatically play the correct get-out
    // animation, and clear that need expression. Otherwise, it will succeed immediately
    class PlayNeedsGetOutAnimIfNeeded : public TriggerAnimationAction
    {
      using Base = TriggerAnimationAction;
    public:
      PlayNeedsGetOutAnimIfNeeded(Robot& robot);
      
      virtual ~PlayNeedsGetOutAnimIfNeeded();
      
    protected:
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;

    private:
      bool _hasClearedExpression = false;
    };
      
  
  }
}

#endif /* ANKI_COZMO_ANIM_ACTIONS_H */
