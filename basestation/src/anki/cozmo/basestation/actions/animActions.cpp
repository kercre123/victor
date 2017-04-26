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

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/animationContainers/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotManager.h"

namespace Anki {
  
  namespace Cozmo {

    #pragma mark ---- PlayAnimationAction ----

    PlayAnimationAction::PlayAnimationAction(Robot& robot,
                                             const std::string& animName,
                                             u32 numLoops,
                                             bool interruptRunning,
                                             u8 tracksToLock,
                                             float timeout_sec)
    : IAction(robot,
              "PlayAnimation" + animName,
              RobotActionType::PLAY_ANIMATION,
              tracksToLock)
    , _animName(animName)
    , _numLoopsRemaining(numLoops)
    , _interruptRunning(interruptRunning)
    , _timeout_sec(timeout_sec)
    {
      
    }
    
    PlayAnimationAction::PlayAnimationAction(Robot& robot,
                                             Animation* animation,
                                             u32 numLoops,
                                             bool interruptRunning,
                                             u8 tracksToLock,
                                             float timeout_sec)
    : IAction(robot,
              "PlayAnimation" + animation->GetName(),
              RobotActionType::PLAY_ANIMATION,
              tracksToLock)
    , _animName(animation->GetName())
    , _numLoopsRemaining(numLoops)
    , _interruptRunning(interruptRunning)
    , _animPointer(animation)
    , _timeout_sec(timeout_sec)
    {
     
    }
    
    PlayAnimationAction::~PlayAnimationAction()
    {
      // If we're cleaning up but we didn't hit the end of this animation and we haven't been cleanly aborted
      // by animationStreamer (the source of the event that marks _wasAborted), then explicitly tell animationStreamer
      // to clean up
      if (_robot.GetAnimationStreamer().GetStreamingAnimation() == _animPointer) {
        PRINT_NAMED_INFO("PlayAnimationAction.Destructor.StillStreaming",
                         "Action destructing, but AnimationStreamer is still streaming this animation(%s). Telling"
                         "AnimationStreamer to stream null.", _animName.c_str());
        _robot.GetAnimationStreamer().SetStreamingAnimation(nullptr);
      }
    }

    ActionResult PlayAnimationAction::Init()
    {
      _startedPlaying = false;
      _stoppedPlaying = false;
      _wasAborted     = false;
      
      if (nullptr == _animPointer)
      {
        _animPointer = _robot.GetContext()->GetRobotManager()->GetCannedAnimations().GetAnimation(_animName);
      }
      
      _animTag = _robot.GetAnimationStreamer().SetStreamingAnimation(_animPointer, _numLoopsRemaining, _interruptRunning);
      
      if(_animTag == AnimationStreamer::NotAnimatingTag) {
        _wasAborted = true;
        return ActionResult::ANIM_ABORTED;
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
          if( _numLoopsRemaining == 0 ) {
            // If numLoopsRemaining == 0 before decrementing, it means it _started_
            // equal to zero, which means we were asked to loop this animation forever,
            // presumably to be cancelled by some other action at some point, or
            // until we timeout. This is valid, according to the constructor, but
            // log this situation, to help catch accidental infinite loop usage.
            PRINT_NAMED_INFO("PlayAnimation.EndAnimationHandler.LoopingForever",
                             "Animation tag %d finished a loop, continuing until timeout or cancel",
                             this->_animTag);
          }
          else {
            _numLoopsRemaining--;
            if( _numLoopsRemaining == 0 ) {
              PRINT_NAMED_INFO("PlayAnimation.EndAnimationHandler", "Animation tag %d ended", this->_animTag);
              _stoppedPlaying = true;
            }
            else {
              PRINT_NAMED_DEBUG("PlayAnimation.FinishedLoop", "Animation tag %d finished a loop, %d left",
                                this->_animTag, _numLoopsRemaining);
            }
          }
        }
      };
      
