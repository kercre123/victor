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
      virtual ~PlayAnimationAction();
      
      virtual const std::string& GetName() const override { return _name; }
      virtual RobotActionType GetType() const override { return RobotActionType::PLAY_ANIMATION; }
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
      virtual f32 GetTimeoutInSeconds() const override { return 60.f; }
      
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
      
      // For responding to AnimationStarted and AnimationEnded events
      Signal::SmartHandle _startSignalHandle;
      Signal::SmartHandle _endSignalHandle;
      Signal::SmartHandle _abortSignalHandle;
      
    private:
      // For handling playing an altered copy of an animation
      std::unique_ptr<Animation> _alteredAnimation;
      
      bool NeedsAlteredAnimation() const;
      
    }; // class PlayAnimationAction


    class PlayAnimationGroupAction : public PlayAnimationAction
    {
    public:
      explicit PlayAnimationGroupAction(Robot& robot,
                                        const std::string& animGroupName,
                                        u32 numLoops = 1,
                                        bool interruptRunning = true);
      
    protected:
      virtual ActionResult Init() override;
      
      std::string   _animGroupName;
      
    }; // class PlayAnimationGroupAction


    class DeviceAudioAction : public IAction
    {
    public:
      // Play Audio Event
      // TODO: Add bool to set if caller want's to block "wait" until audio is completed
      DeviceAudioAction(Robot& robot,
                        const Audio::GenericEvent event,
                        const Audio::GameObjectType gameObj,
                        const bool waitUntilDone = false);
      
      // Stop All Events on Game Object, pass in Invalid to stop all audio
      DeviceAudioAction(Robot& robot, const Audio::GameObjectType gameObj = Audio::GameObjectType::Invalid);
      
      // Change Music state
      DeviceAudioAction(Robot& robot, const Audio::MUSIC state);
      
      virtual const std::string& GetName() const override { return _name; }
      virtual RobotActionType GetType() const override { return RobotActionType::DEVICE_AUDIO; }
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      enum class AudioActionType : uint8_t {
        Event = 0,
        StopEvents,
        SetState,
      };
      
      AudioActionType           _actionType;
      std::string               _name;
      bool                      _isCompleted    = false;
      bool                      _waitUntilDone  = false;
      Audio::GenericEvent       _event          = Audio::GenericEvent::Invalid;
      Audio::GameObjectType     _gameObj        = Audio::GameObjectType::Invalid;
      Audio::StateGroupType     _stateGroup     = Audio::StateGroupType::Invalid;
      Audio::GenericState       _state          = Audio::GenericState::Invalid;
      
    }; // class PlayAudioAction
  }
}

#endif /* ANKI_COZMO_ANIM_ACTIONS_H */