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

#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/common/basestation/math/pose.h"
#include "clad/types/actionTypes.h"
#include "clad/types/animationTrigger.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/basestation/animation/animationStreamer.h"
#include "clad/types/animationKeyFrames.h"

namespace Anki {
  
  namespace Cozmo {

    class PlayAnimationAction : public IAction
    {
    public:
      PlayAnimationAction(Robot& robot,
                          const std::string& animName,
                          u32 numLoops = 1,
                          bool interruptRunning = true);
      // Constructor for playing an Animation object (e.g. a "live" one created dynamically)
      // Caller owns the animation -- it will not be deleted by this action.
      // Numloops 0 causes the action to loop forever
      PlayAnimationAction(Robot& robot,
                          Animation* animation,
                          u32 numLoops = 1,
                          bool interruptRunning = true);
      
      virtual ~PlayAnimationAction();
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
      virtual f32 GetTimeoutInSeconds() const override { return 60.f; }
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      std::string               _animName;
      u32                       _numLoopsRemaining;
      bool                      _startedPlaying;
      bool                      _stoppedPlaying;
      bool                      _wasAborted;
      AnimationStreamer::Tag    _animTag = AnimationStreamer::NotAnimatingTag;
      bool                      _interruptRunning;
      Animation*                _animPointer = nullptr;
      
      // For responding to AnimationStarted, AnimationEnded, and AnimationEvent events
      Signal::SmartHandle _startSignalHandle;
      Signal::SmartHandle _endSignalHandle;
      Signal::SmartHandle _eventSignalHandle;
      Signal::SmartHandle _abortSignalHandle;
      
    }; // class PlayAnimationAction


    class TriggerAnimationAction : public PlayAnimationAction
    {
    public:
      // Preferred constructor, used by the factory CreatePlayAnimationAction
      // Numloops 0 causes the action to loop forever
      explicit TriggerAnimationAction(Robot& robot,
                                      AnimationTrigger animEvent,
                                      u32 numLoops = 1,
                                      bool interruptRunning = true);
      
    protected:
      virtual ActionResult Init() override;
      
      std::string   _animGroupName;
      
    }; // class TriggerAnimationAction
    

    class DeviceAudioAction : public IAction
    {
    public:
      // Play Audio Event
      // TODO: Add bool to set if caller want's to block "wait" until audio is completed
      DeviceAudioAction(Robot& robot,
                        const Audio::GameEvent::GenericEvent event,
                        const Audio::GameObjectType gameObj,
                        const bool waitUntilDone = false);
      
      // Stop All Events on Game Object, pass in Invalid to stop all audio
      DeviceAudioAction(Robot& robot, const Audio::GameObjectType gameObj = Audio::GameObjectType::Invalid);
      
      // Change Music state
      DeviceAudioAction(Robot& robot, const Audio::GameState::Music state);
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      enum class AudioActionType : uint8_t {
        Event = 0,
        StopEvents,
        SetState,
      };
      
      AudioActionType                   _actionType;
      bool                              _isCompleted    = false;
      bool                              _waitUntilDone  = false;
      Audio::GameEvent::GenericEvent    _event          = Audio::GameEvent::GenericEvent::Invalid;
      Audio::GameObjectType             _gameObj        = Audio::GameObjectType::Invalid;
      Audio::GameState::StateGroupType  _stateGroup     = Audio::GameState::StateGroupType::Invalid;
      Audio::GameState::GenericState    _state          = Audio::GameState::GenericState::Invalid;
      
    }; // class PlayAudioAction
  }
}

#endif /* ANKI_COZMO_ANIM_ACTIONS_H */