      auto eventLambda = [this](const AnkiEvent<RobotToEngine>& event)
      {
        RobotInterface::AnimationEvent payload = event.GetData().Get_animEvent();
        if(_startedPlaying && this->_animTag == payload.tag) {
            PRINT_NAMED_INFO("PlayAnimation.AnimationEventHandler",
                             "Event %s received at time %d while playing animation tag %d",
                             EnumToString(payload.event_id), payload.timestamp, this->_animTag);
            
          ExternalInterface::AnimationEvent msg;
          msg.timestamp = payload.timestamp;
          msg.event_id = payload.event_id;
          _robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::AnimationEvent>(std::move(msg));
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
      
      _startSignalHandle = _robot.GetRobotMessageHandler()->Subscribe(_robot.GetID(),
                                                                      RobotToEngineTag::animStarted,
                                                                      startLambda);
      
      _endSignalHandle = _robot.GetRobotMessageHandler()->Subscribe(_robot.GetID(),
                                                                    RobotToEngineTag::animEnded,
                                                                    endLambda);

      _eventSignalHandle = _robot.GetRobotMessageHandler()->Subscribe(_robot.GetID(),
                                                                      RobotToEngineTag::animEvent,
                                                                      eventLambda);
      
      _abortSignalHandle = _robot.GetExternalInterface()->Subscribe(MessageEngineToGameTag::AnimationAborted,
                                                                    cancelLambda);
      
      if(_animTag != 0) {
        return ActionResult::SUCCESS;
      } else {
        return ActionResult::ANIM_ABORTED;
      }
    }

    ActionResult PlayAnimationAction::CheckIfDone()
    {
      if(_stoppedPlaying) {
        return ActionResult::SUCCESS;
      } else if(_wasAborted) {
        return ActionResult::ANIM_ABORTED;
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

    #pragma mark ---- TriggerAnimationAction ----
    
    TriggerAnimationAction::TriggerAnimationAction(Robot& robot,
                             AnimationTrigger animEvent,
                             u32 numLoops,
                             bool interruptRunning,
                             u8 tracksToLock,
                             float timeout_sec)
    : PlayAnimationAction(robot, "", numLoops, interruptRunning, tracksToLock, timeout_sec)
    , _animTrigger(animEvent)
    , _animGroupName("")
    {
      RobotManager* robot_mgr = robot.GetContext()->GetRobotManager();
      if( robot_mgr->HasAnimationForTrigger(animEvent) )
      {
        _animGroupName = robot_mgr->GetAnimationForTrigger(animEvent);
        if(_animGroupName.empty()) {
          PRINT_NAMED_WARNING("TriggerAnimationAction.EmptyAnimGroupNameForTrigger",
                              "Event: %s", EnumToString(animEvent));
        }
      } else {
        PRINT_NAMED_WARNING("TriggerAnimationAction.NoAnimationForTrigger",
                            "Event: %s", EnumToString(animEvent));
      }
      // will FAILURE_ABORT on Init if not an event
    }

    ActionResult TriggerAnimationAction::Init()
    {
      // If animGroupName is empty we will already print a warning in the constructor so we can just fail immediately
      if(_animGroupName.empty())
      {
        return ActionResult::NO_ANIM_NAME;
      }
      
      _animName = _robot.GetAnimationStreamer().GetAnimationNameFromGroup(_animGroupName, _robot);
      if( _animName.empty() ) {
        return ActionResult::NO_ANIM_NAME;
      }
      else {
        const ActionResult res = PlayAnimationAction::Init();
        if( res == ActionResult::SUCCESS ) {
          const std::string& dataStr = std::string(AnimationTriggerToString(_animTrigger)) + ":" + _animGroupName;
          Anki::Util::sEvent("robot.play_animation",
                             {{DDATA, dataStr.c_str()}},
                             _animName.c_str());
        }

        return res;
      }
    }

    
    #pragma mark ---- TriggerLiftSafeAnimationAction ----

    TriggerLiftSafeAnimationAction::TriggerLiftSafeAnimationAction(Robot& robot,
                                            AnimationTrigger animEvent,
                                            u32 numLoops,
                                            bool interruptRunning,
                                            u8 tracksToLock)
    : TriggerAnimationAction(robot, animEvent, numLoops, interruptRunning,TracksToLock(robot, tracksToLock))
    {
    }
    
    
    u8 TriggerLiftSafeAnimationAction::TracksToLock(Robot& robot, u8 tracksCurrentlyLocked)
    {
      
      // Ensure animation doesn't throw cube down, but still can play get down animations
      if(robot.IsCarryingObject()
         && robot.GetOffTreadsState() == OffTreadsState::OnTreads){
        tracksCurrentlyLocked = tracksCurrentlyLocked | (u8) AnimTrackFlag::LIFT_TRACK;
      }
      
      return tracksCurrentlyLocked;
    }

    

    #pragma mark ---- DeviceAudioAction ----

    DeviceAudioAction::DeviceAudioAction(Robot& robot,
                                         const AudioMetaData::GameEvent::GenericEvent event,
                                         const AudioMetaData::GameObjectType gameObj,
                                         const bool waitUntilDone)
    : IAction(robot,
              "PlayAudioEvent_" + std::string(EnumToString(event)) + "_GameObj_" + std::string(EnumToString(gameObj)),
              RobotActionType::DEVICE_AUDIO,
              (u8)AnimTrackFlag::NO_TRACKS)
    , _actionType( AudioActionType::Event )
    , _waitUntilDone( waitUntilDone )
    , _event( event )
    , _gameObj( gameObj )
    { }

    // Stop All Events on Game Object, pass in Invalid to stop all audio
    DeviceAudioAction::DeviceAudioAction(Robot& robot,
                                         const AudioMetaData::GameObjectType gameObj)
    : IAction(robot,
              "StopAudioEvents_GameObj_" + std::string(EnumToString(gameObj)),
              RobotActionType::DEVICE_AUDIO,
              (u8)AnimTrackFlag::NO_TRACKS)
    , _actionType( AudioActionType::StopEvents )
    , _gameObj( gameObj )
    { }

    // Change Music state
    DeviceAudioAction::DeviceAudioAction(Robot& robot, const AudioMetaData::GameState::Music state)
    : IAction(robot,
              "PlayAudioMusicState_" + std::string(EnumToString(state)),
              RobotActionType::DEVICE_AUDIO,
              (u8)AnimTrackFlag::NO_TRACKS)
    , _actionType( AudioActionType::SetState )
    , _stateGroup( AudioMetaData::GameState::StateGroupType::Music )
    , _state( static_cast<AudioMetaData::GameState::GenericState>(state) )
    { }

    void DeviceAudioAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      DeviceAudioCompleted info;
      info.audioEvent = _event;
      completionUnion.Set_deviceAudioCompleted(std::move( info ));
    }

    ActionResult DeviceAudioAction::Init()
    {
      switch ( _actionType ) {
        case AudioActionType::Event:
        {
          if (_waitUntilDone) {
            
            
            const  Audio::AudioEngineClient::CallbackFunc callback = [this] ( const AudioEngine::Multiplexer::AudioCallback& callback )
            {
              const  AudioEngine::Multiplexer::AudioCallbackInfoTag tag = callback.callbackInfo.GetTag();
              if ( AudioEngine::Multiplexer::AudioCallbackInfoTag::callbackComplete == tag ||
                   AudioEngine::Multiplexer::AudioCallbackInfoTag::callbackError == tag) /* -- Waiting to hear back from WWise about error case -- */ {
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
          if (AudioMetaData::GameState::StateGroupType::Music == _stateGroup) {
            static bool didStartMusic = false;
            if (!didStartMusic) {
              _robot.GetRobotAudioClient()->PostEvent( static_cast<AudioMetaData::GameEvent::GenericEvent>(AudioMetaData::GameEvent::Music::Play), AudioMetaData::GameObjectType::Default );
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
    
    
    
    TriggerCubeAnimationAction::TriggerCubeAnimationAction(Robot& robot,
                                                           const ObjectID& objectID,
                                                           const CubeAnimationTrigger& trigger)
    : IAction(robot,
              "TriggerCubeAnimation_" + std::string(EnumToString(trigger)),
              RobotActionType::PLAY_CUBE_ANIMATION,
              (u8)AnimTrackFlag::NO_TRACKS)
    , _objectID(objectID)
    , _trigger(trigger)
    {
    
    }
    
    TriggerCubeAnimationAction::~TriggerCubeAnimationAction()
    {
      // If the action has started and the light animation has not ended stop the animation
      if(HasStarted() && !_animEnded)
      {
        GetRobot().GetCubeLightComponent().StopLightAnimAndResumePrevious(_trigger);
      }
    }
    
    ActionResult TriggerCubeAnimationAction::Init()
    {
      // If the animation corresponding to the trigger has no duration we can't play it from an
      // action because then the action would never complete
      if(GetRobot().GetCubeLightComponent().GetAnimDuration(_trigger) == 0)
      {
        PRINT_NAMED_WARNING("TriggerCubeAnimationAction.AnimPlaysForever",
                            "AnimTrigger %s plays forever refusing to play in an action",
                            EnumToString(_trigger));
        _animEnded = true;
        return ActionResult::ABORT;
      }
    
      // Use a private function of CubeLightComponent in order to play on the user/game layer instead of
      // the engine layer
      const bool animPlayed = GetRobot().GetCubeLightComponent().PlayLightAnimFromAction(_objectID,
                                                                                            _trigger,
                                                                                            [this](){_animEnded = true;},
                                                                                            GetTag());
      if(!animPlayed)
      {
        _animEnded = true;
        return ActionResult::ABORT;
      }
      return ActionResult::SUCCESS;
    }
    
    ActionResult TriggerCubeAnimationAction::CheckIfDone()
    {
      return (_animEnded ? ActionResult::SUCCESS : ActionResult::RUNNING);
    }
  }
}
