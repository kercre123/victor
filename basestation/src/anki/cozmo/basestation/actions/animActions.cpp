/**
 * File: animActions.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements animation and audio cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "animActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

namespace Anki {
  
  namespace Cozmo {

    #pragma mark ---- PlayAnimationAction ----

    PlayAnimationAction::PlayAnimationAction(Robot& robot, const std::string& animName,
                                             u32 numLoops, bool interruptRunning)
    : IAction(robot)
    , _animName(animName)
    , _name("PlayAnimation" + animName + "Action")
    , _numLoops(numLoops)
    , _interruptRunning(interruptRunning)
    {
      
    }
    
    PlayAnimationAction::~PlayAnimationAction()
    {
      // If we're cleaning up but we didn't hit the end of this animation and we haven't been cleanly aborted
      // by animationStreamer (the source of the event that marks _wasAborted), then expliclty tell animationStreamer
      // to clean up
      if(!_stoppedPlaying && !_wasAborted) {
        _robot.GetAnimationStreamer().SetStreamingAnimation(_robot, nullptr);
      }
    }

    ActionResult PlayAnimationAction::Init()
    {
      _startedPlaying = false;
      _stoppedPlaying = false;
      _wasAborted     = false;
      
      if (NeedsAlteredAnimation())
      {
        const Animation* streamingAnimation = _robot.GetAnimationStreamer().GetStreamingAnimation();
        const Animation* ourAnimation = _robot.GetCannedAnimation(_animName);
        
        _alteredAnimation = std::unique_ptr<Animation>(new Animation(*ourAnimation));
        assert(_alteredAnimation);
        
        bool useStreamingProcFace = !streamingAnimation->GetTrack<ProceduralFaceKeyFrame>().IsEmpty();
        
        if (useStreamingProcFace)
        {
          // Create a copy of the last procedural face frame of the streaming animation with the trigger time defaulted to 0
          auto lastFrame = streamingAnimation->GetTrack<ProceduralFaceKeyFrame>().GetLastKeyFrame();
          ProceduralFaceKeyFrame frameCopy(lastFrame->GetFace());
          _alteredAnimation->AddKeyFrameByTime(frameCopy);
        }
        else
        {
          // Create a copy of the last animating face frame of the streaming animation with the trigger time defaulted to 0
          auto lastFrame = streamingAnimation->GetTrack<FaceAnimationKeyFrame>().GetLastKeyFrame();
          FaceAnimationKeyFrame frameCopy(lastFrame->GetFaceImage(), lastFrame->GetName());
          _alteredAnimation->AddKeyFrameByTime(frameCopy);
        }
      }
      
      // If we've set our altered animation, use that
      if (_alteredAnimation)
      {
        _animTag = _robot.GetAnimationStreamer().SetStreamingAnimation(_robot, _alteredAnimation.get(), _numLoops, _interruptRunning);
      }
      else // do the normal thing
      {
        _animTag = _robot.PlayAnimation(_animName, _numLoops, _interruptRunning);
      }
      
      if(_animTag == AnimationStreamer::NotAnimatingTag) {
        _wasAborted = true;
        return ActionResult::FAILURE_ABORT;
      }
      
      using namespace RobotInterface;
      using namespace ExternalInterface;
      
      auto startLambda = [this](const AnkiEvent<RobotToEngine>& event)
      {
        if(this->_animTag == event.GetData().Get_animStarted().tag) {
          PRINT_NAMED_INFO("PlayAnimation.StartAnimationHandler", "Animation tag %d started", this->_animTag);
          _startedPlaying = true;
        }
      };
      
      auto endLambda = [this](const AnkiEvent<RobotToEngine>& event)
      {
        if(_startedPlaying && this->_animTag == event.GetData().Get_animEnded().tag) {
          PRINT_NAMED_INFO("PlayAnimation.EndAnimationHandler", "Animation tag %d ended", this->_animTag);
          _stoppedPlaying = true;
        }
      };
      
      auto cancelLambda = [this](const AnkiEvent<MessageEngineToGame>& event)
      {
        if(this->_animTag == event.GetData().Get_AnimationAborted().tag) {
          PRINT_NAMED_INFO("PlayAnimation.AbortAnimationHandler",
                           "Animation tag %d was aborted from running",
                           this->_animTag);
          _wasAborted = true;
        }
      };
      
      _startSignalHandle = _robot.GetRobotMessageHandler()->Subscribe(_robot.GetID(), RobotToEngineTag::animStarted, startLambda);
      
      _endSignalHandle   = _robot.GetRobotMessageHandler()->Subscribe(_robot.GetID(), RobotToEngineTag::animEnded,   endLambda);
      
      _abortSignalHandle = _robot.GetExternalInterface()->Subscribe(MessageEngineToGameTag::AnimationAborted, cancelLambda);
      
      if(_animTag != 0) {
        return ActionResult::SUCCESS;
      } else {
        return ActionResult::FAILURE_ABORT;
      }
    }

    bool PlayAnimationAction::NeedsAlteredAnimation() const
    {
      // Animations that don't interrupt never need to be altered
      if (!_interruptRunning)
      {
        return false;
      }
      
      const Animation* streamingAnimation = _robot.GetAnimationStreamer().GetStreamingAnimation();
      // Nothing is currently streaming so no need for alteration
      if (nullptr == streamingAnimation)
      {
        return false;
      }
      
      // The streaming animation has no face tracks, so no need for alteration
      if (streamingAnimation->GetTrack<ProceduralFaceKeyFrame>().IsEmpty() &&
          streamingAnimation->GetTrack<FaceAnimationKeyFrame>().IsEmpty())
      {
        return false;
      }
      
      // Now actually check our animation to see if we have an initial face frame
      const Animation* ourAnimation = _robot.GetCannedAnimation(_animName);
      assert(ourAnimation);
      
      bool animHasInitialFaceFrame = false;
      if (nullptr != ourAnimation)
      {
        auto procFaceTrack = ourAnimation->GetTrack<ProceduralFaceKeyFrame>();
        // If our track is not empty and starts at beginning
        if (!procFaceTrack.IsEmpty() && procFaceTrack.GetFirstKeyFrame()->GetTriggerTime() == 0)
        {
          animHasInitialFaceFrame = true;
        }
        
        auto faceAnimTrack = ourAnimation->GetTrack<FaceAnimationKeyFrame>();
        // If our track is not empty and starts at beginning
        if (!faceAnimTrack.IsEmpty() && faceAnimTrack.GetFirstKeyFrame()->GetTriggerTime() == 0)
        {
          animHasInitialFaceFrame = true;
        }
      }
      
      // If we have an initial face frame, no need to alter the animation
      return !animHasInitialFaceFrame;
    }

    ActionResult PlayAnimationAction::CheckIfDone()
    {
      if(_stoppedPlaying) {
        return ActionResult::SUCCESS;
      } else if(_wasAborted) {
        return ActionResult::FAILURE_ABORT;
      } else {
        return ActionResult::RUNNING;
      }
    }

    void PlayAnimationAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      AnimationCompleted info;
      info.animationName = _animName;
      completionUnion.Set_animationCompleted(std::move( info ));
    }

    #pragma mark ---- PlayAnimationGroupAction ----

    PlayAnimationGroupAction::PlayAnimationGroupAction(Robot& robot,
                                                       const std::string& animGroupName,
                                                       u32 numLoops, bool interruptRunning)
    : PlayAnimationAction(robot, "", numLoops, interruptRunning),
    _animGroupName(animGroupName)
    {
      
    }

    ActionResult PlayAnimationGroupAction::Init()
    {
      _animName = _robot.GetAnimationNameFromGroup(_animGroupName);
      if( _animName.empty() ) {
        return ActionResult::FAILURE_ABORT;
      }
      else {
        return PlayAnimationAction::Init();
      }
    }

    #pragma mark ---- DeviceAudioAction ----

    DeviceAudioAction::DeviceAudioAction(Robot& robot,
                                         const Audio::GameEvent::GenericEvent event,
                                         const Audio::GameObjectType gameObj,
                                         const bool waitUntilDone)
    : IAction(robot)
    , _actionType( AudioActionType::Event )
    , _name( "PlayAudioEvent_" + std::string(EnumToString(event)) + "_GameObj_" + std::string(EnumToString(gameObj)) )
    , _waitUntilDone( waitUntilDone )
    , _event( event )
    , _gameObj( gameObj )
    { }

    // Stop All Events on Game Object, pass in Invalid to stop all audio
    DeviceAudioAction::DeviceAudioAction(Robot& robot,
                                         const Audio::GameObjectType gameObj)
    : IAction(robot)
    , _actionType( AudioActionType::StopEvents )
    , _name( "StopAudioEvents_GameObj_" + std::string(EnumToString(gameObj)) )
    , _gameObj( gameObj )
    { }

    // Change Music state
    DeviceAudioAction::DeviceAudioAction(Robot& robot, const Audio::GameState::Music state)
    : IAction(robot)
    , _actionType( AudioActionType::SetState )
    , _name( "PlayAudioMusicState_" + std::string(EnumToString(state)) )
    , _stateGroup( Audio::GameState::StateGroupType::Music )
    , _state( static_cast<Audio::GameState::GenericState>(state) )
    { }

    void DeviceAudioAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      DeviceAudioCompleted info;
      info.audioEvent = _event;
      completionUnion.Set_deviceAudioCompleted(std::move( info ));
    }

    ActionResult DeviceAudioAction::Init()
    {
      using namespace Audio;
      switch ( _actionType ) {
        case AudioActionType::Event:
        {
          if (_waitUntilDone) {
            
            const AudioEngineClient::CallbackFunc callback = [this] ( AudioCallback callback )
            {
              const AudioCallbackInfoTag tag = callback.callbackInfo.GetTag();
              if (AudioCallbackInfoTag::callbackComplete == tag ||
                  AudioCallbackInfoTag::callbackError == tag) /* -- Waiting to hear back from WWise about error case -- */ {
                _isCompleted = true;
              }
            };
            
            _robot.GetRobotAudioClient()->PostEvent(_event, _gameObj, callback);
          }
          else {
            _robot.GetRobotAudioClient()->PostEvent(_event, _gameObj);
            _isCompleted = true;
          }
        }
          break;
          
        case AudioActionType::StopEvents:
        {
          _robot.GetRobotAudioClient()->StopAllEvents(_gameObj);
          _isCompleted = true;
        }
          break;
          
        case AudioActionType::SetState:
        {
          // FIXME: This is temp until we add boot process which will start music at launch
          if (Audio::GameState::StateGroupType:: Music == _stateGroup) {
            static bool didStartMusic = false;
            if (!didStartMusic) {
              _robot.GetRobotAudioClient()->PostEvent( static_cast<Audio::GameEvent::GenericEvent>(GameEvent::Music::Play), GameObjectType::Default );
              didStartMusic = true;
            }
          }
          
          _robot.GetRobotAudioClient()->PostGameState(_stateGroup, _state);
          _isCompleted = true;
        }
          break;
      }
      
      return ActionResult::SUCCESS;
    }

    ActionResult DeviceAudioAction::CheckIfDone()
    {
      return _isCompleted ? ActionResult::SUCCESS : ActionResult::RUNNING;
    }
  }
}
