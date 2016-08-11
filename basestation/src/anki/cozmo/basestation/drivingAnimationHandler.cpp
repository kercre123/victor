/**
 * File: drivingAnimationHandler.cpp
 *
 * Author: Al Chaussee
 * Date:   5/6/2016
 *
 * Description: Handles playing animations while driving
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "drivingAnimationHandler.h"
#include "util/console/consoleInterface.h"

namespace Anki {
  namespace Cozmo {

    // Which docking method actions should use
    CONSOLE_VAR(bool, kEnableDrivingAnimations, "DrivingAnimationHandler", true);
    
    
    DrivingAnimationHandler::DrivingAnimationHandler(Robot& robot)
    : _robot(robot)
    , kDefaultDrivingAnimations({AnimationTrigger::DriveStartDefault,
                                 AnimationTrigger::DriveLoopDefault,
                                 AnimationTrigger::DriveEndDefault})
    , kAngryDrivingAnimations({AnimationTrigger::DriveStartAngry,
                               AnimationTrigger::DriveLoopAngry,
                               AnimationTrigger::DriveEndAngry})
    {

      _currDrivingAnimations = kDefaultDrivingAnimations;
      
      if(_robot.HasExternalInterface())
      {
        _signalHandles.push_back(_robot.GetExternalInterface()->Subscribe(
                                   ExternalInterface::MessageEngineToGameTag::RobotCompletedAction,
        [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
        {
          ASSERT_NAMED(event.GetData().GetTag() == ExternalInterface::MessageEngineToGameTag::RobotCompletedAction,
                       "Wrong event type from callback");
          HandleActionCompleted(event.GetData().Get_RobotCompletedAction());
        } ));
        
        _signalHandles.push_back(_robot.GetExternalInterface()->Subscribe(
                                   ExternalInterface::MessageGameToEngineTag::PushDrivingAnimations,
        [this](const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
        {
          auto const& payload = event.GetData().Get_PushDrivingAnimations();
          PushDrivingAnimations({payload.drivingStartAnim, payload.drivingLoopAnim, payload.drivingEndAnim});
        }));
        
        _signalHandles.push_back(_robot.GetExternalInterface()->Subscribe(
                                   ExternalInterface::MessageGameToEngineTag::PopDrivingAnimations,
        [this](const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
        {
          PopDrivingAnimations();
        }));
      }
    }
    
    void DrivingAnimationHandler::PushDrivingAnimations(const DrivingAnimations& drivingAnimations)
    {
      if(IsCurrentlyPlayingAnimation())
      {
        PRINT_NAMED_WARNING("DrivingAnimationHandler.PushDrivingAnimations",
                            "Pushing new animations while currently playing");
      }
      _drivingAnimationStack.push_back(drivingAnimations);
    }
    
    void DrivingAnimationHandler::PopDrivingAnimations()
    {
      if(IsCurrentlyPlayingAnimation())
      {
        PRINT_NAMED_WARNING("DrivingAnimationHandler.PopDrivingAnimations",
                            "Popping animations while currently playing");
      }
    
      if (_drivingAnimationStack.empty())
      {
        PRINT_NAMED_WARNING("DrivingAnimationHandler.PopDrivingAnimations",
                            "Tried to pop animations but the stack is empty!");
      }
      else
      {
        _drivingAnimationStack.pop_back();
      }
    }

    void DrivingAnimationHandler::ClearAllDrivingAnimations()
    {
      _drivingAnimationStack.clear();
    }

    void DrivingAnimationHandler::UpdateCurrDrivingAnimations()
    {
      if( _drivingAnimationStack.empty() ) {
        // use mood to determine which anims to play
        if( _robot.GetMoodManager().GetSimpleMood() == SimpleMoodType::Sad ) {
          _currDrivingAnimations = kAngryDrivingAnimations;
        }
        else {
          _currDrivingAnimations = kDefaultDrivingAnimations;
        }
      }
      else {
        _currDrivingAnimations = _drivingAnimationStack.back();
      }
    }

    void DrivingAnimationHandler::HandleActionCompleted(const ExternalInterface::RobotCompletedAction& msg)
    {
      // Only start playing drivingLoop if start successfully completes
      if(msg.idTag == _drivingStartAnimTag && msg.result == ActionResult::SUCCESS)
      {
        if(_currDrivingAnimations.drivingLoopAnim != AnimationTrigger::Count)
        {
          PlayDrivingLoopAnim();
        }
      }
      else if(msg.idTag == _drivingLoopAnimTag)
      {
        if(_robot.IsTraversingPath() && msg.result == ActionResult::SUCCESS)
        {
          PlayDrivingLoopAnim();
        }
        else
        {
          // Unlock our tracks so that endAnim can use them
          // This should be safe since we have finished driving
          _robot.GetMoveComponent().UnlockTracks(_tracksToUnlock);
          
          PlayDrivingEndAnim();
          _endAnimStarted = true;
        }
      }
      else if(msg.idTag == _drivingEndAnimTag)
      {
        _endAnimStarted = false;
        _endAnimCompleted = true;
        _startedPlayingAnimation = false;
        
        // Relock tracks like nothing ever happend
        _robot.GetMoveComponent().LockTracks(_tracksToUnlock);
      }
    }
    
    void DrivingAnimationHandler::ActionIsBeingDestroyed()
    {
      _startedPlayingAnimation = false;
      
      _robot.GetActionList().Cancel(_drivingStartAnimTag);
      _robot.GetActionList().Cancel(_drivingLoopAnimTag);
      _robot.GetActionList().Cancel(_drivingEndAnimTag);
    }
    
    void DrivingAnimationHandler::PlayStartAnim(u8 tracksToUnlock)
    {
      if (!kEnableDrivingAnimations) {
        return;
      }
      
      // If we have already been inited do nothing
      if(_startedPlayingAnimation)
      {
        return;
      }

      UpdateCurrDrivingAnimations();
        
      _endAnimCompleted = false;
      _endAnimStarted = false;
      _drivingStartAnimTag = ActionConstants::INVALID_TAG;
      _drivingLoopAnimTag = ActionConstants::INVALID_TAG;
      _drivingEndAnimTag = ActionConstants::INVALID_TAG;
      _tracksToUnlock = tracksToUnlock;
      
      if(_currDrivingAnimations.drivingLoopAnim != AnimationTrigger::Count)
      {
        PlayDrivingStartAnim();
        _startedPlayingAnimation = true;
      }
      else if(_currDrivingAnimations.drivingLoopAnim != AnimationTrigger::Count)
      {
        PlayDrivingLoopAnim();
        _startedPlayingAnimation = true;
      }
    }
    
    bool DrivingAnimationHandler::PlayEndAnim()
    {
      if (!kEnableDrivingAnimations) {
        return false;
      }
      
      _robot.GetActionList().Cancel(_drivingStartAnimTag);
      _robot.GetActionList().Cancel(_drivingLoopAnimTag);
      
      if(_currDrivingAnimations.drivingEndAnim != AnimationTrigger::Count && !_endAnimStarted && !_endAnimCompleted)
      {
        // Unlock our tracks so that endAnim can use them
        // This should be safe since we have finished driving
        _robot.GetMoveComponent().UnlockTracks(_tracksToUnlock);
        
        PlayDrivingEndAnim();
        _endAnimStarted = true;
      }
      return _endAnimStarted;
    }
  
    void DrivingAnimationHandler::PlayDrivingStartAnim()
    {
      IActionRunner* animAction = new TriggerAnimationAction(_robot, _currDrivingAnimations.drivingStartAnim, 1, true);
      _drivingStartAnimTag = animAction->GetTag();
      _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, animAction);
    }
    
    void DrivingAnimationHandler::PlayDrivingLoopAnim()
    {
      IActionRunner* animAction = new TriggerAnimationAction(_robot, _currDrivingAnimations.drivingLoopAnim, 1, true);
      _drivingLoopAnimTag = animAction->GetTag();
      _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, animAction);
    }
    
    void DrivingAnimationHandler::PlayDrivingEndAnim()
    {
      IActionRunner* animAction = new TriggerAnimationAction(_robot, _currDrivingAnimations.drivingEndAnim, 1, true);
      _drivingEndAnimTag = animAction->GetTag();
      _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, animAction);
    }
  }
}
