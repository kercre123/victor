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
#include "clad/types/gameEvent.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/basestation/animation/animationStreamer.h"
#include "clad/types/animationKeyFrames.h"

namespace Anki {
  
  namespace Cozmo {

    class PlayAnimationAction : public IAction
    {
    public:
      // Deprecated constructor hardcoded use of string names
      PlayAnimationAction(Robot& robot,
                          const std::string& animName,
                          u32 numLoops = 1,
                          bool interruptRunning = true);
      // Preferred constructor, used by the factory CreatePlayAnimationAction
      PlayAnimationAction(Robot& robot,
                          GameEvent animName,
                          u32 numLoops = 1,
                          bool interruptRunning = true);
      // Backup constructor for transition, allows to use designer driven config or hardcoded backup
      PlayAnimationAction(Robot& robot,
                          GameEvent animName,
                          const std::string& backupAnimName,
                          u32 numLoops = 1,
                          bool interruptRunning = true);
      // Constructor for playing an Animation object (e.g. a "live" one created dynamically)
      PlayAnimationAction(Robot& robot,
                          Animation* animation,
                          u32 numLoops = 1,
                          bool interruptRunning = true);
      
      virtual ~PlayAnimationAction();
      
      virtual const std::string& GetName() const override { return _name; }
      virtual RobotActionType GetType() const override { return RobotActionType::PLAY_ANIMATION; }
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
      virtual f32 GetTimeoutInSeconds() const override { return 60.f; }
      
      virtual u8 GetTracksToLock() const override { return (u8)AnimTrackFlag::NO_TRACKS; }
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      std::string               _animName;
      std::string               _name;
      u32                       _numLoops;
      bool                      _startedPlaying;
      bool                      _stoppedPlaying;
      bool                      _wasAborted;
      AnimationStreamer::Tag    _animTag = AnimationStreamer::NotAnimatingTag;
      bool                      _interruptRunning;
      Animation*                _animation = nullptr;
      
      // For responding to AnimationStarted and AnimationEnded events
      Signal::SmartHandle _startSignalHandle;
      Signal::SmartHandle _endSignalHandle;
      Signal::SmartHandle _abortSignalHandle;
      
    private:
      // For handling playing an altered copy of an animation
      std::unique_ptr<Animation> _alteredAnimation;
      
      const Animation* GetOurAnimation() const;
      bool NeedsAlteredAnimation() const;
      
    }; // class PlayAnimationAction


    class PlayAnimationGroupAction : public PlayAnimationAction
    {
    public:
      // Deprecated
      explicit PlayAnimationGroupAction(Robot& robot,
                                        const std::string& animGroupName,
                                        u32 numLoops = 1,
                                        bool interruptRunning = true);
      // Preferred constructor, used by the factory CreatePlayAnimationAction
      explicit PlayAnimationGroupAction(Robot& robot,
                                        GameEvent animEvent,
                                        u32 numLoops = 1,
                                        bool interruptRunning = true);
      
    protected:
      virtual ActionResult Init() override;
      
      std::string   _animGroupName;
      
    }; // class PlayAnimationGroupAction
    
    // Checks if something is an animation or a group and returns the right action
    PlayAnimationAction* CreatePlayAnimationAction(Robot& robot, GameEvent animEvent,
                                                    u32 numLoops = 1,bool interruptRunning = true);
    // This will fallback on a hardcoded name if used.
    PlayAnimationAction* CreatePlayAnimationAction(Robot& robot, GameEvent animEvent, const std::string& backupAnimName,
                        u32 numLoops = 1,bool interruptRunning = true);


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
      
      virtual const std::string& GetName() const override { return _name; }
      virtual RobotActionType GetType() const override { return RobotActionType::DEVICE_AUDIO; }
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
      virtual u8 GetTracksToLock() const override { return (u8)AnimTrackFlag::NO_TRACKS; }
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      enum class AudioActionType : uint8_t {
        Event = 0,
        StopEvents,
        SetState,
      };
      
      AudioActionType                   _actionType;
      std::string                       _name;
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